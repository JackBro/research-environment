// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Receive routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "route.h"
#include "fragment.h"
#include "mobile.h"
#include "security.h"

#include "ipsec.h"

struct ReassemblyList ReassemblyList;

typedef struct Options {
    uint JumboLength;             // Length of packet excluding IPv6 header.
    IPv6RouterAlertOption *Alert; // REVIEW: Pointer to the Alert option.
    IPv6HomeAddressOption *HomeAddress;  // Home Address mobility option.
    IPv6BindingUpdateOption *BindingUpdate; // Binding Update mobility option.
} Options;

int  
OptionProcessing(
    IPv6Packet *Packet,     // The packet handed to us by IPv6Receive.
    uchar HdrType,          // Hop-by-hop or destination.
    IPv6OptionsHeader *Hdr, // The options header with following data.
    uint HdrLength,         // Length of the entire options area.
    Options *Opts);         // Return option values to caller.

extern void TCPRcvComplete(void);

//* IPv6ReceiveComplete - Handle a receive complete.
//
//  Called by the lower layer when receives are temporarily done.
//
void
IPv6ReceiveComplete(void)
{
    // REVIEW: Original IP implementation had code here to call every
    // REVIEW: UL protocol's receive complete routine (yes, all of them) here.

    TCPRcvComplete();  // BUGBUG: shouldn't be hard-wired.
}

//
// By default, test pullup in checked builds.
//
#ifndef PULLUP_TEST
#define PULLUP_TEST        DBG
#endif

#if PULLUP_TEST

#define PULLUP_TEST_MAX_BUFFERS                8
#define PULLUP_TEST_MAX_BUFFER_SIZE            32

//* PullupTestChooseDistribution
//
//  Choose a random distribution.
//  Divides Size bytes into NumBuffers pieces,
//  and returns the result in the Counts array.
//
void
PullupTestChooseDistribution(
    uint Counts[],
    uint NumBuffers,
    uint Size)
{
    uint i;
    uint ThisBuffer;

    //
    // We are somewhat biased towards cutting the packet
    // up into small pieces with a large remainder.
    // This puts the fragment boundaries at the beginning,
    // where the headers are.
    //

    for (i = 0; i < NumBuffers - 1; i++) {
        ThisBuffer = RandomNumber(1, PULLUP_TEST_MAX_BUFFER_SIZE);

        //
        // Make sure that each segment has non-zero length.
        //
        if (ThisBuffer > Size - (NumBuffers - 1 - i))
            ThisBuffer = Size - (NumBuffers - 1 - i);

        Counts[i] = ThisBuffer;
        Size -= ThisBuffer;
    }
    Counts[i] = Size;
}

//* PullupTestCreatePacket
//
//  Given an IPv6 packet, creates a new IPv6 packet
//  that can be handed up the receive path.
//
//  We randomly fragment the IPv6 packet into multiple buffers.
//  This tests pull-up processing in the receive path.
//
//  Because this is only used for testing,
//  we don't do real error checking/cleanup.
//
IPv6Packet *
PullupTestCreatePacket(IPv6Packet *Packet)
{
    IPv6Packet *TestPacket;

    //
    // We mostly want to test discontiguous packets.
    // But occasionally test a contiguous packet.
    //
    if (RandomNumber(0, 10) == 0) {
        //
        // We want a contiguous packet.
        //
        if (Packet->NdisPacket == NULL) {
            //
            // The packet is already contiguous.
            // We just need to shallow-copy it so the
            // free operation in IPv6Receive works.
            //
            TestPacket = ExAllocatePool(NonPagedPool, sizeof *TestPacket);
            ASSERT(TestPacket != NULL);
            *TestPacket = *Packet;
        }
        else {
            //
            // We need to create a contiguous packet.
            //
            uint MemLen;
            void *Mem;
            PNDIS_BUFFER NdisBuffer;
            uint Offset;

            MemLen = sizeof *TestPacket + Packet->TotalSize;
            TestPacket = ExAllocatePool(NonPagedPool, MemLen);
            ASSERT(TestPacket != NULL);
            Mem = (void *)(TestPacket + 1);

            NdisQueryPacket(Packet->NdisPacket, NULL, NULL, &NdisBuffer, NULL);
            Offset = Packet->Position;
            CopyNdisToFlat(Mem, NdisBuffer, Offset, Packet->TotalSize,
                           &NdisBuffer, &Offset);

            RtlZeroMemory(TestPacket, sizeof *TestPacket);
            TestPacket->Data = Mem;
            TestPacket->ContigSize = TestPacket->TotalSize = Packet->TotalSize;
            TestPacket->Flags = Packet->Flags;
        }
    }
    else {
        //
        // Create a packet with a single NDIS buffer.
        // Start with an over-estimate of the size of the MDLs we need.
        //
        uint NumPages = (Packet->TotalSize >> PAGE_SHIFT) + 2;
        uint MdlSize = sizeof(MDL) + (NumPages * sizeof(ULONG));
        uint MemLen;
        uint Counts[PULLUP_TEST_MAX_BUFFERS];
        uint NumBuffers;
        void *Mem;
        PNDIS_PACKET NdisPacket;
        PNDIS_BUFFER NdisBuffer;
        uint Garbage = 0xdeadbeef;
        uint i;

        //
        // Choose the number of buffers/MDLs that we will use
        // and the distribution of bytes into those buffers.
        //
        NumBuffers = RandomNumber(1, PULLUP_TEST_MAX_BUFFERS);
        PullupTestChooseDistribution(Counts, NumBuffers, Packet->TotalSize);

        //
        // Allocate all the memory that we will need.
        // (Actually a bit of an over-estimate.)
        //
        MemLen = (sizeof *TestPacket + sizeof(NDIS_PACKET) + 
                  NumBuffers * (MdlSize + sizeof Garbage) +
                  Packet->TotalSize);
        TestPacket = ExAllocatePool(NonPagedPool, MemLen);
        ASSERT(TestPacket != NULL);
        NdisPacket = (PNDIS_PACKET)(TestPacket + 1);
        NdisBuffer = (PNDIS_BUFFER)(NdisPacket + 1);
        Mem = (void *)((uchar *)NdisBuffer + NumBuffers * MdlSize);

        //
        // Initialize the NDIS packet and buffers.
        //
        RtlZeroMemory(NdisPacket, sizeof *NdisPacket);
        for (i = 0; i < NumBuffers; i++) {
            MmInitializeMdl(NdisBuffer, Mem, Counts[i]);
            MmBuildMdlForNonPagedPool(NdisBuffer);
            NdisChainBufferAtBack(NdisPacket, NdisBuffer);
            RtlCopyMemory((uchar *)Mem + Counts[i], &Garbage, sizeof Garbage);

            (uchar *)Mem += Counts[i] + sizeof Garbage;
            (uchar *)NdisBuffer += MdlSize;
        }

        //
        // Copy data to the new packet.
        //
        CopyToBufferChain((PNDIS_BUFFER)(NdisPacket + 1), 0,
                          Packet->NdisPacket, Packet->Position,
                          Packet->Data, Packet->TotalSize);

        //
        // Initialize the new packet.
        //
        InitializePacketFromNdis(TestPacket, NdisPacket, 0);
        TestPacket->Flags = Packet->Flags;
    }

    return TestPacket;
}
#endif // PULLUP_TEST


//* IPv6Receive - Receive an incoming IPv6 datagram.
//
//  This is the routine called by the link layer module when an incoming IPv6
//  datagram is to be processed.  We validate the datagram and decide what to
//  do with it.
//
//  NB: The datagram may either be held in a NDIS_PACKET allocated by the
//  link-layer or the interface driver (in which case 'Packet->NdisPacket'
//  is non-NULL and 'Data' points to the first data buffer in the buffer
//  chain), or the datagram may still be held by NDIS (in which case
//  'Packet->NdisPacket' is NULL and 'Data' points to a buffer containing
//  the entire datagram).
//
//  NB: We do NOT check for link-level multi/broadcasts to
//  IPv6 unicast destinations.  In the IPv4 world, receivers dropped
//  such packets, but in the IPv6 world they are accepted.
//
int                       // Returns: Reference count of packets held.
IPv6Receive(
    void *MyContext,      // Context value we gave to the link layer.
    IPv6Packet *Packet)   // Packet handed to us by the link layer.
{
    uchar *NextHeaderLocation;     // Ptr to current header's NextHeader field.
    uchar First = IP_PROTOCOL_V6;  // Always first header in packet.
    uchar * (*Handler)();
    SALinkage *ThisSA, *NextSA;
    int PktRefs;

#if PULLUP_TEST
    Packet = PullupTestCreatePacket(Packet);
#endif

    //
    // Depending upon who our caller is, we could be called with
    // either an NTE or an Interface.
    //
    Packet->NTEorIF = (NetTableEntryOrInterface *)MyContext;
    ASSERT((Packet->Flags & PACKET_HOLDS_NTE_REF) == 0);

    //
    // Iteratively switch out to the handler for each successive next header
    // until we reach a handler that reports no more headers follow it.
    //
    // NB: We do NOT check NTE->DADStatus here.
    // That is the responsibility of higher-level protocols.
    //
    NextHeaderLocation = &First;
    do {
        //
        // Current header indicates that another header follows.
        // See if we have a handler for it.
        //
        Handler = ProtocolSwitchTable[*NextHeaderLocation].DataReceive;
        if (Handler == NULL) {
            //
            // If we don't have a handler for this header type,
            // send an ICMP error message and drop the packet.
            // ICMP Pointer value is the offset from the start of the
            // incoming packet's IPv6 header to the offending field.
            //
            KdPrint(("IPv6 Receive: Next Header type %d isn't supported.\n",
                     *NextHeaderLocation));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM, 
                            ICMPv6_UNRECOGNIZED_NEXT_HEADER,
                            (GetPacketPositionFromPointer(Packet,
                                                          NextHeaderLocation) -
                             Packet->IPPosition),
                            *NextHeaderLocation, FALSE);
            break;
        }

        NextHeaderLocation = (*Handler)(Packet);
    } while (NextHeaderLocation != NULL);

    //
    // If this packet holds a reference on an NTE, free it now.
    //
    ASSERT(Packet->NTEorIF != NULL);
    if (Packet->Flags & PACKET_HOLDS_NTE_REF) {
        ASSERT(IsNTE(Packet->NTEorIF));
        ReleaseNTE(CastToNTE(Packet->NTEorIF));
    }

    //
    // Clean up any contiguous regions left by PacketPullup.
    //
    PacketPullupCleanup(Packet);

    //
    // Clean up list of SA's performed.
    //
    for (ThisSA = Packet->SAPerformed; ThisSA != NULL; ThisSA = NextSA) {
        NextSA = ThisSA->Next;
        if (--ThisSA->This->RefCount == 0) {
            FreeSA(ThisSA->This); 
        }

        ExFreePool(ThisSA);
    }

    PktRefs = Packet->RefCnt;
