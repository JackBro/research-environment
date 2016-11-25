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
// Transmit routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "icmp.h"
#include "neighbor.h"
#include "fragment.h"
#include "security.h"  
#include "ipsec.h"
#include "md5.h"

#define IPSEC_ENABLE

//
// Structure of completion data for "Care Of" packets.
//
typedef struct CareOfCompletionInfo {
    void (*SavedCompletionHandler)(PNDIS_PACKET);  // Original handler.
    void *SavedCompletionData;                     // Original data.
    PNDIS_BUFFER SavedFirstBuffer;
    uint NumESPTrailers;
} CareOfCompletionInfo;


ulong FragmentId = 0;

//* NewFragmentId - generate a unique fragment identifier.
//
//  Returns a fragment id.
//
__inline
ulong
NewFragmentId(void)
{
    return InterlockedIncrement(&FragmentId);
}


//* IPv6AllocatePacket
//
//  Allocates a single-buffer packet.
//
//  The completion handler for the packet is set to IPv6FreePacket,
//  although the caller can easily change that if desired.
//
NDIS_STATUS
IPv6AllocatePacket(
    uint Length,
    PNDIS_PACKET *pPacket,
    void **pMemory)
{
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    void *Memory;
    NDIS_STATUS Status;

    NdisAllocatePacket(&Status, &Packet, IPv6PacketPool);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("IPv6AllocatePacket - couldn't allocate header!?!\n"));
        return Status;
    }

    Memory = ExAllocatePool(NonPagedPool, Length);
    if (Memory == NULL) {
        KdPrint(("IPv6AllocatePacket - couldn't allocate pool!?!\n"));
        NdisFreePacket(Packet);
        return NDIS_STATUS_RESOURCES;
    }

    NdisAllocateBuffer(&Status, &Buffer, IPv6BufferPool,
                       Memory, Length);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("IPv6AllocatePacket - couldn't allocate buffer!?!\n"));
        ExFreePool(Memory);
        NdisFreePacket(Packet);
        return Status;
    }

    NdisChainBufferAtFront(Packet, Buffer);
    PC(Packet)->CompletionHandler = IPv6FreePacket;
    *pPacket = Packet;
    *pMemory = Memory;
    return NDIS_STATUS_SUCCESS;
}


//* IPv6FreePacket - free an IPv6 packet.
//
//  Frees a packet whose buffers were allocated from the IPv6BufferPool.
//
void
IPv6FreePacket(PNDIS_PACKET Packet)
{
    PNDIS_BUFFER Buffer, NextBuffer;

    //
    // Free all the buffers in the packet.
    // Start with the first buffer in the packet and follow the chain.
    //
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        VOID *Mem;
        UINT Unused;

        //
        // Free the buffer descriptor back to IPv6BufferPool and its
        // associated memory back to the heap.  Not clear if it would be
        // safe to free the memory before the buffer (because the buffer
        // references the memory), but this order should definitely be safe.
        //
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Mem, &Unused);
        NdisFreeBuffer(Buffer);
        ExFreePool(Mem);
    }

    //
    // Free the packet back to IPv6PacketPool.
    //
    NdisFreePacket(Packet);
}


//* IPv6CareOfComplete - Completion handler for "Care Of" packets.
//
//  Completion handler for packets that had a routing header inserted
//  because of a Binding Cache Entry.
//
void  //  Returns: Nothing.
IPv6CareOfComplete(PNDIS_PACKET Packet)
{
    PNDIS_BUFFER Buffer;
    uchar *Memory;
    uint Length;

    CareOfCompletionInfo *CareOfInfo = 
        (CareOfCompletionInfo *)PC(Packet)->CompletionData;

    ASSERT(CareOfInfo->SavedFirstBuffer != NULL);
    
    //
    // Remove the first buffer that IPv6Send1 created, re-chain
    // the original first buffer, and restore the original packet
    // completion info.
    //
    NdisUnchainBufferAtFront(Packet, &Buffer);
    NdisChainBufferAtFront(Packet, CareOfInfo->SavedFirstBuffer);
    PC(Packet)->CompletionHandler = CareOfInfo->SavedCompletionHandler;
    PC(Packet)->CompletionData = CareOfInfo->SavedCompletionData;

    //
    // Now free the removed buffer and its memory.
    //
    NdisQueryBuffer(Buffer, &Memory, &Length);
    NdisFreeBuffer(Buffer);
    ExFreePool(Memory);

    //
    // Check if there is any ESP trailers that need to be freed.
    //
    for ( ; CareOfInfo->NumESPTrailers > 0; CareOfInfo->NumESPTrailers--) {
        // Remove the ESP Trailer.
        NdisUnchainBufferAtBack(Packet, &Buffer);
        //
        // Free the removed buffer and its memory.
        //
        NdisQueryBuffer(Buffer, &Memory, &Length);
        NdisFreeBuffer(Buffer);
        ExFreePool(Memory);
    }

    //
    // Free care-of completion data.
    //
    ExFreePool(CareOfInfo);

    //
    // The packet should now have it's original completion handler
    // specified for us to call.
    //
    ASSERT(PC(Packet)->CompletionHandler != NULL);

    //
    // Call the packet's designated completion handler.
    //
    (*PC(Packet)->CompletionHandler)(Packet);
}


//* IPv6SendComplete - IP send complete handler.
//
//  Called by the link layer when a send completes.  We're given a pointer to
//  a net structure, as well as the completing send packet and the final status
//  of the send.
//
void                      //  Returns: Nothing.
IPv6SendComplete(
    void *Context,        // Context we gave to the link layer on registration.
    PNDIS_PACKET Packet,  // Packet completing send.
    NDIS_STATUS Status)   // Final status of send.
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Status);

    //
    // The packet should have a completion handler specified for us to call.
    //
    ASSERT(PC(Packet)->CompletionHandler != NULL);

    //
    // Call the packet's designated completion handler.
    //
    (*PC(Packet)->CompletionHandler)(Packet);
}