#if PULLUP_TEST
    ExFreePool(Packet);
#endif
    return PktRefs;
}


//* IPv6HeaderReceive - Handle a IPv6 header.
//
//  This is the routine called to process an IPv6 header, a next header
//  value of 41 (e.g. as would be encountered with v6 in v6 tunnels).  To
//  avoid code duplication, it is also used to process the initial IPv6
//  header found in all IPv6 packets, in which mode it may be viewed as
//  a continuation of IPv6Receive.
//
uchar *
IPv6HeaderReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    uint PayloadLength;
    uchar *NextHeaderLocation;
    int Forwarding;     // TRUE means Forwarding, FALSE means Receiving.

    //
    // Sanity-check ContigSize & TotalSize.
    // Higher-level code in the receive path relies on these conditions.
    //
    ASSERT((Packet->NdisPacket == NULL) ?
           (Packet->ContigSize == Packet->TotalSize) :
           (Packet->ContigSize <= Packet->TotalSize));

    //
    // Make sure we have enough contiguous bytes for an IPv6 header, otherwise
    // attempt to pullup that amount.  Then stash away a pointer to the header
    // and also remember the offset into the packet at which it begins (needed
    // to calculate an offset for certain ICMP error messages).
    //
    if (Packet->ContigSize < sizeof(IPv6Header)) {
        if (PacketPullup(Packet, sizeof(IPv6Header)) == 0) {
            // Pullup failed.
            KdPrint(("IPv6HdeaderReceive: "
                     "Packet too small to contain IPv6 header\n"));
            return NULL;
        }
    }
    Packet->IP = (IPv6Header UNALIGNED *)Packet->Data;
    Packet->SrcAddr = &Packet->IP->Source;
    Packet->IPPosition = Packet->Position;

    //
    // Skip over IPv6 header (note we keep our pointer to it).
    //
    AdjustPacketParams(Packet, sizeof(IPv6Header));

    //
    // Check the IP version is correct.
    // We specifically do NOT check HopLimit.
    // HopLimit is only checked when forwarding.
    //
    if ((Packet->IP->VersClassFlow & IP_VER_MASK) != IP_VERSION) {
        // Silently discard the packet.
        KdPrint(("IPv6HeaderReceive: bad version\n"));
        return NULL;
    }

    //
    // If we've already done some IPSec processing on this packet,
    // than this is a tunnel header and the preceeding IPSec header
    // is operating in tunnel mode.
    //
    if (Packet->SAPerformed != NULL)
        Packet->SAPerformed->Mode = TUNNEL;

    if (IsNTE(Packet->NTEorIF)) {
        NetTableEntry *NTE;

        //
        // We were called with an NTE.
        // Our caller (or the packet itself) should be holding a reference.
        //
        NTE = CastToNTE(Packet->NTEorIF);

        //
        // Verify that the packet's destination address is
        // consistent with this NTE.
        //
        if (!IP6_ADDR_EQUAL(&Packet->IP->Dest, &NTE->Address)) {
            //
            // We can't accept this new header on this NTE.
            // Convert to an Interface and punt to forwarding code below.
            //
            if (Packet->Flags & PACKET_HOLDS_NTE_REF) {
                Packet->Flags &= ~PACKET_HOLDS_NTE_REF;
                ReleaseNTE(NTE);
            }
            Packet->NTEorIF = CastFromIF(NTE->IF);
            goto Forward;
        }

        //
        // We are Receiving the packet.
        //
        Forwarding = FALSE;

    } else {
        NetTableEntryOrInterface *NTEorIF;
        ushort Type;

        //
        // We were called with an Interface.
        // Find an NTE on this interface that can accept the packet.
        //
        ASSERT((Packet->Flags & PACKET_HOLDS_NTE_REF) == 0);
        NTEorIF = FindAddressOnInterface(CastToIF(Packet->NTEorIF),
                                         &Packet->IP->Dest, &Type);
        if (NTEorIF == NULL) {
            //
            // The address is not assigned to this interface.  Check to see
            // if it is appropriate for us to forward this packet.
            // If not, drop it.  At this point, we are fairly
            // conservative about what we will forward.
            //
Forward:
            if (!(CastToIF(Packet->NTEorIF)->Flags & IF_FLAG_FORWARDS) ||
                (Packet->Flags & PACKET_NOT_LINK_UNICAST) ||
                !IsUniqueAddress(&Packet->IP->Source) ||
                IsLoopback(&Packet->IP->Source)) {
                //
                // Drop the packet with no ICMP error.
                //
                return NULL;
            }

            //
            // No support yet for forwarding multicast packets.
            //
            if (IsUnspecified(&Packet->IP->Dest) ||
                IsLoopback(&Packet->IP->Dest) ||
                IsMulticast(&Packet->IP->Dest)) {
                //
                // Send an ICMP error.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_DESTINATION_UNREACHABLE,
                                ICMPv6_COMMUNICATION_PROHIBITED,
                                0, Packet->IP->NextHeader, FALSE);
                return NULL;
            }

            //
            // We do the actual forwarding below...
            //
            Forwarding = TRUE;

        } else {
            //
            // We found an ADE on this IF to accept the packet,
            // so we will be Receiving it.
            //
            Forwarding = FALSE;

            //
            // If we found a unicast ADE, then remember the NTE.
            // Conceptually, we think of the packet as holding
            // the reference to the NTE. For multicast/anycast
            // addresses, we delay our choice of an appropriate NTE
            // until it is time to reply to the packet.
            //
            Packet->NTEorIF = NTEorIF;
            if (IsNTE(NTEorIF))
                Packet->Flags |= PACKET_HOLDS_NTE_REF;
        }
    }

    //
    // At this point, the Forwarding variable tells us
    // if we are forwarding or receiving the packet.
    //

    //
    // Before processing any headers, including Hop-by-Hop,
    // check that the amount of payload the IPv6 header thinks is present
    // can actually fit inside the packet data area that the link handed us.
    // Note that a Payload Length of zero *might* mean a Jumbo Payload option.
    //
    PayloadLength = net_short(Packet->IP->PayloadLength);
    if (PayloadLength > Packet->TotalSize) {
        // Silently discard the packet.
        KdPrint(("IPv6HeaderReceive: Header's PayloadLength is greater than "
                 "the amount of data received\n"));
        return NULL;
    }

    //
    // Check for Hop-by-Hop Options.
    //
    if (Packet->IP->NextHeader == IP_PROTOCOL_HOP_BY_HOP) {
        //
        // If there is a Jumbo Payload option, HopByHopOptionsReceive
        // will adjust the packet size. Otherwise we take care of it
        // now, before reading the Hop-by-Hop header.
        //
        if (PayloadLength != 0) {
            Packet->TotalSize = PayloadLength;
            if (Packet->ContigSize > PayloadLength)
                Packet->ContigSize = PayloadLength;
        }

        //
        // Parse the Hop-by-Hop options.
        //
        NextHeaderLocation = HopByHopOptionsReceive(Packet);
    } else {
        //
        // No Jumbo Payload option. Adjust the packet size.
        //
        Packet->TotalSize = PayloadLength;
        if (Packet->ContigSize > PayloadLength)
            Packet->ContigSize = PayloadLength;

        //
        // No Hop-by-Hop options.
        //
        NextHeaderLocation = &Packet->IP->NextHeader;
    }

    //
    // Check if we are forwarding this packet.
    //
    if (Forwarding) {
        IPv6Header UNALIGNED *FwdIP;
        NDIS_PACKET *FwdPacket;
        NDIS_STATUS NdisStatus;
        uint PayloadLength;
        uint Offset;
        uint MemLen;
        uchar *Mem;
        uint TunnelStart = NO_TUNNEL, IPSecBytes = 0;
        IPSecProc *IPSecToDo;
        uint Action;
        RouteCacheEntry *RCE;
        uint DestScopeId;
        IP_STATUS Status;

        if (NextHeaderLocation == NULL) {
            //
            // The packet had bad Hop-by-Hop Options.
            // We should not forward it.
            //
            return NULL;
        }

        //
        // Verify IPSec was performed.
        //
        if (InboundSPLookup(Packet, 0, 0, 0,
                            CastToIF(Packet->NTEorIF)) != TRUE) {
            // 
            // No SP was found or the SP found indicated to drop the packet.
            //
            KdPrint(("IPv6Receive: "
                     "IPSec lookup failed or policy was to drop\n"));        
            return NULL;
        }

        //
        // At this time, we need to copy the incoming packet,
        // for several reasons: We can't hold the Packet
        // once IPv6HeaderReceive returns, yet we need to queue
        // packet to forward it.  We need to modify the packet
        // (in IPv6Forward) by decrementing the hop count,
        // yet our incoming packet is read-only.  Finally,
        // we need space in the outgoing packet for the outgoing
        // interface's link-level header, which may differ in size
        // from that of the incoming interface.  Someday, we can
        // implement support for returning a non-zero reference
        // count from IPv6Receive and only copy the incoming
        // packet's header to construct the outgoing packet.
        //       

        //
        // Find a route to the new destination.
        //
        DestScopeId = DetermineScopeId(&Packet->IP->Dest,
                                       CastToIF(Packet->NTEorIF));
        Status = RouteToDestination(&Packet->IP->Dest, DestScopeId, NULL,
                                    RTD_FLAG_NORMAL, &RCE);
        if (Status != IP_SUCCESS) {
            KdPrint(("IPv6HeaderReceive: "
                     "No route to destination for forwarding.\n"));  
            ICMPv6SendError(Packet,
                            ICMPv6_DESTINATION_UNREACHABLE,
                            ICMPv6_NO_ROUTE_TO_DESTINATION,
                            0, *NextHeaderLocation, FALSE);
            return NULL;
        }

        //
        // Find the Security Policy for this outbound traffic.
        //
        IPSecToDo = OutboundSPLookup(&Packet->IP->Source, &Packet->IP->Dest, 
                                     0, 0, 0, RCE->NCE->IF, &Action);

        if (IPSecToDo == NULL) {
            //
            // Check Action.
            //
            if (Action == LOOKUP_DROP) {
                // Drop packet.                            
                ReleaseRCE(RCE);
                return NULL;
            } else {
                if (Action == LOOKUP_IKE_NEG) {
                    KdPrint(("IPv6HeaderReceive: IKE not supported yet.\n"));
                    ReleaseRCE(RCE);
                    return NULL;
                }
            }

            //
            // With no IPSec to perform, IPv6Forward won't be changing the
            // outgoing interface from what we currently think it will be.
            // So we can use the exact size of its link-level header.
            //
            Offset = RCE->NCE->IF->LinkHeaderSize;

        } else {
            //
            // Calculate the space needed for the IPSec headers.
            //
            IPSecBytes = IPSecBytesToInsert(IPSecToDo, &TunnelStart);

            if (TunnelStart != 0) {
                KdPrint(("IPv6HeaderReceive: IPSec Tunnel mode only.\n"));
                FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
                ReleaseRCE(RCE);
                return NULL;
            }

            //
            // The IPSec code in IPv6Forward might change the outgoing
            // interface from what we currently think it will be.
            // Leave the max amount of space for its link-level header.
            //
            Offset = MAX_LINK_HEADER_SIZE;
        }

        PayloadLength = Packet->TotalSize;
        MemLen = Offset + sizeof(IPv6Header) + PayloadLength + IPSecBytes;

        NdisStatus = IPv6AllocatePacket(MemLen, &FwdPacket, &Mem);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            if (IPSecToDo) {
                FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
            }
            ReleaseRCE(RCE);
            return NULL;  // We can't forward.
        }

        FwdIP = (IPv6Header UNALIGNED *)(Mem + Offset + IPSecBytes);

        //
        // Copy from the incoming packet to the outgoing packet.
        //
        CopyPacketToBuffer((uchar *)FwdIP, Packet,
                           sizeof(IPv6Header) + PayloadLength,
                           Packet->IPPosition);

        //
        // Send the outgoing packet.
        //
        IPv6Forward(Packet->NTEorIF, FwdPacket, Offset + IPSecBytes, FwdIP,
                    PayloadLength, TRUE, // OK to Redirect.
                    IPSecToDo, RCE);

        if (IPSecToDo) {
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
        }

        ReleaseRCE(RCE);

        return NULL;
    } // end of if (Forwarding)

    //
    // Packet is for this node.
    // Note: We may only be an intermediate node and not the packet's final
    // destination, if there is a routing header.
    //
    return NextHeaderLocation;
}


//* ReassemblyInit
//
//  Initialize data structures required for fragment reassembly.
//
void
ReassemblyInit(void)
{
    KeInitializeSpinLock(&ReassemblyList.Lock);
    ReassemblyList.First = ReassemblyList.Last = SentinelReassembly;
}


//* FragmentReceive - Handle a IPv6 datagram fragment.
// 
//  This is the routine called by IPv6 when it receives a fragment of an
//  IPv6 datagram, i.e. a next header value of 44.  Here we attempt to 
//  reassemble incoming fragments into complete IPv6 datagrams.
//
//  BUGBUG - There are locking & refcount issues with reassembly records.
//  We need to prevent a reassembly record from being deleted
//  after we find it. We need to prevent simultaneous changes
//  to a reassembly record.
//
uchar *
FragmentReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    FragmentHeader *Frag;
    Reassembly *Reass;
    ushort FragOffset;
    PacketShim *Shim, *ThisShim, **MoveShim;

    //
    // First, check if we have too much reassembly data buffered.
    // This will remove old reassembly buffers if necessary.
    //
    PruneReassemblyList();

    // 
    // If a jumbo payload option was seen, send an ICMP error.
    // Set ICMP pointer to the offset of the fragment header.
    //
    if (Packet->Flags & PACKET_JUMBO_OPTION) {
        //
        // The NextHeader value passed to ICMPv6SendError
        // is IP_PROTOCOL_FRAGMENT because we haven't moved
        // past the fragment header yet.
        //
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM, 
                        ICMPv6_ERRONEOUS_HEADER_FIELD, 
                        Packet->Position - Packet->IPPosition,
                        IP_PROTOCOL_FRAGMENT, FALSE);
        return NULL;  // Drop packet.
    }

    //
    // Verify that we have enough contiguous data to overlay a FragmentHeader
    // structure on the incoming packet.  Then do so.
    //
    if (Packet->ContigSize < sizeof(FragmentHeader)) {
        if (Packet->TotalSize < sizeof(FragmentHeader)) {
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
            return NULL;  // Drop packet.
        }
        if (PacketPullup(Packet, sizeof(FragmentHeader)) == 0) {
            // Pullup failed.
            return NULL;  // Drop packet.
        }
    }
    Frag = (FragmentHeader *) Packet->Data;

    //
    // Skip over fragment header.
    //
    AdjustPacketParams(Packet, sizeof *Frag);

    //
    // Lookup this fragment triple (Source Address, Destination Address, and
    // Identification field) to see if we've already received other fragments
    // of this packet.
    //
    Reass = FragmentLookup(&Packet->IP->Source, &Packet->IP->Dest, Frag->Id);
    if (Reass == NULL) {
        //
        // Handle a special case first: if this is the first, last, and only
        // fragment, then we can just continue parsing without reassembly.
        // Test both paths in checked builds.
        //
        if ((Frag->OffsetFlag == 0)
#if DBG
            && ((int)Random() < 0)
#endif
            ) {
            //
            // Return location of higher layer protocol field.
            //
            return &Frag->NextHeader;
        }

        //
        // This is the first fragment of this datagram we've received.
        // Allocate a reassembly structure to keep track of the pieces.
        //
        Reass = ExAllocatePool(NonPagedPool, sizeof(struct Reassembly));
        if (Reass == NULL) {
            KdPrint(("FragmentReceive: Couldn't allocate memory!?!\n"));
            return NULL;
        }

        RtlCopyMemory((uchar *)&(Reass->IPHdr), (uchar *)Packet->IP, 
                      sizeof(IPv6Header));
        Reass->Id = Frag->Id;
        Reass->ContigList = NULL;
#if DBG
        Reass->ContigEnd = NULL;
#endif
        Reass->GapList = NULL;
        Reass->Timer = DEFAULT_REASSEMBLY_TIMEOUT;
        Reass->Marker = 0;
        //
        // We must initialize DataLength to an invalid value.
        // Initializing to zero doesn't work.
        //
        Reass->DataLength = (uint)-1;
        Reass->UnfragmentLength = 0;
        Reass->UnfragData = NULL;
        Reass->NTEorIF = NULL;
        Reass->Flags = 0;
        Reass->Size = REASSEMBLY_SIZE_PACKET;

        //
        // Add new Reassembly struct to front of the ReassemblyList.
        //
        AddToReassemblyList(Reass);
    }
    else {
        //
        // We have found an existing reassembly structure.
        // Because we remove the reassembly structure in every
        // error situation below, an existing reassembly structure
        // must have a shim that has been successfully added to it.
        //
        ASSERT((Reass->ContigList != NULL) || (Reass->GapList != NULL));
    }

    //
    // Update the saved packet flags from this fragment packet.
    // We are really only interested in PACKET_NOT_LINK_UNICAST.
    //
    Reass->Flags |= Packet->Flags;

    FragOffset = net_short(Frag->OffsetFlag) & FRAGMENT_OFFSET_MASK;

    // 
    // Send ICMP error if this fragment causes the total packet length 
    // to exceed 65,535 bytes.  Set ICMP pointer equal to the offset to
    // the Fragment Offset field.
    //
    if (FragOffset + Packet->TotalSize > MAX_IPv6_PAYLOAD) {
        DeleteFromReassemblyList(Reass);
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (Packet->Position - sizeof(FragmentHeader) +
                         (uint)FIELD_OFFSET(FragmentHeader, OffsetFlag) -
                         Packet->IPPosition),
                        ((FragOffset == 0) ?
                         Frag->NextHeader : IP_PROTOCOL_NONE),
                        FALSE);
        return NULL;
    }


    //
    // If this is the last fragment (more fragments bit not set), then
    // remember the total data length, else, check the length is a multiple 
    // of 8 bytes.
    //
    if ((net_short(Frag->OffsetFlag) & FRAGMENT_FLAG_MASK) == 0) {
        Reass->DataLength = FragOffset + Packet->TotalSize;
    } else {
        if ((Packet->TotalSize % 8) != 0) {
            //
            // Length is not multiple of 8, send ICMP error with a pointer 
            // value equal to offset of payload length field in IP header.
            //
            DeleteFromReassemblyList(Reass);
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM, 
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            ((FragOffset == 0) ?
                             Frag->NextHeader : IP_PROTOCOL_NONE),
                            FALSE);
            return NULL; // Drop packet.
        }
    }

    //
    // We can't hold onto the incoming packet data indefinitely,
    // so copy the data contents into our own storage buffer.
    //
    Shim = ExAllocatePool(NonPagedPool, sizeof *Shim + Packet->TotalSize);
    if (Shim == NULL) {
        KdPrint(("FragmentReceive: Couldn't allocate memory!?!\n"));
        DeleteFromReassemblyList(Reass);
        return NULL;
    }

    IncreaseReassemblySize(Reass, REASSEMBLY_SIZE_FRAG + Packet->TotalSize);
    CopyPacketToBuffer(PacketShimData(Shim), Packet,
                       Packet->TotalSize, Packet->Position);
    Shim->Len = Packet->TotalSize;
    Shim->Offset = FragOffset;
    Shim->Next = NULL;

    //
    // Determine where this fragment fits among the previous ones.
    //
    // BUGBUG - This code doesn't handle overlapping fragments properly.
    //
    if (FragOffset == Reass->Marker) {
        //
        // This fragment extends the contiguous list.
        //
        if (Reass->ContigList == NULL) {
            //
            // We're first on the list.
            //
            ASSERT(FragOffset == 0);
            Reass->ContigList = Shim;

            //
            // We use info from the offset zero fragment to recreate
            // original datagram.
            //

            // Save the next header value.
            Reass->NextHeader = Frag->NextHeader;

            //
            // Grab the unfragmentable data, i.e. the extension headers that
            // preceded the fragment header.
            //
            Reass->UnfragmentLength =
                (Packet->Position - sizeof(FragmentHeader)) -
                (Packet->IPPosition + sizeof(IPv6Header));
            
            if (Reass->UnfragmentLength) {
                Reass->UnfragData = ExAllocatePool(NonPagedPool,
                                                   Reass->UnfragmentLength);
                if (Reass->UnfragData == NULL) {
                    // Out of memory!?!  Clean up and drop packet.
                    ExFreePool(Shim);
                    KdPrint(("FragmentReceive: Couldn't allocate memory?\n"));
                    DeleteFromReassemblyList(Reass);
                    return NULL;
                }
                IncreaseReassemblySize(Reass, Reass->UnfragmentLength);
                CopyPacketToBuffer(Reass->UnfragData, Packet,
                                   Reass->UnfragmentLength,
                                   Packet->IPPosition + sizeof(IPv6Header));
            }

            //
            // We need to have the IP header of the offset-zero fragment.
            // (Every fragment normally will have the same IP header,
            // except for PayloadLength, and unfragmentable headers,
            // but they might not.) ReassembleDatagram and
            // CreateFragmentPacket both need it.
            //
            // Of the 40 bytes in the header, the 32 bytes in the source
            // and destination addresses are already correct.
            // So we just copy the other 8 bytes.
            //
            RtlCopyMemory(&Reass->IPHdr, Packet->IP, 8);

            //
            // Save the NTEorIF pointer because it's needed if reassembly 
            // timer expires and we need to send an ICMP error.
            // BUGBUG - Reference counting?
            //
            Reass->NTEorIF = Packet->NTEorIF;

        } else {
            //
            // Add us to the end of the list.
            //
            Reass->ContigEnd->Next = Shim;
        }
        Reass->ContigEnd = Shim;

        //
        // Increment our contiguous extent marker.
        //
        Reass->Marker += Packet->TotalSize;

        //
        // Now peruse the non-contiguous list here to see if we already
        // have the next fragment to extend the contiguous list, and if so,
        // move it on over.  Repeat until we can't.
        //
        MoveShim = &Reass->GapList;
        while ((ThisShim = *MoveShim) != NULL) {
            if (ThisShim->Offset == Reass->Marker) {
                //
                // This fragment now extends the contiguous list.
                // Add it to the end of the list.
                //
                Reass->ContigEnd->Next = ThisShim;
                Reass->ContigEnd = ThisShim;
                Reass->Marker += ThisShim->Len;

                //
                // Remove it from non-contiguous list.
                //
                *MoveShim = ThisShim->Next;
                ThisShim->Next = NULL;

                //
                // Start over from beginning of the non-contiguous list.
                // REVIEW: Sort the GapList to avoid this?
                //
                MoveShim = &Reass->GapList;
                continue;
            }
            MoveShim = &ThisShim->Next;
        }
        
        //
        // Check to see if contiguous list now holds entire packet.
        //
        if (Reass->Marker == Reass->DataLength) {
            ReassembleDatagram(Packet, Reass);
        }
    } else {
        //
        // Exile this fragment to the non-contiguous list.
        // REVIEW: We don't keep them ordered.  Probably should.
        //
        Shim->Next = Reass->GapList;
        Reass->GapList = Shim;
    }
    return NULL;
}