//* IPv6Send0 - Send a fully-formed IPv6 packet.
//
//  Lowest-level send routine.  We already know the first-hop destination and
//  have a completed packet ready to send.  All we really do here
//  is check & update the NCE's neighbor discovery state.
//
//  Discovery Address is the source address to use in neighbor 
//  discovery solicitations.
//
//  If DiscoveryAddress is not NULL, it must NOT be the address
//  of the packet's source address, because that memory might
//  be gone might by the time we reference it in NeighborSolicitSend.
//  It must point to memory that will remain valid across
//  IPv6Send0's entire execution.
//
//  If DiscoveryAddress is NULL, then the Packet must be well-formed.
//  It must have a valid IPv6 header. For example, the raw-send
//  path can NOT pass in NULL.
//
//  REVIEW - Should IPV6Send0 live in send.c or neighbor.c?
//
//  Callable from thread or DPC context.
//
void
IPv6Send0(
    PNDIS_PACKET Packet,        // Packet to send.
    uint Offset,                // Offset from start of Packet to IP header.
    NeighborCacheEntry *NCE,    // First-hop neighbor information.
    IPv6Addr *DiscoveryAddress) // Address to use for neighbor discovery.
{
    KIRQL OldIrql;      // For locking the interface.
    Interface *IF;      // Interface to send via.
    LARGE_INTEGER Now;  // Time since boot in system interval timer ticks.

    ASSERT(NCE != NULL);
    IF = NCE->IF;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    //
    // If the interface is disabled, we can't send packets.
    //
    if (IsDisabledIF(IF)) {
      AbortRequest:
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
        return;
    }

    //
    // Check the Neighbor Discovery Protocol state of our Neighbor to
    // insure that we have current information to work with.  We don't
    // have a timer going off to drive this in the common case, but
    // instead check the reachability timestamp directly here.
    //
    switch (NCE->NDState) {
    case ND_STATE_PERMANENT:
        //
        // This neighbor is always valid.
        //
        break;

    case ND_STATE_REACHABLE:
        //
        // Common case.  We've verified neighbor reachability within
        // the last 'ReachableTime' ticks of the system interval timer.
        // If the time limit hasn't expired, we're free to go.
        //
        // Note that the following arithmetic will correctly handle wraps
        // of the IPv6 tick counter.
        //
        if ((uint)(IPv6TickCount - NCE->LastReachable) <= IF->ReachableTime) {
            //
            // Got here within the time limit.  Just send it.
            //
            break;
        }

        //
        // Too long since last send.  Entry went stale.  Conceptually,
        // we've been in the STALE state since the above quantity went
        // positive.  So just drop on into it now...
        // 

    case ND_STATE_STALE:
        //
        // We have a stale entry in our neighbor cache.  Go into DELAY
        // state, start the delay timer, and send the packet anyway.
        //
        NCE->NDState = ND_STATE_DELAY;
        NCE->EventTimer = DELAY_FIRST_PROBE_TIME;
        break;

    case ND_STATE_DELAY:
    case ND_STATE_PROBE:
        //
        // While in the DELAY and PROBE states, we continue to send to our
        // cached address and hope for the best.  Sending more packets in
        // either state has no other effect.
        //
        break;

    case ND_STATE_INCOMPLETE: {
        PNDIS_PACKET OldPacket;
        int SendSolicit;
        IPv6Addr DiscoveryAddressBuffer;

        //
        // Get DiscoveryAddress from the packet
        // if we don't already have it.
        // We SHOULD use the packet's source address if possible.
        //
        if (DiscoveryAddress == NULL) {
            IPv6Header UNALIGNED *IP;
            IPv6Header HdrBuffer;
            NetTableEntry *NTE;

            IP = GetIPv6Header(Packet, Offset, &HdrBuffer);
            ASSERT(IP != NULL);
            DiscoveryAddressBuffer = IP->Source;

            DiscoveryAddress = &DiscoveryAddressBuffer;

            //
            // Check that the address is a valid unicast address
            // assigned to the outgoing interface.
            //
            NTE = (NetTableEntry *) *FindADE(IF, DiscoveryAddress);
            if ((NTE == NULL) ||
                (NTE->Type != ADE_UNICAST) ||
                !IsValidNTE(NTE)) {
                //
                // Can't use the packet's source address.
                // Try the interface's link-local address.
                //
                NTE = IF->LinkLocalNTE;
                if ((NTE == NULL) || !IsValidNTE(NTE)) {
                    //
                    // Without a valid link-local address, give up.
                    //
                    goto AbortRequest;
                }

                DiscoveryAddressBuffer = NTE->Address;
            }
        }

        //
        // We do not have a valid link-level address for the neighbor.
        // We must queue the packet, pending neighbor discovery.
        // Remember the packet's offset in the Packet6Context area.
        // REVIEW: For now, wait queue is just one packet deep.
        //
        OldPacket = NCE->WaitQueue;
        PC(Packet)->pc_offset = Offset;
        PC(Packet)->DiscoveryAddress = *DiscoveryAddress;
        NCE->WaitQueue = Packet;

        //
        // If we have not started neighbor discovery yet,
        // do so now by sending the first solicit.
        //
        if (SendSolicit = (NCE->EventCount == 0)) {

            NCE->EventCount = 1;
            NCE->EventTimer = IF->RetransTimer;
        }

        KeReleaseSpinLock(&IF->Lock, OldIrql);

        if (SendSolicit) {
            NeighborSolicitSend(NCE, DiscoveryAddress);
        }

        if (OldPacket != NULL) {
            //
            // This queue overflow is congestion of a sort,
            // so we must not send an ICMPv6 error.
            //
            IPv6SendComplete(NULL, OldPacket, NDIS_STATUS_REQUEST_ABORTED);
        }
        return;
    }

    default:
        //
        // Should never happen.
        //
        ASSERTMSG("IPv6Send0: Invalid Neighbor Cache NDState field!\n", FALSE);
    }

    //
    // Move the NCE to the head of the LRU list,
    // because we are using it to send a packet.
    //
    if (NCE != IF->FirstNCE) {
        //
        // Remove NCE from the list.
        //
        NCE->Next->Prev = NCE->Prev;
        NCE->Prev->Next = NCE->Next;

        //
        // Add NCE to the head of the list.
        //
        NCE->Next = IF->FirstNCE;
        NCE->Next->Prev = NCE;
        NCE->Prev = SentinelNCE(IF);
        NCE->Prev->Next = NCE;
    }

    //
    // Unlock before transmitting the packet.
    // This means that there is a very small chance that NCE->LinkAddress
    // could change out from underneath us. (For example, if we process
    // an advertisement changing the link-level address.)
    // In practice this won't happen, and if it does the worst that
    // will happen is that we'll send a packet somewhere strange.
    // The alternatives are copying the LinkAddress or holding the lock.
    //
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // BUGBUG - What if the interface is disabled now?
    // Need separate close/release methods on IF->LinkContext.
    //
    (*IF->Transmit)(IF->LinkContext, Packet, Offset, NCE->LinkAddress);
}


//* IPv6SendFragments - Fragment an IPv6 datagram.
//
//  Helper routine for creating and sending IPv6 fragments.
//  Called from IPv6Send1 when the datagram is bigger than the path MTU.
//
//  The PathMTU is passed separately so that we use a consistent value.
//  The value in the RCE is subject to change.
//
//  REVIEW: The algorithm used here generates all the fragments at once.
//  It might be better to generate the fragments one at a time,
//  waiting for each send to complete before moving on.
//  This would mean breaking up the work and having a special
//  send-completion routine.
//
void
IPv6SendFragments(
    PNDIS_PACKET Packet,   // Packet to send.
    uint Offset,           // Offset from start of Packet to IP header.
    IPv6Header *IP,        // Pointer to Packet's IPv6 header.
    uint PayloadLength,    // Packet payload length.
    RouteCacheEntry *RCE,  // First-hop neighbor information.
    uint PathMTU)          // PathMTU to use when fragmenting.
{
    NeighborCacheEntry *NCE = RCE->NCE;
    NDIS_STATUS Status;
    PNDIS_PACKET FragPacket;
    FragmentHeader FragHdr;
    uchar *Mem;
    uint MemLen;
    uint PktOffset;
    uint UnfragBytes;
    uint BytesLeft;
    uint BytesSent;
    uchar HdrType;
    uchar *tbuf;
    PNDIS_BUFFER SrcBuffer;
    uint SrcOffset;
    uint NextHeaderOffset;
    uint FragPayloadLength;

    //
    // Determine the 'unfragmentable' portion of this packet.
    // We do this by scanning through all extension headers,
    // and noting the last occurrence, if any, of
    // a routing or hop-by-hop header.
    // We do not assume the extension headers are in recommended order,
    // but otherwise we assume that the headers are well-formed.
    // BUGBUG: We assume that they are contiguous.
    //
    UnfragBytes = sizeof *IP;
    HdrType = IP->NextHeader;
    NextHeaderOffset = (uchar *)&IP->NextHeader - (uchar *)IP;
    tbuf = (uchar *)(IP + 1);
    while ((HdrType == IP_PROTOCOL_HOP_BY_HOP) ||
           (HdrType == IP_PROTOCOL_ROUTING) ||
           (HdrType == IP_PROTOCOL_DEST_OPTS)) {
        ExtensionHeader *EHdr = (ExtensionHeader *) tbuf;
        uint EHdrLen = (EHdr->HeaderExtLength + 1) * 8;

        tbuf += EHdrLen;
        if (HdrType != IP_PROTOCOL_DEST_OPTS) {
            UnfragBytes = tbuf - (uchar *)IP;
            NextHeaderOffset = (uchar *)&EHdr->NextHeader - (uchar *)IP;
        }
        HdrType = EHdr->NextHeader;
    }

    //
    // Check that we can actually fragment this packet.
    // If the unfragmentable part is too large, we can't.
    // We need to send at least 8 bytes of fragmentable data
    // in each fragment.
    //
    if (UnfragBytes + sizeof(FragmentHeader) + 8 > PathMTU) {
        KdPrint(("IPv6SendFragments: can't fragment\n"));
        Status = NDIS_STATUS_REQUEST_ABORTED;
        goto ErrorExit;
    }

    FragHdr.NextHeader = HdrType;
    FragHdr.Reserved = 0;
    FragHdr.Id = net_long(NewFragmentId());

    //
    // Initialize SrcBuffer and SrcOffset, which point
    // to the fragmentable data in the packet.
    // SrcOffset is the offset into SrcBuffer's data,
    // NOT an offset into the packet.
    //
    NdisQueryPacket(Packet, NULL, NULL, &SrcBuffer, NULL);
    SrcOffset = Offset + UnfragBytes;

    //
    // Create new packets of MTU size until all data is sent.
    //
    BytesLeft = sizeof *IP + PayloadLength - UnfragBytes;
    PktOffset = 0; // relative to fragmentable part of original packet

    while (BytesLeft != 0) {
        //
        // Determine new IP payload length (a multiple of 8) and 
        // and set the Fragment Header offset.
        //
        if ((BytesLeft + UnfragBytes + sizeof(FragmentHeader)) > PathMTU) {
            BytesSent = (PathMTU - UnfragBytes - sizeof(FragmentHeader)) &~ 7;
            // Not the last fragment, so turn on the M bit.
            FragHdr.OffsetFlag = net_short((ushort)(PktOffset | 1));
        } else {
            BytesSent = BytesLeft;
            FragHdr.OffsetFlag = net_short((ushort)PktOffset);
        }

        //
        // Allocate packet (and a buffer) and Memory for new fragment
        //
        MemLen = Offset + UnfragBytes + sizeof(FragmentHeader) + BytesSent;
        Status = IPv6AllocatePacket(MemLen, &FragPacket, &Mem);
        if (Status != NDIS_STATUS_SUCCESS) {
            goto ErrorExit;
        }

        //
        // Copy IP header, Frag Header, and a portion of data to fragment.
        //
        RtlCopyMemory(Mem + Offset, IP, UnfragBytes);
        RtlCopyMemory(Mem + Offset + UnfragBytes, &FragHdr,
                      sizeof(FragmentHeader));
        CopyNdisToFlat(Mem + Offset + UnfragBytes + sizeof(FragmentHeader),
                       SrcBuffer, SrcOffset, BytesSent,
                       &SrcBuffer, &SrcOffset);

        //
        // Correct the PayloadLength and NextHeader fields.
        //
        FragPayloadLength = UnfragBytes + sizeof(FragmentHeader) +
                                BytesSent - sizeof(IPv6Header);
        ASSERT(FragPayloadLength <= MAX_IPv6_PAYLOAD);
        ((IPv6Header *)(Mem + Offset))->PayloadLength =
            net_short((ushort) FragPayloadLength);
        ASSERT(Mem[Offset + NextHeaderOffset] == HdrType);
        Mem[Offset + NextHeaderOffset] = IP_PROTOCOL_FRAGMENT;

        BytesLeft -= BytesSent;
        PktOffset += BytesSent;

        //
        // Send the fragment.
        //
        IPv6Send0(FragPacket, Offset, NCE, NULL);
    }

    //
    // BUGBUG: Really we should be monitoring send-completion
    // status information for each of the fragments.
    //
    Status = NDIS_STATUS_SUCCESS;
  ErrorExit:
    IPv6SendComplete(NULL, Packet, Status);
}