//* FragmentLookup - look for record of previous fragments from this datagram.
//
//  A datagram is uniquely identified by its {source address, destination
//  address, identification field} triple.  This function checks our
//  reassembly list for previously received fragments of a given datagram.
//
//  Returns NULL if there is no existing reassembly record.
//
//  Callable from DPC context, not from thread context.
//
Reassembly *
FragmentLookup(
    IPv6Addr *Src,  // Source address to match.
    IPv6Addr *Dst,  // Destination address to match.
    ulong Id)       // Fragment identification field to match.
{
    Reassembly *Reass;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);

    for (Reass = ReassemblyList.First;; Reass = Reass->Next) {
        if (Reass == SentinelReassembly) {
            Reass = NULL;
            break;
        }
        if ((Reass->Id == Id) &&
            IP6_ADDR_EQUAL(&Reass->IPHdr.Source, Src) &&
            IP6_ADDR_EQUAL(&Reass->IPHdr.Dest, Dst)) {
            break;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
    return Reass;
}


//* AddToReassemblyList
//
//  Add the reassembly record to the list.
//  It must NOT already be on the list.
//
//  Callable from DPC context, not from thread context.
//
void
AddToReassemblyList(Reassembly *Reass)
{
    Reassembly *AfterReass = SentinelReassembly;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    Reass->Prev = AfterReass;
    (Reass->Next = AfterReass->Next)->Prev = Reass;
    AfterReass->Next = Reass;
    ReassemblyList.Size += Reass->Size;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
}


//* RemoveReassembly
//
//  Remove a reassembly record from the list.
//
//  Called with the reassembly lock held.
//
void
RemoveReassembly(Reassembly *Reass)
{
    Reass->Prev->Next = Reass->Next;
    Reass->Next->Prev = Reass->Prev;
    ReassemblyList.Size -= Reass->Size;
}


//* IncreaseReassemblySize
//
//  Increase the size of the reassembly record.
//
//  Callable from DPC context, not from thread context.
//
void
IncreaseReassemblySize(Reassembly *Reass, uint Size)
{
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    Reass->Size += Size;
    ReassemblyList.Size += Size;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
}


//* DeleteReassembly
//
//  Delete a reassembly record.
//
void
DeleteReassembly(Reassembly *Reass)
{
    PacketShim *ThisShim, *PrevShim;

    //
    // Free ContigList if populated.
    //
    PrevShim = ThisShim = Reass->ContigList;
    while (ThisShim != NULL) {
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;
        ExFreePool(PrevShim);
    }

    //
    // Free GapList if populated.
    //
    PrevShim = ThisShim = Reass->GapList;
    while (ThisShim != NULL) {
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;
        ExFreePool(PrevShim);
    }

    //
    // Free unfragmentable data.
    //
    if (Reass->UnfragData) {
        ExFreePool(Reass->UnfragData);
    }

    ExFreePool(Reass);
}


//* DeleteFromReassemblyList
//
//  Remove and delete the reassembly record.
//  The reassembly record MUST be on the list.
//
//  Callable from DPC context, not from thread context.
//
void
DeleteFromReassemblyList(Reassembly *Reass)
{
    //
    // Remove the reassembly record from the list.
    //
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    RemoveReassembly(Reass);
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);

    //
    // Delete the reassembly record.
    //
    DeleteReassembly(Reass);
}

//* PruneReassemblyList
//
//  Delete reassembly records if necessary,
//  to bring the reassembly buffering back under quota.
//
//  Callable from DPC context, not from thread context.
//
void
PruneReassemblyList(void)
{
    Reassembly *Reass;
    Reassembly *DeleteList = NULL;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    while (ReassemblyList.Size > MAX_REASSEMBLY_SIZE) {
        //
        // Suppose we are receiving fragmented packets from a single
        // source, and we are having reassembly failures because some
        // fragments are getting dropped. Then it makes sense to
        // delete the oldest unreassembled fragments. This is not the
        // right thing to do if we are receiving fragmented packets
        // from many many senders - then it would be better to delete
        // the newest unreassembled fragments, so at least some
        // packets can get reassembled.
        //
        // As a heuristic, compare source addresses of the oldest and
        // newest reassembly records. If they are the same, drop
        // the oldest. If they are different, drop the newest.
        //
        ASSERT((ReassemblyList.First != SentinelReassembly) &&
               (ReassemblyList.Last != SentinelReassembly));
        if (IP6_ADDR_EQUAL(&ReassemblyList.First->IPHdr.Source,
                           &ReassemblyList.Last->IPHdr.Source))
            Reass = ReassemblyList.Last;
        else
            Reass = ReassemblyList.First;
        RemoveReassembly(Reass);

        //
        // And put it on our temporary list.
        //
        Reass->Next = DeleteList;
        DeleteList = Reass;
    }
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);

    //
    // Delete the reassembly records.
    // We do not send ICMP errors in this situation.
    // The reassembly timer has not expired.
    // This is more analogous to a router dropping packets
    // when a queue gets full, and no ICMP error is sent
    // in that situation.
    //
    while ((Reass = DeleteList) != NULL) {
        DeleteList = Reass->Next;
        DeleteReassembly(Reass);
    }
}


typedef struct ReassembledReceiveContext {
    WORK_QUEUE_ITEM WQItem;
    NetTableEntryOrInterface *NTEorIF;
    IPv6Packet Packet;
    uchar Data[];
} ReassembledReceiveContext;

//* ReassembledReceive
//
//  Receive a reassembled packet.
//  This function is called from a kernel worker thread context.
//  It prevents "reassembly recursion".
//
void
ReassembledReceive(PVOID Context)
{
    ReassembledReceiveContext *rrc = (ReassembledReceiveContext *) Context;
    KIRQL Irql;
    int PktRefs;

    //
    // All receive processing normally happens at DPC level,
    // so we must pretend to be a DPC, so we raise IRQL.
    // (System worker threads typically run at PASSIVE_LEVEL).
    //
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    PktRefs = IPv6Receive(rrc->NTEorIF, &rrc->Packet);
    ASSERT(PktRefs == 0);
    KeLowerIrql(Irql);
    ExFreePool(rrc);
}


//* ReassembleDatagram - put all the fragments together.
//
//  Called when we have all the fragments to complete a datagram.
//  Patch them together and pass the packet up.
//
//  We allocate a single contiguous buffer and copy the fragments
//  into this buffer.
//  REVIEW: Instead use ndis buffers to chain the fragments?
//
void
ReassembleDatagram(
    IPv6Packet *Packet,      // The packet being currently received.
    Reassembly *Reass)       // Reassembly record for fragmented datagram.
{
    uint DataLen;
    uint TotalLength;
    uint memptr = sizeof(IPv6Header);
    PacketShim *ThisShim, *PrevShim;
    ReassembledReceiveContext *rrc;
    IPv6Packet *ReassPacket;
    uchar *ReassBuffer;
    uchar *pNextHeader;
    uint Offset;

    DataLen = Reass->DataLength + Reass->UnfragmentLength;
    ASSERT(DataLen <= MAX_IPv6_PAYLOAD);
    TotalLength = sizeof(IPv6Header) + DataLen;

    //
    // Allocate memory for buffer and copy fragment data into it.
    // At the same time we allocate space for context information
    // and an IPv6 packet structure.
    //
    rrc = ExAllocatePool(NonPagedPool, sizeof *rrc + TotalLength);
    if (rrc == NULL) {
        KdPrint(("ReassembleDatagram: Couldn't allocate memory!?!\n"));
        return;
    }

    ReassPacket = &rrc->Packet;
    ReassBuffer = rrc->Data;

    //
    // Generate the original IP hdr and copy it and any unfragmentable
    // data into the new packet.  Note we have to update the next header
    // field in the last unfragmentable header (or the IP hdr, if none).
    // The unfragmentable headers have been parsed previously,
    // so they should be syntactically correct.
    //

    pNextHeader = &Reass->IPHdr.NextHeader;
    Offset = 0;
    while (Offset < Reass->UnfragmentLength) {
        switch (*pNextHeader) {
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING: {
            ExtensionHeader *Hdr = (ExtensionHeader *)
                (Reass->UnfragData + Offset);
            pNextHeader = &Hdr->NextHeader;
            Offset += (Hdr->HeaderExtLength + 1) * EXT_LEN_UNIT;
            break;
        }
        case IP_PROTOCOL_FRAGMENT: {
            FragmentHeader *Hdr = (FragmentHeader *)
                (Reass->UnfragData + Offset);
            //
            // This can only happen when we took the fast path in
            // FragmentReceive and skipped past a fragment header
            // instead of reassembling and stripping it.
            //
            ASSERT(Hdr->OffsetFlag == 0);
            pNextHeader = &Hdr->NextHeader;
            Offset += sizeof *Hdr;
            break;
        }
        default:
            //
            // BUGBUG - Can a fragment header follow an AH/ESP header?
            // Or will they be "stripped" at this point?
            // (Like fragment headers.)
            //
            ASSERT(!"unfrag parse error?\n");
            Offset = Reass->UnfragmentLength;
            break;
        }
    }

    ASSERT(Offset == Reass->UnfragmentLength);
    ASSERT(*pNextHeader == IP_PROTOCOL_FRAGMENT);
    *pNextHeader = Reass->NextHeader;

    RtlCopyMemory(ReassBuffer + memptr, Reass->UnfragData,
                  Reass->UnfragmentLength);
    memptr += Reass->UnfragmentLength;

    Reass->IPHdr.PayloadLength = net_short((ushort)DataLen);
    RtlCopyMemory(ReassBuffer, (uchar *)&Reass->IPHdr, sizeof(IPv6Header));

    //
    // Run through the contiguous list, copying data over to our new packet.
    //
    PrevShim = ThisShim = Reass->ContigList;
    while(ThisShim != NULL) {
        RtlCopyMemory(ReassBuffer + memptr, PacketShimData(ThisShim),
                      ThisShim->Len);
        memptr += ThisShim->Len;
        if (memptr > TotalLength) {
            KdPrint(("ReassembleDatagram: packets don't add up\n"));
        }
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;

        ExFreePool(PrevShim);
    }

    //
    // Initialize the reassembled packet structure.
    //
    RtlZeroMemory(ReassPacket, sizeof *ReassPacket);
    ReassPacket->Data = ReassBuffer;
    ReassPacket->ContigSize = TotalLength;
    ReassPacket->TotalSize = TotalLength;
    ReassPacket->Flags = PACKET_REASSEMBLED |
        (Reass->Flags & PACKET_NOT_LINK_UNICAST);

    //
    // Explicitly null out the ContigList which was freed above and
    // clean up the reassembly struct.
    //
    Reass->ContigList = NULL;
    DeleteFromReassemblyList(Reass);

    //
    // Receive the reassembled packet.
    // If the current fragment was reassembled,
    // then we should avoid another level of recursion.
    // We must prevent "reassembly recursion".
    // Test both paths in checked builds.
    //
    if ((Packet->Flags & PACKET_REASSEMBLED)
#if DBG
        || ((int)Random() < 0)
#endif
        ) {
        ExInitializeWorkItem(&rrc->WQItem, ReassembledReceive, rrc);
        // BUGBUG - Reference counting.
        rrc->NTEorIF = Reass->NTEorIF;
        ExQueueWorkItem(&rrc->WQItem, CriticalWorkQueue);
    }
    else {
        int PktRefs = IPv6Receive(Reass->NTEorIF, ReassPacket);
        ASSERT(PktRefs == 0);
        ExFreePool(rrc);
    }
}