//* IPv6Send2 - Send an IPv6 datagram.
//
//  Slightly higher-level send routine.  We have a completed datagram and a
//  RCE indicating where to direct it to.  Here we deal with any packetization
//  issues (inserting a Jumbo Payload option, fragmentation, etc.) that are
//  necessary, and pick a NCE for the first hop.
//
void
IPv6Send2(
    PNDIS_PACKET Packet,       // Packet to send.
    uint Offset,               // Offset from start of Packet to IP header.
    IPv6Header *IP,            // Pointer to Packet's IPv6 header.
    uint PayloadLength,        // Packet payload length.
    RouteCacheEntry *RCE,      // First-hop neighbor information.
    ushort TransportProtocol,
    ushort SourcePort,
    ushort DestPort)
{
    uint PacketLength;        // Size of complete IP packet in bytes.
    NeighborCacheEntry *NCE;  // First-hop neighbor information.
    uint PathMTU;
    PNDIS_BUFFER OrigBuffer1, NewBuffer1;
    uchar *OrigMemory, *NewMemory,
        *EndOrigMemory, *EndNewMemory, *InsertPoint;
    uint OrigBufSize, NewBufSize, TotalPacketSize, Size, RtHdrSize;
    IPv6RoutingHeader *SavedRtHdr = NULL, *RtHdr = NULL;
    IPv6Header *IPNew;
    uint BytesToInsert = 0;
    uchar *BufPtr, *PrevNextHdr;
    ExtensionHeader *EHdr;
    uint EHdrLen;
    uchar HdrType;
    NDIS_STATUS Status;
    RouteCacheEntry *CareOfRCE = NULL;
    CareOfCompletionInfo *CareOfInfo;
    KIRQL OldIrql;
    IPSecProc *IPSecToDo;
    uint Action;
    uint i;
    uint TunnelStart = NO_TUNNEL;
    uint JUST_ESP;
    uint IPSEC_TUNNEL = FALSE;
    uint NumESPTrailers = 0;

    //
    // Find the Security Policy for this outbound traffic.
    //
    IPSecToDo = OutboundSPLookup(&IP->Source, &IP->Dest, TransportProtocol,
                                 SourcePort, DestPort, RCE->NTE->IF, &Action);
    if (IPSecToDo == NULL) {
        //
        // Check Action.
        // Bypass is handled by checking for IPSecToDo.
        //
        if (Action == LOOKUP_DROP) {
            // Drop packet.
            goto ContinueSend2;
        }
        if (Action == LOOKUP_IKE_NEG) {
            KdPrint(("IPv6Send2: IKE not supported yet.\n"));
            goto ContinueSend2;
        }
    }

    if (IPSecToDo) {
        //
        // Calculate the space needed for the IPSec headers.
        //
        BytesToInsert = IPSecBytesToInsert(IPSecToDo, &TunnelStart);

        if (TunnelStart != NO_TUNNEL) {
            IPSEC_TUNNEL = TRUE;
        }
    }

    //
    // Save.
    //
    HdrType = IP->NextHeader;
    PrevNextHdr = &IP->NextHeader;
    BufPtr = (uchar *)(IP + 1);

    //
    // If this packet is being sent to a mobile care-of address, then it must
    // contain a routing extension header.  If one already exists then add the
    // destination address as the last entry.  If no routing header exists
    // insert one with the home address as the first (and only) address.
    //
    // This code assumes that the packet is contiguous at least up to the
    // insertion point.
    //
    if (RCE->CareOfRCE != NULL) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RCE->CareOfRCE != NULL) {
            CareOfRCE = RCE->CareOfRCE;
            AddRefRCE(CareOfRCE);
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    // 
    // Skip H-H and Dest. extension headers.
    //
    while ((HdrType == IP_PROTOCOL_HOP_BY_HOP) ||
           (HdrType == IP_PROTOCOL_DEST_OPTS)) {
        //
        // Skip the hop-by-hop and destination options extension headers
        // if they exist.
        //
        EHdr = (ExtensionHeader UNALIGNED *) BufPtr;
        EHdrLen = (EHdr->HeaderExtLength + 1) * 8;
        BufPtr += EHdrLen;
        HdrType = EHdr->NextHeader;
        PrevNextHdr = &EHdr->NextHeader;
    }

    //
    // Check if there is a routing header.
    //
    if (HdrType == IP_PROTOCOL_ROUTING) {
        EHdr = (ExtensionHeader UNALIGNED *) BufPtr;
        EHdrLen = (EHdr->HeaderExtLength + 1) * 8;

        RtHdrSize = EHdrLen;

        PrevNextHdr = &EHdr->NextHeader;

        //
        // Check if this header will be modified due to mobility.
        //
        if (CareOfRCE) {
            CareOfRCE = ValidateRCE(CareOfRCE);

            // Save Routing Header location for later use.
            RtHdr = (IPv6RoutingHeader *)BufPtr;

            //
            // Check if there is room to store the Home Address.
            // REVIEW: Is this necessary, what should happen
            // REVIEW: if the routing header is full?
            //
            if (RtHdr->HeaderExtLength / 2 < 23) {
                BytesToInsert += sizeof (IPv6Addr);
            }
        } else {
            // Adjust BufPtr to end of routing header.
            BufPtr += EHdrLen;
        }
    } else {
        //
        // No routing header present, but check if one needs to be
        // inserted due to mobility.
        //
        if (CareOfRCE) {
            CareOfRCE = ValidateRCE(CareOfRCE);
            BytesToInsert += (sizeof (IPv6RoutingHeader) + sizeof (IPv6Addr));
        }
    }

    // Only will happen for IPSec bypass mode with no mobility.
    if (BytesToInsert == 0) {
        //
        // Nothing to do.
        //
        Action = LOOKUP_CONT;
        goto ContinueSend2;
    }

    //
    // We have something to insert.  We will replace the packet's
    // first NDIS_BUFFER with a new buffer that we allocate to hold the
    // all data from the existing first buffer plus the inserted data.
    //

    //
    // We get the first buffer and determine its size, then
    // allocate memory for the new buffer.
    //
    NdisGetFirstBufferFromPacket(Packet, &OrigBuffer1, &OrigMemory,
                                 &OrigBufSize, &TotalPacketSize);
    TotalPacketSize -= Offset;
    NewBufSize = (OrigBufSize - Offset) + MAX_LINK_HEADER_SIZE + BytesToInsert;
    Offset = MAX_LINK_HEADER_SIZE;
    NewMemory = ExAllocatePool(NonPagedPool, NewBufSize);
    if (NewMemory == NULL) {
        KdPrint(("IPv6Send2: - couldn't allocate pool!?!\n"));
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    NdisAllocateBuffer(&Status, &NewBuffer1, IPv6BufferPool, NewMemory,
                       NewBufSize);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("IPv6Send2 - couldn't allocate buffer!?!\n"));
        ExFreePool(NewMemory);
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    //
    // We've sucessfully allocated a new buffer.  Now copy the data from
    // the existing buffer to the new one.  First we copy all data after
    // the insertion point.  This is essentially the transport layer data
    // (no Extension headers).
    //

    //
    // Calculate Insertion Point for upper layer data.
    //
    EndOrigMemory = OrigMemory + OrigBufSize;
    EndNewMemory = NewMemory + NewBufSize;
    Size = EndOrigMemory - BufPtr;
    InsertPoint = EndNewMemory - Size;

    // Copy upper layer data to end of new buffer.
    RtlCopyMemory(InsertPoint, BufPtr, Size);

    BytesToInsert = 0;

    //
    // Insert Transport IPSec headers.
    //
    if (IPSecToDo) {
        Action = IPSecInsertHeaders(TRANSPORT, IPSecToDo, &InsertPoint,
                                    NewMemory, Packet, &TotalPacketSize,
                                    PrevNextHdr, TunnelStart, &BytesToInsert,
                                    &NumESPTrailers, &JUST_ESP);
        if (Action == LOOKUP_DROP) {
            NdisFreeBuffer(NewBuffer1);
            ExFreePool(NewMemory);
            goto ContinueSend2;
        }
    } // end of if (IPSecToDo).

    //
    // Check if mobility needs to be done.
    //
    if (CareOfRCE) {
        // Check if routing header is already present in original buffer..
        if (RtHdr != NULL) {
            //
            // Need to insert the home address in the routing header.
            //
            RtHdrSize += sizeof (IPv6Addr);
            // Move insert point up to start of routing header.
            InsertPoint -= RtHdrSize;

            BytesToInsert += sizeof(IPv6Addr);

            // Insert the routing header.
            RtlCopyMemory(InsertPoint, RtHdr, RtHdrSize);

            // Insert the Home address.
            RtlCopyMemory(InsertPoint + RtHdrSize - sizeof (IPv6Addr),
                          &IP->Dest, sizeof (IPv6Addr));

            RtHdr = (IPv6RoutingHeader *)InsertPoint;

            // Adjust size of routing header.
            RtHdr->HeaderExtLength += 2;

        } else {
            //
            // No routing header present - need to create new Routing header.
            //
            RtHdrSize = sizeof (IPv6RoutingHeader) + sizeof(IPv6Addr);

            // Move insert point up to start of routing header.
            InsertPoint -= RtHdrSize;

            BytesToInsert += RtHdrSize;

            //
            // Insert an entire routing header.
            //
            RtHdr = (IPv6RoutingHeader UNALIGNED *)InsertPoint;
            RtHdr->NextHeader = *PrevNextHdr;
            RtHdr->HeaderExtLength = 2;
            RtHdr->RoutingType = 0;
            RtHdr->Reserved = 0;
            RtHdr->SegmentsLeft = 1;
            // Insert the home address.
            RtlCopyMemory(RtHdr+1 , &IP->Dest, sizeof (IPv6Addr));

            //
            // Fix the previous NextHeader field to indicate that it now points
            // to a routing header.
            //
            *(PrevNextHdr) = IP_PROTOCOL_ROUTING;
        }

        // Change the destination IPv6 address to the care-of address.
        RtlCopyMemory(&IP->Dest, &RCE->CareOfRCE->Destination,
                      sizeof (IPv6Addr));
    } // end of if(CareOfRCE)

    //
    // Copy original IP plus any extension headers.
    // If a care-of address was added, the Routing header is not part
    // of this copy because it has already been copied.
    //
    Size = BufPtr - (uchar *)IP;
    // Move insert point up to start of IP.
    InsertPoint -= Size;

    // Adjust length of payload.
    PayloadLength += BytesToInsert;

    // Set the new IP payload length.
    IP->PayloadLength = net_short((ushort)PayloadLength);

    RtlCopyMemory(InsertPoint, (uchar *)IP, Size);

    IPNew = (IPv6Header *)InsertPoint;

    //
    // Check if any Transport mode IPSec was performed and
    // if mutable fields need to be adjusted.
    //
    if (TunnelStart != 0 && IPSecToDo && !JUST_ESP) {
        if (RtHdr) {
            //
            // Save the new routing header so it can be restored after
            // authenticating.
            //
            SavedRtHdr = ExAllocatePool(NonPagedPool, RtHdrSize);
            if (SavedRtHdr == NULL) {
                KdPrint(("IPv6Send2: - couldn't allocate SavedRtHdr!?!\n"));
                NdisFreeBuffer(NewBuffer1);
                ExFreePool(NewMemory);
                Action = LOOKUP_DROP;
                goto ContinueSend2;
            }
            
            RtlCopyMemory(SavedRtHdr, RtHdr, RtHdrSize);
        }

        //
        // Adjust mutable fields before doing Authentication.
        //
        Action = IPSecAdjustMutableFields(InsertPoint, SavedRtHdr);

        if (Action == LOOKUP_DROP) {
            NdisFreeBuffer(NewBuffer1);
            ExFreePool(NewMemory);
            goto ContinueSend2;
        }
    } // end of if(IPSecToDo && !JUST_ESP)

    //
    // We need to save the existing completion handler & data.  We'll
    // use these fields here, and restore them in IPv6CareOfComplete.
    //
    CareOfInfo = ExAllocatePool(NonPagedPool, sizeof(*CareOfInfo));
    if (CareOfInfo == NULL) {
        KdPrint(("IPv6Send2 - couldn't allocate completion info!?!\n"));
        NdisFreeBuffer(NewBuffer1);
        ExFreePool(NewMemory);
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    CareOfInfo->SavedCompletionHandler = PC(Packet)->CompletionHandler;
    CareOfInfo->SavedCompletionData = PC(Packet)->CompletionData;
    CareOfInfo->SavedFirstBuffer = OrigBuffer1;
    CareOfInfo->NumESPTrailers = NumESPTrailers;
    PC(Packet)->CompletionHandler = IPv6CareOfComplete;
    PC(Packet)->CompletionData = CareOfInfo;

    // Unchain the original first buffer from the packet.
    NdisUnchainBufferAtFront(Packet, &OrigBuffer1);
    // Chain the new buffer to the front of the packet.
    NdisChainBufferAtFront(Packet, NewBuffer1);

    //
    // Do authentication for transport mode IPSec.
    //
    if (IPSecToDo) {         
        IPSecAuthenticatePacket(TRANSPORT, IPSecToDo, InsertPoint,
                                &TunnelStart, NewMemory, EndNewMemory,
                                NewBuffer1);
        
        if (!JUST_ESP) {
            //
            // Reset the mutable fields to correct values.
            // Just copy from old packet to new packet for IP and
            // unmodified Ext. headers.
            //
            RtlCopyMemory(InsertPoint, (uchar *)IP, Size);

            // Check if the Routing header needs to be restored.
            if (CareOfRCE) {
                // Copy the saved routing header to the new buffer.
                RtlCopyMemory(RtHdr, SavedRtHdr, RtHdrSize);
            }
        }
    } // end of if (IPSecToDo)

    //
    // We're done with the transport copy.  Use the care-of RCE for 
    // sending the packet.  This may change if destination is a tunnel that 
    // differs from the transport destination.
    //
    if (CareOfRCE) {    
        // Release old RCE.
        ReleaseRCE(RCE);
        // Set new RCE.
        RCE = CareOfRCE;
        // Set CareOfRCE to NULL to indicate the RCE change.
        CareOfRCE = NULL;
    }

    //
    // Insert tunnel IPSec headers.
    //
    if (IPSEC_TUNNEL) {
        uint i = 0;
        RouteCacheEntry *TunnelRCE = NULL;

        // Loop through the different Tunnels.
        while (TunnelStart < IPSecToDo->BundleSize) {
            uchar NextHeader = IP_PROTOCOL_V6;

            NumESPTrailers = 0;

            i++;

            // Reset byte count.
            BytesToInsert = 0;

            Action = IPSecInsertHeaders(TUNNEL, IPSecToDo, &InsertPoint,
                                        NewMemory, Packet, &TotalPacketSize,
                                        &NextHeader, TunnelStart,
                                        &BytesToInsert, &NumESPTrailers,
                                        &JUST_ESP);
            if (Action == LOOKUP_DROP) {
                goto ContinueSend2;
            }

            // Add the ESP trailer header number.
            CareOfInfo->NumESPTrailers += NumESPTrailers;

            // Move insert point up to start of IP.
            InsertPoint -= sizeof(IPv6Header);

            //
            // Adjust length of payload.
            //
            PayloadLength = BytesToInsert + PayloadLength + sizeof(IPv6Header);

            // Insert IP header fields.
            IPNew = (IPv6Header *)InsertPoint;

            IPNew->PayloadLength = net_short((ushort)PayloadLength);
            IPNew->NextHeader = NextHeader;

            if (!JUST_ESP) {
                // Adjust mutable fields.
                IPNew->VersClassFlow = IP_VERSION;
                IPNew->HopLimit = 0;
            } else {
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            // Source address same as inner header.
            RtlCopyMemory(&IPNew->Source, &IP->Source, sizeof (IPv6Addr));
            // Dest address to the tunnel end point.
            RtlCopyMemory(&IPNew->Dest, &IPSecToDo[TunnelStart].SA->SADestAddr,
                          sizeof (IPv6Addr));

            //
            // Check if a new RCE is needed.
            //
            if (!(IP6_ADDR_EQUAL(&IPNew->Dest, &IP->Dest))) {
                // Get a new route to the tunnel end point.
                Status = RouteToDestination(&IPNew->Dest, 0, NULL,
                                            RTD_FLAG_NORMAL, &TunnelRCE);
                if (Status != IP_SUCCESS) {
                    KdPrint(("IPv6Send2: No route to IPSec tunnel dest."));
                    IPv6SendAbort(CastFromNTE(RCE->NTE), Packet, Offset,
                                  ICMPv6_DESTINATION_UNREACHABLE,
                                  ICMPv6_NO_ROUTE_TO_DESTINATION, 0, FALSE);
                    Action = LOOKUP_DROP;
                    goto ContinueSend2;
                }

                // Free old RCE.
                ReleaseRCE(RCE);
                // Set new RCE;
                RCE = TunnelRCE;
            }

            //
            // Do authentication for tunnel mode IPSec.
            //
            IPSecAuthenticatePacket(TUNNEL, IPSecToDo, InsertPoint,
                                    &TunnelStart, NewMemory, EndNewMemory,
                                    NewBuffer1);

            if (!JUST_ESP) {
                //
                // Reset the mutable fields to correct values.
                //
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }
        } // end of while (TunnelStart < IPSecToDo->BundleSize)
    } // end of if (IPSEC_TUNNEL)
    
    // Set the IP pointer to the new IP pointer.
    IP = IPNew;

    if (IPSecToDo) {
        // Free IPSecToDo.
        FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);

        if (SavedRtHdr) {
            // Free the saved routing header.
            ExFreePool(SavedRtHdr);
        }
    }

ContinueSend2:

    if (Action == LOOKUP_DROP) {
        // Error occured.        
        KdPrint(("IPv6Send2: Drop packet.\n"));
        IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
        if (CareOfRCE) {
            ReleaseRCE(CareOfRCE);
        }
        
        if (IPSecToDo) {
            // Free IPSecToDo.
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);

            if (SavedRtHdr) {
                // Free the saved routing header.
                ExFreePool(SavedRtHdr);
            }
        }
        return;
    }

    //
    // We only have one NCE per RCE for now,
    // so picking one is really easy...
    //
    NCE = RCE->NCE;

#if 0
    //
    // This check is disabled because we should allow the packet
    // to be looped-back if it is sent to a multicast address
    // that we are receiving from.
    //
    // Only allow a zero hop-limit if the packet is being looped-back.
    //
    if (IP->HopLimit == 0) {
        if (NCE != LoopNCE) {
            KdPrint(("IPv6Send2: attempted to send with zero hop-limit\n"));
            IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
            return;
        }
    }
#endif

    //
    // See if we need to insert a Jumbo Payload option.
    //
    if (PayloadLength > MAX_IPv6_PAYLOAD) {
        // BUGBUG: Add code to insert a Jumbo Payload hop-by-hop option here.
        KdPrint(("IPv6Send2: attempted to send a Jumbo Payload!\n"));
        IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);        
        return;
    }

    //
    // Check the path's MTU.  If we're larger, fragment.
    //
    PacketLength = PayloadLength + sizeof(IPv6Header);
    PathMTU = GetPathMTUFromRCE(RCE);
    if (PacketLength > PathMTU) {

        //
        // BUGBUG - What about CareOfRCE and TunnelRCE in this path?
        //

        IPv6SendFragments(Packet, Offset, IP, PayloadLength, RCE, PathMTU);

    } else {
        //
        // Fill in packet's PayloadLength field.
        // We already set the IP->PayloadLength if IPSec was done.
        //
        if (!IPSecToDo) {
            IP->PayloadLength = net_short((ushort)PayloadLength);
        }

        IPv6Send0(Packet, Offset, NCE, NULL);
    }
    
}