//* ReassemblyTimeout - Handle a reassembly timer event.
//
//  This routine is called periodically by IPv6Timeout to check for
//  timed out fragments.
//
void
ReassemblyTimeout(void)
{
    Reassembly *ThisReass, *NextReass; 
    Reassembly *Expired = NULL;

    //
    // Scan the ReassemblyList checking for expired reassembly contexts.
    //
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    for (ThisReass = ReassemblyList.First;
         ThisReass != SentinelReassembly;
         ThisReass = NextReass) {
        NextReass = ThisReass->Next;

        //
        // First decrement the timer then check if it has expired.  If so,
        // remove the reassembly record.  This is basically the same code 
        // as in DeleteFromReassemblyList().
        //
        ThisReass->Timer--;

        if (ThisReass->Timer == 0) {
            //
            // Move this reassembly context to the expired list.
            //
            RemoveReassembly(ThisReass);
            ThisReass->Next = Expired;
            Expired = ThisReass;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);

    //
    // Now that we no longer need the reassembly list lock,
    // we can send ICMP errors at our leisure.
    //

    while ((ThisReass = Expired) != NULL) {
        Expired = ThisReass->Next;
        KdPrint(("ReassemblyTimeout Id = %u\n", ThisReass->Id));

        //
        // Send ICMP error IF we have received the first fragment.
        // NB: Checking Marker != 0 is wrong, because we might have
        // received a zero-length first fragment.
        //
        if (ThisReass->ContigList != NULL) {
            IPv6Packet *Packet;

            ASSERT(ThisReass->NTEorIF != NULL);
            Packet = CreateFragmentPacket(ThisReass);
            if (Packet != NULL) {
                ICMPv6SendError(Packet,
                                ICMPv6_TIME_EXCEEDED,
                                ICMPv6_REASSEMBLY_TIME_EXCEEDED, 0,
                                Packet->IP->NextHeader, FALSE);
                ExFreePool(Packet);
            }
        }

        //
        // Delete the reassembly record.
        //
        DeleteReassembly(ThisReass);
    }
}


//* DestinationOptionsReceive - Handle IPv6 Destination options.
// 
//  This is the routine called to process a Destination Options Header,
//  a next header value of 60.
// 
uchar *
DestinationOptionsReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    IPv6OptionsHeader *DestOpt;
    uint ExtLen;
    Options Opts;

    //
    // Verify that we have enough contiguous data to overlay a Destination
    // Options Header structure on the incoming packet.  Then do so.
    //
    if (Packet->ContigSize < sizeof *DestOpt) {
        if (Packet->TotalSize < sizeof *DestOpt) {
          BadPayloadLength:
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
            return NULL;  // Drop packet.
        }
        if (PacketPullup(Packet, sizeof *DestOpt) == 0) {
            // Pullup failed.
            KdPrint(("DestinationOptionsReceive: Incoming packet too small"
                     " to contain destination options header\n"));
            return NULL;  // Drop packet.
        }
    }
    DestOpt = (IPv6OptionsHeader *) Packet->Data;

    //
    // Check that length of destination options also fit in remaining data.
    //
    ExtLen = (DestOpt->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (Packet->ContigSize < ExtLen) {
        if (Packet->TotalSize < ExtLen)
            goto BadPayloadLength;

        if (PacketPullup(Packet, ExtLen) == 0) {
            // Pullup failed.
            KdPrint(("DestinationOptionsReceive: Incoming packet too small"
                     " to contain destination options\n"));
            return NULL;  // Drop packet.
        }
        DestOpt = (IPv6OptionsHeader *) Packet->Data;
    }

    //
    // Skip over the extension header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, ExtLen);

    //
    // Process options in this extension header.  If an error occurs 
    // while processing the options, discard packet.
    //
    if (! OptionProcessing(Packet, IP_PROTOCOL_DEST_OPTS, DestOpt, ExtLen,
                           &Opts)) {
        return NULL;  // Drop packet.
    }


    //
    // Process binding update option.
    //
    if (Opts.BindingUpdate) {

        //
        // If a binding update was received and a home address option was not
        // included then we MUST drop this packet.
        //
        if (!Opts.HomeAddress) 
            return NULL;  // Drop packet.

        //
        // Process the binding update now.
        //
        if (IPv6RecvBindingUpdate(Packet, Opts.BindingUpdate,
                                  &(Opts.HomeAddress->HomeAddress))) {
            //
            // Couldn't process the binding update.  Drop the packet.
            //
            return NULL;
        }
    }


    //
    // Process the home address option.
    //
    if (Opts.HomeAddress) {
        if (IPv6RecvHomeAddress(Packet, Opts.HomeAddress)) {
            //
            // Couldn't process the home address option.  Drop the packet.
            //
            return NULL;
        }
    }

    //
    // Return location of higher layer protocol field.
    //
    return &DestOpt->NextHeader;
}


//* HopByHopOptionsReceive - Handle a IPv6 Hop-by-Hop Options.
//
//  This is the routine called to process a Hop-by-Hop Options Header,
//  next header value of 0.
//
uchar *
HopByHopOptionsReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    IPv6OptionsHeader *HopByHop;
    uint ExtLen;
    Options Opts;

    //
    // Verify that we have enough contiguous data to overlay a minimum
    // length Hop-by-Hop Options Header.  Then do so.
    //
    if (Packet->ContigSize < sizeof *HopByHop) {
        if (Packet->TotalSize < sizeof *HopByHop) {
          BadPayloadLength:
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
            return NULL;  // Drop packet.
        }
        if (PacketPullup(Packet, sizeof *HopByHop) == 0) {
            // Pullup failed.
            KdPrint(("HopByHopOptionsReceive: Incoming packet too small"
                     " to contain Hop-by-Hop Options header\n"));
            return NULL;  // Drop packet.
        }
    }
    HopByHop = (IPv6OptionsHeader *) Packet->Data;

    //
    // Check that length of the entire Hop-by-Hop options also fits in
    // remaining data.
    //
    ExtLen = (HopByHop->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (Packet->ContigSize < ExtLen) {
        if (Packet->TotalSize < ExtLen)
            goto BadPayloadLength;

        if (PacketPullup(Packet, ExtLen) == 0) {
            // Pullup failed.
            KdPrint(("HopByHopOptionsReceive: Incoming packet too small"
                     " to contain Hop-by-Hop options\n"));
            return NULL;  // Drop packet.
        }
        HopByHop = (IPv6OptionsHeader *) Packet->Data;
    }

    //
    // Skip over the extension header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, ExtLen);

    //
    // Process options in this extension header.  If an error occurs 
    // while processing the options, discard packet.
    //
    if (! OptionProcessing(Packet, IP_PROTOCOL_HOP_BY_HOP, HopByHop, ExtLen,
                           &Opts)) {
        return NULL;  // Drop packet.
    }

    //
    // If we have a valid Jumbo Payload Option, use its value as
    // the packet PayloadLength.
    //
    if (Opts.JumboLength) {
        uint PayloadLength = Opts.JumboLength;

        ASSERT(Packet->IP->PayloadLength == 0);

        //
        // Check that the jumbo length is big enough to include
        // the extension header length. This must be true because
        // the extension-header length is at most 11 bits,
        // while the jumbo length is at least 16 bits.
        //
        ASSERT(PayloadLength > ExtLen);
        PayloadLength -= ExtLen;

        //
        // Check that the amount of payload specified in the Jumbo
        // Payload value fits in the buffer handed to us.
        //
        if (PayloadLength > Packet->TotalSize) {
            //  Silently discard data.
            KdPrint(("HopByHopOptionsReceive: Jumbo payload length is greater "
                     "than the amount of data received\n"));
            return NULL;
        }

        //
        // As in IPv6HeaderReceive, adjust the TotalSize to be exactly the 
        // IP payload size (assume excess is media padding).
        //
        Packet->TotalSize = PayloadLength;
        if (Packet->ContigSize > PayloadLength)
            Packet->ContigSize = PayloadLength;

        //
        // Set the jumbo option packet flag.
        //
        Packet->Flags |= PACKET_JUMBO_OPTION;
    }
    else if (Packet->IP->PayloadLength == 0) {
        //
        // We should have a Jumbo Payload option,
        // but we didn't find it. Send an ICMP error.
        //
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        FIELD_OFFSET(IPv6Header, PayloadLength),
                        HopByHop->NextHeader, FALSE);
        return NULL;
    }

    //
    // Return location of higher layer protocol field.
    //
    return &HopByHop->NextHeader;
}


//* OptionProcessing - Routine for generic header options processsing.
//
//  Returns TRUE if the options were successfully processed.
//  Returns FALSE if the packet should be discarded.
//
int
OptionProcessing(
    IPv6Packet *Packet,     // The packet handed to us by IPv6Receive.
    uchar HdrType,          // Hop-by-hop or destination.
    IPv6OptionsHeader *Hdr, // The options header with following data.
    uint HdrLength,         // Length of the entire options area.
    Options *Opts)          // Return option values to caller.
{
    uchar *OptPtr;
    uint OptSizeLeft;
    OptionHeader *OptHdr;
    uint OptLen;

    ASSERT((HdrType == IP_PROTOCOL_DEST_OPTS) ||
           (HdrType == IP_PROTOCOL_HOP_BY_HOP));

    //
    // Zero out the Options struct that is returned.
    //
    RtlZeroMemory(Opts, sizeof *Opts);

    //
    // Skip over the extension header.
    //
    OptPtr = (uchar *)(Hdr + 1);
    OptSizeLeft = HdrLength - sizeof *Hdr;

    //
    // Note that if there are multiple options
    // of the same type, we just use the last one encountered
    // unless the spec says specifically it is an error.
    //

    while (OptSizeLeft > 0) {

        //
        // First we check the option length and ensure that it fits.
        // We move OptPtr past this option while leaving OptHdr
        // for use by the option processing code below.
        //

        OptHdr = (OptionHeader *) OptPtr;
        if (OptHdr->Type == OPT6_PAD_1) {
            //
            // This is a special pad option which is just a one byte field, 
            // i.e. it has no length or data field.
            //
            OptLen = 1;
        }
        else {
            //
            // This is a multi-byte option.
            //
            if ((sizeof *OptHdr > OptSizeLeft) ||
                ((OptLen = sizeof *OptHdr + OptHdr->DataLength) >
                 OptSizeLeft)) {
                //
                // Bad length, generate error and discard packet.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_PARAMETER_PROBLEM,
                                ICMPv6_ERRONEOUS_HEADER_FIELD,
                                (GetPacketPositionFromPointer(Packet,
                                                              &Hdr->HeaderExtLength) - 
                                 Packet->IPPosition),
                                Hdr->NextHeader, FALSE);
                return FALSE;
            }
        }
        OptPtr += OptLen;
        OptSizeLeft -= OptLen;

        switch (OptHdr->Type) {
        case OPT6_PAD_1:
        case OPT6_PAD_N:
            break;

        case OPT6_JUMBO_PAYLOAD:
            if (HdrType != IP_PROTOCOL_HOP_BY_HOP)
                goto BadOptionType;

            if (OptHdr->DataLength != sizeof Opts->JumboLength)
                goto BadOptionLength;

            if (Packet->IP->PayloadLength != 0) {
                //
                // Jumbo option encountered when IP payload is not zero.
                // Send ICMP error, set pointer to offset of this option type.
                //
                goto BadOptionType;
            }

            Opts->JumboLength = net_long(*(ulong *)(OptHdr + 1));
            if (Opts->JumboLength <= MAX_IPv6_PAYLOAD) {
                //
                // Jumbo payload length is not jumbo, send ICMP error.
                // ICMP pointer is set to offset of jumbo payload len field.
                //
                goto BadOptionData;
            }
            break;

        case OPT6_ROUTER_ALERT:
            if (HdrType != IP_PROTOCOL_HOP_BY_HOP)
                goto BadOptionType;

            if (OptLen != sizeof *Opts->Alert)
                goto BadOptionLength;

            if (Opts->Alert != NULL) {
                //
                // Can only have one router alert option.
                //
                goto BadOptionType;
            }

            //
            // Return the pointer to the router alert struct.
            //
            Opts->Alert = (IPv6RouterAlertOption *)(OptHdr + 1);
            break;

        case OPT6_HOME_ADDRESS:
            if (HdrType != IP_PROTOCOL_DEST_OPTS)
                goto BadOptionType;

            if (OptLen < sizeof *Opts->HomeAddress)
                    goto BadOptionLength;

            //
            // Return the pointer to the home address option.
            //
            Opts->HomeAddress = (IPv6HomeAddressOption UNALIGNED *)OptHdr;
            break;

        case OPT6_BINDING_UPDATE:
            if (HdrType != IP_PROTOCOL_DEST_OPTS)
                goto BadOptionType;

            //
            // At a minimum, the binding update must include all fields except
            // for the care-of address.
            //
            if (OptLen < sizeof (IPv6BindingUpdateOption) - sizeof (IPv6Addr))
                    goto BadOptionLength;

            //
            // Save pointer to the binding update option.  Note we still
            // need to do further length checking.
            //
            Opts->BindingUpdate = (IPv6BindingUpdateOption UNALIGNED *)OptHdr;

            //
            // If the care-of flag is set, then the care-of address must be
            // present also.
            //
            if (Opts->BindingUpdate->Flags & IPV6_BINDING_CARE_OF_ADDR &&
                OptLen < sizeof *Opts->BindingUpdate)
                    goto BadOptionLength;
            break;

        default:
            if (OPT6_ACTION(OptHdr->Type) == OPT6_A_SKIP) {
                //
                // Ignore the unrecognized option.
                //
                break;
            }
            else if (OPT6_ACTION(OptHdr->Type) == OPT6_A_DISCARD) {
                //
                // Discard the packet.
                //
                return FALSE;
            }
            else {
                //
                // Send an ICMP error.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_PARAMETER_PROBLEM,
                                ICMPv6_UNRECOGNIZED_OPTION,
                                (GetPacketPositionFromPointer(Packet,
                                                              &OptHdr->Type) -
                                 Packet->IPPosition),
                                Hdr->NextHeader,
                                OPT6_ACTION(OptHdr->Type) == 
                                OPT6_A_SEND_ICMP_ALL);
                return FALSE;  // discard the packet.
            }
        }
    }

    return TRUE;