#ifndef IPSEC_ENABLE
//* IPv6Send1 - Send an IPv6 datagram.
//
//  Slightly higher-level send routine.  We have a completed datagram and a
//  RCE indicating where to direct it to.  Here we deal with any packetization
//  issues (inserting a Jumbo Payload option, fragmentation, etc.) that are
//  necessary, and pick a NCE for the first hop.
//
//  With IPsec, we use IPv6Send2 instead of IPv6Send1.
//
void
IPv6Send1(
    PNDIS_PACKET Packet,   // Packet to send.
    uint Offset,           // Offset from start of Packet to IP header.
    IPv6Header *IP,        // Pointer to Packet's IPv6 header.
    uint PayloadLength,    // Packet payload length.
    RouteCacheEntry *RCE)  // First-hop neighbor information.
{
    uint PacketLength;        // Size of complete IP packet in bytes.
    NeighborCacheEntry *NCE;  // First-hop neighbor information.
    uint PathMTU;
    PNDIS_BUFFER OrigBuffer1, NewBuffer1;
    uchar *OrigMemory, *NewMemory;
    uint OrigBufSize, NewBufSize, TotalPacketSize;
    IPv6RoutingHeader *RtHdr;
    ushort BytesToInsert = 0;
    uchar *BufPtr, *PrevNextHdr;
    ExtensionHeader *EHdr;
    uint EHdrLen;
    uchar HdrType;
    NDIS_STATUS Status;
    int VADelta;
    RouteCacheEntry *CareOfRCE = NULL;
    CareOfCompletionInfo *CareOfInfo;
    KIRQL OldIrql;

    //
    // If this packet is being sent to a mobile care-of address, then it must
    // contain a routing extension header.  If one already exists then add the
    // destination address as the last entry.  If no routing header exists
    // insert one with the home address as the first (and only) address.
    //
    // This code assumes that the packet is contiguous at least up to the
    // insertion point.
    //
    if (RCE->CareOfRCE != NULL) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RCE->CareOfRCE != NULL) {
            CareOfRCE = RCE->CareOfRCE;
            AddRefRCE(CareOfRCE);
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    if (CareOfRCE) {
        CareOfRCE = ValidateRCE(CareOfRCE);
        HdrType = IP->NextHeader;
        PrevNextHdr = &IP->NextHeader;
        BufPtr = (uchar *)(IP + 1);

        while ((HdrType == IP_PROTOCOL_HOP_BY_HOP) ||
               (HdrType == IP_PROTOCOL_DEST_OPTS)) {
            //
            // Skip the hop-by-hop and destination options extension headers
            // if they exist.
            //
            EHdr = (ExtensionHeader UNALIGNED *) BufPtr;
            EHdrLen = (EHdr->HeaderExtLength + 1) * 8;
            BufPtr += EHdrLen;
            HdrType = EHdr->NextHeader;
            PrevNextHdr = &EHdr->NextHeader;
        }

        if (HdrType == IP_PROTOCOL_ROUTING) {
            RtHdr = (IPv6RoutingHeader *) EHdr;     // save for later use.
            if (EHdrLen / 2 < 23)
                BytesToInsert = sizeof (IPv6Addr);
        } else
            BytesToInsert = sizeof (IPv6RoutingHeader) + sizeof (IPv6Addr);
    }

    if (BytesToInsert == 0) {
        //
        // Nothing to do.
        //
        goto ContinueSend1;
    }

    //
    // We have something to insert.  We will replace the packet's
    // first NDIS_BUFFER with a new buffer that we allocate to hold the
    // all data from the existing first buffer plus the inserted data.
    //

    //
    // We get the first buffer and determine its' size, then
    // allocate memory for the new buffer.
    //
    NdisGetFirstBufferFromPacket(Packet, &OrigBuffer1, &OrigMemory,
                                 &OrigBufSize, &TotalPacketSize);
    NewBufSize = (OrigBufSize - Offset) + MAX_LINK_HEADER_SIZE + BytesToInsert;
    NewMemory = ExAllocatePool(NonPagedPool, NewBufSize);
    if (NewMemory == NULL) {
        KdPrint(("IPv6Send1: - couldn't allocate pool!?!\n"));
        goto ContinueSend1;
    }

    NdisAllocateBuffer(&Status, &NewBuffer1, IPv6BufferPool, NewMemory,
                       NewBufSize);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("IPv6Send1 - couldn't allocate buffer!?!\n"));
        ExFreePool(NewMemory);
        goto ContinueSend1;
    }

    //
    // We need to save the existing completion handler & data.  We'll
    // use these fields here, and restore them in IPv6CareOfComplete.
    //
    CareOfInfo = ExAllocatePool(NonPagedPool, sizeof(*CareOfInfo));
    if (CareOfInfo == NULL) {
        KdPrint(("IPv6Send1 - couldn't allocate completion info!?!\n"));
        NdisFreeBuffer(NewBuffer1);
        ExFreePool(NewMemory);
        goto ContinueSend1;
    }

    CareOfInfo->SavedCompletionHandler = PC(Packet)->CompletionHandler;
    CareOfInfo->SavedCompletionData = PC(Packet)->CompletionData;
    CareOfInfo->SavedFirstBuffer = OrigBuffer1;
    PC(Packet)->CompletionHandler = IPv6CareOfComplete;
    PC(Packet)->CompletionData = CareOfInfo;

    //
    // We've sucessfully allocated a new buffer.  Now copy the data from
    // the existing buffer to the new one.  First we copy all data up to
    // the insertion point, then we insert the new data, and finally we
    // copy any remaining data from the existing buffer to the new one.
    //
    // VADelta contains the distance in memory between the first bytes of
    // the original and new data blocks.  Add this value to original buffer
    // pointers to point to the same position in the new buffer.
    //
    // At this point BufPtr points to the insertion point in the existing
    // buffer.
    //
    VADelta = NewMemory - OrigMemory;
    RtlCopyMemory(NewMemory, OrigMemory, BufPtr-OrigMemory);

    // Move the IP pointer to the corresponding position in the new buffer,
    // and adjust the packet's size.
    IP = (IPv6Header *) ((char *)IP + VADelta);
    PayloadLength += BytesToInsert;

    if (BytesToInsert == sizeof (IPv6Addr)) {
        //
        // Here we're inserting just the home address into an existing
        // routing header, and adjusting the header's size.
        //
        RtlCopyMemory(BufPtr + VADelta, &IP->Dest, sizeof (IPv6Addr));
        EHdr = (ExtensionHeader *) ((char *)EHdr + VADelta);
        EHdr->HeaderExtLength += 2;
    } else {
        //
        // Insert an entire routing header.
        //
        RtHdr = (IPv6RoutingHeader UNALIGNED *) (BufPtr + VADelta);
        RtHdr->NextHeader = *PrevNextHdr;
        RtHdr->HeaderExtLength = 2;
        RtHdr->RoutingType = 0;
        RtHdr->SegmentsLeft = 1;
        RtHdr->Reserved = 0;
        RtlCopyMemory(RtHdr+1 , &IP->Dest, sizeof (IPv6Addr));

        //
        // Fix the previous NextHeader field to indicate that it now points
        // to a routing header.
        //
        *(PrevNextHdr + VADelta) = IP_PROTOCOL_ROUTING;
    }

    //
    // Change the destination IPv6 address, then copy the remaing data
    // to the new buffer.
    //
    RtlCopyMemory(&IP->Dest, &RCE->Destination, sizeof (IPv6Addr));
    RtlCopyMemory(BufPtr + VADelta + BytesToInsert, BufPtr,
                  OrigMemory + OrigBufSize - BufPtr );
    NdisUnchainBufferAtFront(Packet, &OrigBuffer1);
    NdisChainBufferAtFront(Packet, NewBuffer1);

    //
    // We're done the copy.  Use the care-of RCE below for sending the
    // packet.
    //
    RCE = CareOfRCE;

ContinueSend1:

    //
    // We only have one NCE per RCE for now,
    // so picking one is really easy...
    //
    NCE = RCE->NCE;

#if 0
    //
    // This check is disabled because we should allow the packet
    // to be looped-back if it is sent to a multicast address
    // that we are receiving from.
    //
    // Only allow a zero hop-limit if the packet is being looped-back.
    //
    if (IP->HopLimit == 0) {
        if (NCE != LoopNCE) {
            KdPrint(("IPv6Send1: attempted to send with zero hop-limit\n"));
            IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
            return;
        }
    }
#endif

    //
    // See if we need to insert a Jumbo Payload option.
    //
    if (PayloadLength > MAX_IPv6_PAYLOAD) {
        // BUGBUG: Add code to insert a Jumbo Payload hop-by-hop option here.
        KdPrint(("IPv6Send1: attempted to send a Jumbo Payload!\n"));
        IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
        return;
    }

    //
    // Check the path's MTU.  If we're larger, fragment.
    //
    PacketLength = PayloadLength + sizeof(IPv6Header);
    PathMTU = GetPathMTUFromRCE(RCE);
    if (PacketLength > PathMTU) {
        IPv6SendFragments(Packet, Offset, IP, PayloadLength, RCE, PathMTU);
    } else {
        //
        // Fill in packet's PayloadLength field.
        //
        IP->PayloadLength = net_short((ushort)PayloadLength);

        IPv6Send0(Packet, Offset, NCE, NULL);

        if (CareOfRCE)
            ReleaseRCE(CareOfRCE);
    }
}
#endif // IPSEC_ENABLE