BadOptionType:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  &OptHdr->Type) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.

BadOptionLength:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  &OptHdr->DataLength) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.

BadOptionData:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  (uchar *)(OptHdr + 1)) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.
}


//* ExtHdrControlReceive - generic extension header skip-over routine.
//
//  Routine for processing the extension headers in an ICMP error message
//  before delivering the error message to the upper-layer protocol.
//
uchar *
ExtHdrControlReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6ErrorReceive.
    StatusArg *StatArg)         // ICMP Error code and offset pointer.
{
    uchar *NextHdr = &StatArg->IP->NextHeader;
    uint HdrLen;

    for (;;) {
        switch (*NextHdr) {
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING: {
            ExtensionHeader *ExtHdr;  // Generic exension header.

            //
            // Here we take advantage of the fact that all of these extension
            // headers share the same first two fields (except as noted below).
            // Since those two fields (Next Header and Header Extension Length)
            // provide us with all the information we need to skip over the
            // header, they're all we need to look at here.
            //
            if (Packet->ContigSize < sizeof *ExtHdr) {
                if (PacketPullup(Packet, sizeof *ExtHdr) == 0) {
                    //
                    // Pullup failed.  There isn't enough of the invoking
                    // packet included in the error message to figure out
                    // what upper layer protocol it originated with.
                    //
                    KdPrint(("ExtHdrControlReceive: Incoming ICMP error packet"
                             " doesn't contain enough of invoking packet\n"));
                    return NULL;  // Drop packet.
                }
            }

            ExtHdr = (ExtensionHeader *) Packet->Data;
            HdrLen = (ExtHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;

            //
            // Now that we know the actual length of this extension header,
            // skip over it.
            //
            if (Packet->ContigSize < HdrLen) {
                //
                // REVIEW: We could rework this to use GetDataAtPacketOffset
                // REVIEW: here instead of PacketPullup as we don't need to
                // REVIEW: look at the data we're skipping over.  Better?
                //
                if (PacketPullup(Packet, HdrLen) == 0) {
                    //
                    // Pullup failed.  There isn't enough of the invoking
                    // packet included in the error message to figure out
                    // what upper layer protocol it originated with.
                    //
                    KdPrint(("ExtHdrControlReceive: Incoming ICMP error packet"
                             " doesn't contain enough of invoking packet\n"));
                    return NULL;  // Drop packet.
                }
            }

            NextHdr = &ExtHdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_FRAGMENT: {
            FragmentHeader *FragHdr;

            if (Packet->ContigSize < sizeof *FragHdr) {
                if (PacketPullup(Packet, sizeof *FragHdr) == 0) {
                    //
                    // Pullup failed.  There isn't enough of the invoking
                    // packet included in the error message to figure out
                    // what upper layer protocol it originated with.
                    //
                    KdPrint(("ExtHdrControlReceive: Incoming ICMP error packet"
                             " doesn't contain enough of invoking packet\n"));
                    return NULL;  // Drop packet.
                }
            }

            FragHdr = (FragmentHeader *) Packet->Data;

            if ((net_short(FragHdr->OffsetFlag) & FRAGMENT_OFFSET_MASK) != 0) {
                //
                // We can only continue parsing if this
                // fragment has offset zero.
                //
                KdPrint(("ExtHdrControlReceive: non-zero-offset fragment\n"));
                return NULL;
            }

            HdrLen = sizeof *FragHdr;
            NextHdr = &FragHdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_AH:
        case IP_PROTOCOL_ESP:
            //
            // BUGBUG - What is the correct thing here?
            //
            KdPrint(("ExtHdrControlReceive: found AH/ESP\n"));
            return NULL;

        default:
            //
            // We came to a header that we do not recognize,
            // so we can not continue parsing here.
            // But our caller might recognize this header type.
            //
            return NextHdr;
        }

        //
        // Move past this extension header.
        //
        AdjustPacketParams(Packet, HdrLen);
    }
}


//* CreateFragmentPacket
//
//  Recreates the first fragment packet for purposes of notifying a source
//  of a 'fragment reassembly time exceeded'.
//
IPv6Packet *
CreateFragmentPacket(
    Reassembly *Reass)
{
    PacketShim *FirstFrag;
    IPv6Packet *Packet;
    FragmentHeader *FragHdr;
    uint PayloadLength;
    uint PacketLength;
    uint MemLen;
    uchar *Mem;

    //
    // There must be a first (offset-zero) fragment.
    //
    FirstFrag = Reass->ContigList;
    ASSERT((FirstFrag != NULL) && (FirstFrag->Offset == 0));

    //
    // Allocate memory for creating the first fragment, i.e. the first
    // buffer in our contig list. We include space for an IPv6Packet.
    //
    PayloadLength = (Reass->UnfragmentLength + sizeof(FragmentHeader) +
                     FirstFrag->Len);
    PacketLength = sizeof(IPv6Header) + PayloadLength;
    MemLen = sizeof(IPv6Packet) + PacketLength;
    Mem = ExAllocatePool(NonPagedPool, MemLen);
    if (Mem == NULL) {
        KdPrint(("CreateFragmentPacket: Couldn't allocate memory!?!\n"));
        return NULL;
    }

    Packet = (IPv6Packet *) Mem;
    Mem += sizeof(IPv6Packet);

    Packet->Next = NULL;
    Packet->IP = (IPv6Header *) Mem;
    Packet->IPPosition = 0;
    Packet->Data = Mem;
    Packet->Position = 0;
    Packet->ContigSize = Packet->TotalSize = PacketLength;
    Packet->NdisPacket = NULL;
    Packet->AuxList = NULL;
    Packet->Flags = 0;
    Packet->SrcAddr = &Packet->IP->Source;
    Packet->SAPerformed = NULL;
    Packet->NTEorIF = Reass->NTEorIF;
    AdjustPacketParams(Packet, sizeof(IPv6Header));

    //
    // Copy the original IPv6 header into the packet.
    // Note that FragmentReceive ensures that
    // Reass->IPHdr, Reass->UnfragData, and FirstFrag
    // are all consistent.
    //
    RtlCopyMemory(Mem, (uchar *)&Reass->IPHdr, sizeof(IPv6Header));
    Mem += sizeof(IPv6Header);

    ASSERT(Reass->IPHdr.PayloadLength == net_short((ushort)PayloadLength));

    //
    // Copy the unfragmentable data into the packet.
    //
    RtlCopyMemory(Mem, Reass->UnfragData, Reass->UnfragmentLength);
    Mem += Reass->UnfragmentLength;

    //
    // Create a fragment header in the packet.
    //
    FragHdr = (FragmentHeader *) Mem;
    Mem += sizeof(FragmentHeader);

    //
    // Note that if the original offset-zero fragment had
    // a non-zero value in the Reserved field, then we will
    // not recreate it properly. It shouldn't do that.
    //
    FragHdr->NextHeader = Reass->NextHeader;
    FragHdr->Reserved = 0;
    FragHdr->OffsetFlag = net_short(FRAGMENT_FLAG_MASK);
    FragHdr->Id = Reass->Id;

    //
    // Copy the original fragment data into the packet.
    //
    RtlCopyMemory(Mem, PacketShimData(FirstFrag), FirstFrag->Len);

    return Packet;
}


//* DoNothingReceive - handler for next header 59.
//
uchar *
DoNothingReceive(
    IPv6Packet *Packet)         // Packet handed to us by link layer.
{
    UNREFERENCED_PARAMETER(Packet);

    return NULL;
}


//* RoutingReceive - Handle the IPv6 Routing Header.
//
//  Called from IPv6Receive when we encounter a Routing Header,
//  next header value of 43.
//
uchar *
RoutingReceive(
    IPv6Packet *Packet)         // Packet handed to us by link layer.
{
    IPv6RoutingHeader UNALIGNED *RH;
    uint HeaderLength;
    uint SegmentsLeft;
    uint NumAddresses, i;
    IPv6Addr *Addresses;
    uint DestScopeId;
    IP_STATUS Status;
    uchar *Mem;
    uint MemLen, Offset;
    NDIS_PACKET *FwdPacket;
    NDIS_STATUS NdisStatus;
    IPv6Header UNALIGNED *FwdIP;
    IPv6RoutingHeader UNALIGNED *FwdRH;
    IPv6Addr *FwdAddresses;
    IPv6Addr FwdDest;
    int Delta;
    uint PayloadLength;
    uint TunnelStart = NO_TUNNEL, IPSecBytes = 0;
    IPSecProc *IPSecToDo;
    RouteCacheEntry *RCE;
    uint Action;

    //
    // Verify that we have enough contiguous data,
    // then get a pointer to the routing header.
    //
    if (Packet->ContigSize < sizeof *RH) {
        if (Packet->TotalSize < sizeof *RH) {
          BadPayloadLength:
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
            return NULL;  // Drop packet.
        }
        if (PacketPullup(Packet, sizeof *RH) == 0) {
            KdPrint(("RoutingReceive: Incoming packet too small"
                     " to contain routing header\n"));
            return NULL;  // Drop packet.
        }
    }
    RH = (IPv6RoutingHeader UNALIGNED *) Packet->Data;

    //
    // Now get the entire routing header.
    //
    HeaderLength = (RH->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (Packet->ContigSize < HeaderLength) {
        if (Packet->TotalSize < HeaderLength)
            goto BadPayloadLength;

        if (PacketPullup(Packet, HeaderLength) == 0) {
            KdPrint(("RoutingReceive: Incoming packet too small"
                     " to contain routing header\n"));
            return NULL;  // Drop packet.
        }
        RH = (IPv6RoutingHeader UNALIGNED *) Packet->Data;
    }

    //
    // Move past the routing header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, HeaderLength);

    //
    // If SegmentsLeft is zero, we proceed directly to the next header.
    // We must not check the Type value or HeaderLength.
    //
    SegmentsLeft = RH->SegmentsLeft;
    if (SegmentsLeft == 0) {
        //
        // Return location of higher layer protocol field.
        //
        return &RH->NextHeader;
    }

    //
    // If we do not recognize the Type value, generate an ICMP error.
    //
    if (RH->RoutingType != 0) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->RoutingType) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return NULL;  // No further processing of this packet.
    }

    //
    // We must have an integral number of IPv6 addresses
    // in the routing header.
    //
    if (RH->HeaderExtLength & 1) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->HeaderExtLength) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return NULL;  // No further processing of this packet.
    }

    NumAddresses = RH->HeaderExtLength / 2;

    //
    // Sanity check SegmentsLeft.
    //
    if (SegmentsLeft > NumAddresses) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->SegmentsLeft) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return NULL; // No further processing of this packet.
    }

    i = NumAddresses - SegmentsLeft;
    Addresses = (IPv6Addr *) (RH + 1);

    //
    // Sanity check the addresses involved.
    // The spec doesn't mention checking for an unspecified address,
    // but I think it's a good idea.
    // REVIEW: What about the loopback address?
    // REVIEW: Should we also drop link-local and site-local addresses,
    // REVIEW: since there's no way to determine their scope id?
    //
    if (IsMulticast(&Packet->IP->Dest) ||
        IsMulticast(&Addresses[i]) ||
        IsUnspecified(&Addresses[i])) {
        //
        // Just drop the packet, no ICMP error in this case.
        //
        return NULL; // No further processing of this packet.
    }

    //
    // Verify IPSec was performed.
    //
    if (InboundSPLookup(Packet, 0, 0, 0, Packet->NTEorIF->IF) != TRUE) {
        //
        // No SP was found or the SP found indicated to drop the packet.
        //
        KdPrint(("RoutingReceive: "
                 "IPSec lookup failed or policy was to drop\n"));
        return NULL;  // Drop packet.
    }

    //
    // Find a route to the new destination.
    //
    // REVIEW: This really is fudging for scoped addresses, as there is no
    // REVIEW: way to know that the receiving interface is really in scope.
    // REVIEW: We need to bring this up in the working group, I think we
    // REVIEW: should drop the packet unless the address is global.
    //
    DestScopeId = DetermineScopeId(&Addresses[i], Packet->NTEorIF->IF);
    Status = RouteToDestination(&Addresses[i], DestScopeId, NULL,
                                RTD_FLAG_NORMAL, &RCE);
    if (Status != IP_SUCCESS) {
        KdPrint(("RoutingReceive: "
                 "No route to destination for forwarding.\n"));
        ICMPv6SendError(Packet,
                        ICMPv6_DESTINATION_UNREACHABLE,
                        ICMPv6_NO_ROUTE_TO_DESTINATION,
                        0, RH->NextHeader, FALSE);
        return NULL;
    }

    //
    // Find the Security Policy for this outbound traffic.
    // The source address is the same but the destination address is the
    // next hop from the routing header.
    //
    IPSecToDo = OutboundSPLookup(&Packet->IP->Source, &Addresses[i],
                                 0, 0, 0, RCE->NCE->IF, &Action);

    if (IPSecToDo == NULL) {
        //
        // Check Action.
        //
        if (Action == LOOKUP_DROP) {
            // Drop packet.
            ReleaseRCE(RCE);
            return NULL;
        } else {
            if (Action == LOOKUP_IKE_NEG) {
                KdPrint(("RoutingReceive: IKE not supported yet.\n"));
                ReleaseRCE(RCE);
                return NULL;
            }
        }

        //
        // With no IPSec to perform, IPv6Forward won't be changing the
        // outgoing interface from what we currently think it will be.
        // So we can use the exact size of its link-level header.
        //
        Offset = RCE->NCE->IF->LinkHeaderSize;

    } else {
        //
        // Calculate the space needed for the IPSec headers.
        //
        IPSecBytes = IPSecBytesToInsert(IPSecToDo, &TunnelStart);

        if (TunnelStart != 0) {
            KdPrint(("RoutingReceive: IPSec Tunnel mode only.\n"));
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
            ReleaseRCE(RCE);
            return NULL;
        }

        //
        // The IPSec code in IPv6Forward might change the outgoing
        // interface from what we currently think it will be.  Play it
        // safe and leave the max amount of space for its link-level header.
        //
        Offset = MAX_LINK_HEADER_SIZE;
    }

    //
    // The packet has passed all our checks.
    // We can construct a revised packet for transmission.
    // First we allocate a packet, buffer, and memory.
    //
    // NB: The original packet is read-only for us. Furthermore
    // we can not keep a pointer to it beyond the return of this
    // function. So we must copy the packet and then modify it.
    //

    // Packet->IP->PayloadLength might be zero with jumbograms.
    Delta = Packet->Position - Packet->IPPosition;
    PayloadLength = Packet->TotalSize + Delta - sizeof(IPv6Header);
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength + IPSecBytes;

    NdisStatus = IPv6AllocatePacket(MemLen, &FwdPacket, &Mem);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        if (IPSecToDo) {
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
        }
        ReleaseRCE(RCE);
        return NULL; // No further processing of this packet.
    }

    FwdIP = (IPv6Header UNALIGNED *)(Mem + Offset + IPSecBytes);
    FwdRH = (IPv6RoutingHeader UNALIGNED *)
        ((uchar *)FwdIP + Delta - HeaderLength);
    FwdAddresses = (IPv6Addr *) (FwdRH + 1);

    //
    // Now we copy from the original packet to the new packet.
    //
    CopyPacketToBuffer((uchar *)FwdIP, Packet,
                       sizeof(IPv6Header) + PayloadLength,
                       Packet->IPPosition);

    //
    // Fix up the new packet - put in the new destination address
    // and decrement SegmentsLeft.
    // NB: We pass the Reserved field through unmodified!
    // This violates a strict reading of the spec,
    // but Steve Deering has confirmed that this is his intent.
    //
    FwdDest = FwdAddresses[i];
    FwdAddresses[i] = FwdIP->Dest;
    FwdIP->Dest = FwdDest;
    FwdRH->SegmentsLeft--;

    //
    // Forward the packet. This decrements the Hop Limit and generates
    // any applicable ICMP errors (Time Limit Exceeded, Destination
    // Unreachable, Packet Too Big). Note that previous ICMP errors
    // that we generated were based on the unmodified incoming packet,
    // while from here on the ICMP errors are based on the new FwdPacket.
    //
    IPv6Forward(Packet->NTEorIF, FwdPacket, Offset + IPSecBytes, FwdIP,
                PayloadLength, FALSE, // Don't Redirect.
                IPSecToDo, RCE);

    if (IPSecToDo) {
        FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
    }

    ReleaseRCE(RCE);
    return NULL;  // No further processing of this packet.
}