//* IPv6Forward - Forward a packet onto a new link.
//
//  Somewhat like IPv6Send1, but for forwarding packets
//  instead of sending freshly-generated packets.
//
//  We are given ownership of the packet. The packet data
//  must be writable and the IP header must be contiguous.
//
//  We can generate several possible ICMP errors:
//  Time Limit Exceeded, Destination Unreachable, Packet Too Big.
//  We decrement the hop limit.
//  We do not fragment the packet.
//
//  We assume that our caller has already sanity-checked
//  the packet's destination address. Routing-header forwarding
//  may allow some cases (like link-local or loopback destinations)
//  that normal router forwarding does not permit.
//  Our caller provides the NCE of the next hop for the packet.
//
void
IPv6Forward(
    NetTableEntryOrInterface *RecvNTEorIF,
    PNDIS_PACKET Packet,
    uint Offset,
    IPv6Header *IP,
    uint PayloadLength,
    int Redirect,
    IPSecProc *IPSecToDo,
    RouteCacheEntry *RCE)
{
    uchar ICMPType, ICMPCode;
    uint ErrorParameter;    
    uint PacketLength;
    uint LinkMTU, IPSecBytesInserted = 0;
    IP_STATUS Status;
    KIRQL OldIrql;
    uint IPSecOffset = Offset;    
    NeighborCacheEntry *NCE = RCE->NCE;

    ASSERT(IP == GetIPv6Header(Packet, Offset, NULL));

    //
    // Check for "scope" errors.  We can't allow a packet with a scoped
    // source address to leave its scope.
    //
    if ((IsLinkLocal(&IP->Source) && (NCE->IF != RecvNTEorIF->IF)) ||
        (IsSiteLocal(&IP->Source) &&
         (NCE->IF->Site != RecvNTEorIF->IF->Site))) {
        IPv6SendAbort(RecvNTEorIF, Packet, Offset,
                      ICMPv6_DESTINATION_UNREACHABLE, ICMPv6_SCOPE_MISMATCH,
                      0, FALSE);
        return;
    }

    //
    // We can only send a Redirect if our caller permits it
    // (for example Redirect is not allowed when source-routing)
    // and if the interface groks Neighbor Discovery.
    //
    if (Redirect && (RecvNTEorIF->IF->Flags & IF_FLAG_DISCOVERS)) {
        //
        // We SHOULD send a Redirect, whenever
        // 1. The Source address of the packet specifies a neighbor, and
        // 2. A better first-hop resides on the same link, and
        // 3. The Destination address is not multicast.
        // See Section 8.2 of the ND spec.
        //
        if ((NCE->IF == RecvNTEorIF->IF) && !IsMulticast(&IP->Dest)) {
            uint SrcScopeId;
            RouteCacheEntry *SrcRCE;
            NeighborCacheEntry *SrcNCE;

            //
            // Get an RCE for the Source of this packet.
            //
            SrcScopeId = DetermineScopeId(&IP->Source, RecvNTEorIF->IF);
            Status = RouteToDestination(&IP->Source, SrcScopeId, RecvNTEorIF,
                                        RTD_FLAG_STRICT, &SrcRCE);
            if (Status == IP_SUCCESS) {

                //
                // Because of RTD_FLAG_STRICT.
                //
                ASSERT(SrcRCE->NTE->IF == RecvNTEorIF->IF);

                SrcNCE = SrcRCE->NCE;
                if (IP6_ADDR_EQUAL(&SrcNCE->NeighborAddress, &IP->Source)) {
                    //
                    // The source of this packet is on-link,
                    // so send a Redirect to the source.
                    // Unless rate-limiting prevents it.
                    //
                    if (ICMPv6RateLimit(SrcRCE)) {
                        KdPrint(("RedirectSend - rate limit %s\n",
                                 FormatV6Address(&SrcRCE->Destination)));
                    } else {
                        RedirectSend(SrcNCE, NCE, &IP->Dest, RecvNTEorIF,
                                     Packet, Offset, PayloadLength);
                    }
                }
                ReleaseRCE(SrcRCE);
            }
        }
    }

    //
    // Check that the hop limit allows the packet to be forwarded.
    //
    if (IP->HopLimit <= 1) {
        //
        // It seems to be customary in this case to have the hop limit
        // in the ICMP error's payload be zero.
        //
        IP->HopLimit = 0;

        IPv6SendAbort(RecvNTEorIF, Packet, Offset, ICMPv6_TIME_EXCEEDED,
                      ICMPv6_HOP_LIMIT_EXCEEDED, 0, FALSE);
        return;
    }

    //
    // Note that subsequent ICMP errors (Packet Too Big, Address Unreachable)
    // will show the decremented hop limit. They are also generated
    // from the perspective of the outgoing link. That is, the source address
    // in the ICMP error is an address assigned to the outgoing link.
    //
    IP->HopLimit--;

    // Check if there is IPSec to be done.
    if (IPSecToDo) {
        PNDIS_BUFFER Buffer;
        uchar *Memory, *EndMemory, *InsertPoint;
        uint BufSize, TotalPacketSize, BytesInserted;
        IPv6Header *IPNew;
        uint JUST_ESP, Action, TunnelStart = 0, i = 0;
        NetTableEntry *NTE;
        uint NumESPTrailers = 0; // not used here.     
        RouteCacheEntry *TunnelRCE = NULL;

        // Set the insert point to the start of the IP header.
        InsertPoint = (uchar *)IP;
        // Get the first buffer.
        NdisGetFirstBufferFromPacket(Packet, &Buffer, &Memory, &BufSize,
                                     &TotalPacketSize);
        TotalPacketSize -= Offset;

        // End of this buffer.
        EndMemory = Memory + BufSize;

        // Loop through the different Tunnels.
        while (TunnelStart < IPSecToDo->BundleSize) {
            uchar NextHeader = IP_PROTOCOL_V6;
            BytesInserted = 0;

            i++;

            //
            // Insert Tunnel mode IPSec.
            //
            Action = IPSecInsertHeaders(TUNNEL, IPSecToDo, &InsertPoint,
                                        Memory, Packet, &TotalPacketSize,
                                        &NextHeader, TunnelStart,
                                        &BytesInserted, &NumESPTrailers,
                                        &JUST_ESP);
            if (Action == LOOKUP_DROP) {
                KdPrint(("IPv6Forward: IPSec drop packet.\n"));
                return;
            }

            // Move insert point up to start of IP.
            InsertPoint -= sizeof(IPv6Header);

            // Reset the Offset value to the correct link-layer size.
            IPSecOffset = InsertPoint - Memory;

            // Adjust length of payload.
            PayloadLength = BytesInserted + PayloadLength + sizeof(IPv6Header);

            // Insert IP header fields.
            IPNew = (IPv6Header *)InsertPoint;

            IPNew->PayloadLength = net_short((ushort)PayloadLength);
            IPNew->NextHeader = NextHeader;

            if (!JUST_ESP) {
                // Adjust mutable fields.
                IPNew->VersClassFlow = IP_VERSION;
                IPNew->HopLimit = 0;
            } else {
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            // Dest address to the tunnel end point.
            RtlCopyMemory(&IPNew->Dest, &IPSecToDo[TunnelStart].SA->SADestAddr,
                          sizeof (IPv6Addr));

            // Figure out what source address to use.
            NTE = FindBestSourceAddress(NCE->IF, &IPNew->Dest);
            if (NTE == NULL) {
                //
                // We have no valid source address to use!
                //
                return;
            }

            // Source address is the address of the forwarding interface.
            RtlCopyMemory(&IPNew->Source, &NTE->Address, sizeof (IPv6Addr));

            // Release NTE.
            ReleaseNTE(NTE);

            //
            // Check if a new RCE is needed.
            //
            if (!(IP6_ADDR_EQUAL(&IPNew->Dest, &IP->Dest))) {
                // Get a new route to the tunnel end point.
                Status = RouteToDestination(&IPNew->Dest, 0, NULL,
                                            RTD_FLAG_NORMAL, &TunnelRCE);
                if (Status != IP_SUCCESS) {
                    KdPrint(("IPv6Forward: No route to IPSec tunnel dest."));
                    IPv6SendAbort(RecvNTEorIF, Packet, Offset,
                                  ICMPv6_DESTINATION_UNREACHABLE,
                                  ICMPv6_NO_ROUTE_TO_DESTINATION, 0, FALSE);
                    return;
                }

                // Release the old RCE.
                ReleaseRCE(RCE);
                // Set the new RCE.
                RCE = TunnelRCE;
                // Set new NCE;
                NCE = RCE->NCE;
            }

            //
            // Do authentication for tunnel mode IPSec.
            //
            IPSecAuthenticatePacket(TUNNEL, IPSecToDo, InsertPoint,
                                    &TunnelStart, Memory, EndMemory, Buffer);

            if (!JUST_ESP) {
                //
                // Reset the mutable fields to correct values.
                //
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            IPSecBytesInserted += (BytesInserted + sizeof(IPv6Header));
        } // end of while (TunnelStart < IPSecToDo->BundleSize)
    } // end of if (IPSecToDo)

    //
    // Check that the packet is not too big for the outgoing link.
    // Note that IF->LinkMTU is volatile, so we capture
    // it in a local variable for consistency.
    //
    PacketLength = PayloadLength + sizeof(IPv6Header);
    LinkMTU = NCE->IF->LinkMTU;
    if (PacketLength > LinkMTU) {
        // Change the LinkMTU to account for the IPSec headers.
        LinkMTU -= IPSecBytesInserted;

        //
        // Note that MulticastOverride is TRUE for Packet Too Big errors.
        // This allows Path MTU Discovery to work for multicast.
        //
        IPv6SendAbort(RecvNTEorIF, Packet, Offset, ICMPv6_PACKET_TOO_BIG,
                      0, LinkMTU, TRUE); // MulticastOverride.
        return;
    }

    IPv6Send0(Packet, IPSecOffset, NCE, NULL);
}


//* IPv6SendAbort
//
//  Abort an attempt to send a packet and instead
//  generate an ICMP error.
//
//  Disposes of the aborted packet.
//
//  The caller can specify the source address of the ICMP error,
//  by specifying an NTE, or the caller can provide an interface
//  from which which the best source address is selected.
//
//  Callable from thread or DPC context.
//  Must be called with no locks held.
//
void
IPv6SendAbort(
    NetTableEntryOrInterface *NTEorIF,
    PNDIS_PACKET Packet,        // Aborted packet.
    uint Offset,                // Offset of IPv6 header in aborted packet.
    uchar ICMPType,             // ICMP error type.
    uchar ICMPCode,             // ICMP error code pertaining to type.
    ulong ErrorParameter,       // Parameter included in the error.
    int MulticastOverride)      // Allow replies to multicast packets?
{
    IPv6Header UNALIGNED *IP;
    IPv6Packet DummyPacket;
    IPv6Header HdrBuffer;

    //
    // It's possible for GetIPv6Header to fail
    // when we are sending "raw" packets.
    //
    IP = GetIPv6Header(Packet, Offset, &HdrBuffer);
    if (IP != NULL) {
        InitializePacketFromNdis(&DummyPacket, Packet, Offset);
        DummyPacket.IP = IP;
        DummyPacket.SrcAddr = &IP->Source;
        DummyPacket.IPPosition = Offset;
        AdjustPacketParams(&DummyPacket, sizeof *IP);
        DummyPacket.NTEorIF = NTEorIF;

        ICMPv6SendError(&DummyPacket, ICMPType, ICMPCode, ErrorParameter,
                        IP->NextHeader, MulticastOverride);
    }

    IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);
}
