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
// User Datagram Protocol code.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "udp.h"
#include "info.h"
#include "route.h"
#include "security.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

#define IPSEC_ENABLE

//
// REVIEW: Shouldn't this be in an include file somewhere?
//
#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'uPCT')

#endif // POOL_TAGGING


extern KSPIN_LOCK AddrObjTableLock;


//* UDPSend - Send a user datagram.
//
//  The real send datagram routine.  We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send queue.
//
void                     // Returns: Nothing.
UDPSend(
    AddrObj *SrcAO,      // Address Object of endpoint doing the send.
    DGSendReq *SendReq)  // Datagram send request describing the send.
{
    KIRQL Irql0;
    RouteCacheEntry *RCE;
    NetTableEntryOrInterface *NTEorIF;
    NetTableEntry *NTE;
    Interface *IF;
    IPv6Header *IP;
    UDPHeader *UDP;
    uint PayloadLength;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER UDPBuffer;
    void *Memory;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    TDI_STATUS ErrorValue;
    uint Offset;
    uint HeaderLength;
    uint PacketSendFlags;
    uint ChecksumLength = 0;

    CHECK_STRUCT(SrcAO, ao);
    ASSERT(SrcAO->ao_usecnt != 0);

    //
    // Loop while we have something to send, and can get
    // the resources to send it.
    //
    for (;;) {

        CHECK_STRUCT(SendReq, dsr);

        //
        // Determine NTE to send on (if user cares).
        // We do this prior to allocating packet header buffers so
        // we know how much room to leave for the link-level header.
        //
        // REVIEW: We may need to add a DHCP case later that checks for
        // REVIEW: the AO_DHCP_FLAG and allows src addr to be unspecified.
        //
        if (IsUnspecified(&SrcAO->ao_addr)) {
            if (IsMulticast(&SendReq->dsr_addr) && (SrcAO->ao_mcast_if != 0)) {

                // 
                // This is a multicast packet and the user application has
                // specified an interface number to use for this socket.
                //
                IF = FindInterfaceFromIndex(SrcAO->ao_mcast_if);
                if (IF == NULL) {
                    KdPrint(("UDPSend: Bad mcast interface number\n"));
                    ErrorValue = TDI_INVALID_REQUEST;
                  ReturnError:
                    //
                    // If possible, complete the request with an error.
                    // Free the request structure.
                    //
                    if (SendReq->dsr_rtn != NULL)
                        (*SendReq->dsr_rtn)(SendReq->dsr_context,
                                            ErrorValue, 0);
                    KeAcquireSpinLock(&DGSendReqLock, &Irql0);
                    FreeDGSendReq(SendReq);
                    KeReleaseSpinLock(&DGSendReqLock, Irql0);
                    goto SendComplete;
                }
                NTEorIF = CastFromIF(IF);
                NTE = NULL;
            } else {

                //
                // Our address object doesn't have an address (i.e. wildcard).
                // Let the routing code pick one.
                //
                NTEorIF = NULL;
                NTE = NULL;
                IF = NULL;
            }
        } else {
            //
            // Our address object has a specific address.  Determine which NTE
            // corresponds to it.
            //
            NTE = FindNetworkWithAddress(&SrcAO->ao_addr, SrcAO->ao_scope_id);
            if (NTE == NULL) {
                //
                // Bad source address.  We don't have a network with the
                // requested address.  Error out.  This can only happen
                // if our original source address has gone away.
                //
                KdPrint(("UDPSend: Bad source address\n"));
                ErrorValue = TDI_INVALID_REQUEST;
                goto ReturnError;
            }

            NTEorIF = CastFromNTE(NTE);
            IF = NULL;
        }

        // 
        // Get the route.
        //
        Status = RouteToDestination(&SendReq->dsr_addr, SendReq->dsr_scope_id,
                                    NTEorIF, RTD_FLAG_NORMAL, &RCE);
        if (IF != NULL)
            ReleaseIF(IF);
        if (Status != IP_SUCCESS) {
            //
            // Failed to get a route to the destination.  Error out.
            //
            if ((Status == IP_PARAMETER_PROBLEM) ||
                (Status == IP_BAD_ROUTE))
                ErrorValue = TDI_BAD_ADDR;
            else if (Status == IP_NO_RESOURCES)
                ErrorValue = TDI_NO_RESOURCES;
            else
                ErrorValue = TDI_DEST_UNREACHABLE;
            if (NTE != NULL)
                ReleaseNTE(NTE);
            goto ReturnError;
        }

        //
        // If our address object didn't have a source address,
        // take the one of the sending net from the RCE.
        // Otherwise, use address from AO.
        //
        if (NTE == NULL) {
            NTE = RCE->NTE;
            AddRefNTE(NTE);
        }

        //
        // Allocate a packet header to anchor the buffer list.
        //
        // REVIEW: We have a separate pool for datagram packets.
        // REVIEW: Should we get these from the IPv6PacketPool instead?
        //
        NdisAllocatePacket(&NdisStatus, &Packet, DGPacketPool);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrint(("UDPSend: Couldn't allocate packet header!?!\n"));
            //
            // If we can't get a packet header from the pool, we push
            // the send request back on the queue and queue the address
            // object for when we get resources.
            //
          OutOfResources:
            ReleaseRCE(RCE);
            ReleaseNTE(NTE);
            KeAcquireSpinLock(&SrcAO->ao_lock, &Irql0);
            PUSHQ(&SrcAO->ao_sendq, &SendReq->dsr_q);
            PutPendingQ(SrcAO);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
            return;
        }
        PC(Packet)->CompletionHandler = DGSendComplete;
        PC(Packet)->CompletionData = SendReq;

        //
        // Our header buffer has extra space at the beginning for other
        // headers to be prepended to ours without requiring further
        // allocation calls.
        //
        Offset = RCE->NCE->IF->LinkHeaderSize;
        HeaderLength = Offset + sizeof(*IP) + sizeof(*UDP);
        Memory = ExAllocatePool(NonPagedPool, HeaderLength);
        if (Memory == NULL) {
            KdPrint(("UDPSend: couldn't allocate header memory!?!\n"));
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        NdisAllocateBuffer(&NdisStatus, &UDPBuffer, DGBufferPool,
                           Memory, HeaderLength);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrint(("UDPSend: couldn't allocate buffer!?!\n"));
            ExFreePool(Memory);
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        //
        // Link the data buffers from the send request onto the buffer
        // chain headed by our header buffer.  Then attach this chain
        // to the packet.
        //
        NDIS_BUFFER_LINKAGE(UDPBuffer) = SendReq->dsr_buffer;
        NdisChainBufferAtFront(Packet, UDPBuffer);

        //
        // We now have all the resources we need to send.
        // Prepare the actual packet.
        //

        PayloadLength = SendReq->dsr_size + sizeof(UDPHeader);

        //
        // Our UDP Header buffer has extra space for other buffers to be
        // prepended to ours without requiring further allocation calls.
        // Put the actual UDP/IP header at the end of the buffer.
        //
        IP = (IPv6Header *)((uchar *)Memory + Offset);
        IP->VersClassFlow = IP_VERSION;
        IP->NextHeader = IP_PROTOCOL_UDP;
        IP->Source = NTE->Address;
        IP->Dest = SendReq->dsr_addr;

        // 
        // Multicast packets have a special hop count option.
        //
        if (IsMulticast(&IP->Dest) && (SrcAO->ao_mcast_hops != -1))
            IP->HopLimit = (uchar) SrcAO->ao_mcast_hops;
        else
            IP->HopLimit = RCE->NCE->IF->CurHopLimit;

        //
        // Fill in UDP Header fields.
        //
        UDP = (UDPHeader *)(IP + 1);
        UDP->Source = SrcAO->ao_port;
        UDP->Dest = SendReq->dsr_port;

        //
        // UDP jumbo-gram support: if the PayloadLength is too large
        // for the UDP Length field, set the field to zero.
        //
        if (PayloadLength > MAX_IPv6_PAYLOAD) {
            UDP->Length = 0;
            ChecksumLength = PayloadLength;
        } else {
            // 
            // Check if the user specified a partial UDP checksum.
            // The possible values are 0, 8, or greater.
            //
            if ((SrcAO->ao_udp_cksum_cover > PayloadLength) ||
                (SrcAO->ao_udp_cksum_cover == 0)) {

                //
                // The checksum coverage is the default so just use the 
                // payload length.  Or, the checksum coverage is bigger
                // than the actual payload so include the payload length.
                //                
                UDP->Length = net_short((ushort)PayloadLength);
                ChecksumLength = PayloadLength;
            } else {
                //
                // The checksum coverage is less than the actual payload
                // so use it in the length field.
                //
                UDP->Length = net_short(SrcAO->ao_udp_cksum_cover);
                ChecksumLength = SrcAO->ao_udp_cksum_cover;
            }
        }

        //
        // Compute the UDP checksum.  It covers the entire UDP datagram
        // starting with the UDP header, plus the IPv6 pseudo-header.
        //
        UDP->Checksum = 0;
        UDP->Checksum = ChecksumPacket(
            Packet, Offset + sizeof *IP, NULL, ChecksumLength,
            &IP->Source, &IP->Dest, IP_PROTOCOL_UDP);

#ifdef NDIS_FLAGS_DONT_LOOPBACK
        //
        // Set the packet send flags for multicast packets.
        //
        NdisQuerySendFlags(Packet, &PacketSendFlags);
        if (IsMulticast(&IP->Dest)) {
            //
            // This is a multicast packet.  Does it need to be 'loopbacked'?
            //
            PacketSendFlags |= NDIS_FLAGS_MULTICAST_PACKET;
            if (SrcAO->ao_mcast_loop == 0) 
                PacketSendFlags |= NDIS_FLAGS_DONT_LOOPBACK;
            else
                PacketSendFlags &= ~NDIS_FLAGS_DONT_LOOPBACK;
        } else {
            // 
            // Not a multicast packet.  Turn off multicast & loopback flags.
            //
            PacketSendFlags &= ~(NDIS_FLAGS_MULTICAST_PACKET |
                                 NDIS_FLAGS_DONT_LOOPBACK);
        }
        NdisSetSendFlags(Packet, PacketSendFlags);
        // KdPrint(("TCPIP6: UDPSend: packet flag=%x\n", PacketSendFlags));
#endif
        //
        // Everything's ready.  Now send the packet.
        //
        // Note that IPv6Send1 does not return a status code.
        // Instead it *always* completes the packet
        // with an appropriate status code.
        //
        UStats.us_outdatagrams++;
        
#ifdef IPSEC_ENABLE
        IPv6Send2(Packet, Offset, IP, PayloadLength, RCE, 
            IP_PROTOCOL_UDP, net_short(UDP->Source), net_short(UDP->Dest));
#else
        IPv6Send1(Packet, Offset, IP, PayloadLength, RCE);
#endif
        

        //
        // Release the route and NTE.
        //
        ReleaseRCE(RCE);
        ReleaseNTE(NTE);


      SendComplete:

        //
        // Check the send queue for more to send.
        //
        KeAcquireSpinLock(&SrcAO->ao_lock, &Irql0);
        if (!EMPTYQ(&SrcAO->ao_sendq)) {
            //
            // More to go.  Dequeue next request and loop back to top.
            //
            DEQUEUE(&SrcAO->ao_sendq, SendReq, DGSendReq, dsr_q);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
        } else {
            //
            // Nothing more to send.
            //
            CLEAR_AO_REQUEST(SrcAO, AO_SEND);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
            return;
        }
    }
}


//* UDPDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a UDP user.  We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer.  Otherwise we'll
//  call the receive datagram event handler, if there is one.  If that
//  fails we'll discard the datagram.
//
void  // Returns: Nothing.
UDPDeliver(
    AddrObj *RcvAO,             // AddrObj to receive datagram.
    IPv6Addr *SrcIP,            // Source address of datagram.
    uint SrcScopeId,            // Scope id for source address.
    ushort SrcPort,             // Source port of datagram.
    PNDIS_PACKET Packet,        // Packet holding data (NULL if none).
    uint Position,              // Current offset into above Packet.
    void *Current,              // Pointer to first data area.
    uint CurrentSize,           // Size of first data area.
    uint Length,                // Size of UDP payload data.
    KIRQL Irql0)                // IRQL prior to acquiring AddrObj table lock.
{
    Queue *CurrentQ;
    KIRQL Irql1;
    DGRcvReq *RcvReq;
    uint BytesTaken = 0;
    uchar AddressBuffer[TCP_TA_SIZE];
    uint RcvdSize;
    EventRcvBuffer *ERB = NULL;

    CHECK_STRUCT(RcvAO, ao);

    KeAcquireSpinLock(&RcvAO->ao_lock, &Irql1);
    KeReleaseSpinLock(&AddrObjTableLock, Irql1);

    if (AO_VALID(RcvAO)) {

        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(DGRcvReq, CurrentQ, drr_q);

            CHECK_STRUCT(RcvReq, drr);

            //
            // If this request is a wildcard request, or matches the source IP
            // address and scope id, check the port.
            //
            if (IsUnspecified(&RcvReq->drr_addr) ||
                (IP6_ADDR_EQUAL(&RcvReq->drr_addr, SrcIP) &&
                 (RcvReq->drr_scope_id == SrcScopeId))) {

                //
                // The remote address matches, check the port.
                // We'll match either 0 or the actual port.
                //
                if (RcvReq->drr_port == 0 || RcvReq->drr_port == SrcPort) {
                    TDI_STATUS Status;

                    // The ports matched. Remove this from the queue.
                    REMOVEQ(&RcvReq->drr_q);

                    // We're done. We can free the AddrObj lock now.
                    KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

                    // Copy the data, and then complete the request.
                    RcvdSize = CopyToBufferChain(RcvReq->drr_buffer, 0,
                                                 Packet, Position, Current,
                                                 MIN(Length,
                                                     RcvReq->drr_size));

                    ASSERT(RcvdSize <= RcvReq->drr_size);

                    Status = UpdateConnInfo(RcvReq->drr_conninfo,
                                            SrcIP, SrcScopeId, SrcPort);

                    UStats.us_indatagrams++;

                    (*RcvReq->drr_rtn)(RcvReq->drr_context, Status, RcvdSize);

                    FreeDGRcvReq(RcvReq);

                    return;  // All done.
                }
            }

            //
            // Either the IP address or the port didn't match.
            // Get the next one.
            //
            CurrentQ = QNEXT(CurrentQ);
        }

        //
        // We've walked the list, and not found a buffer.
        // Call the receive handler now, if we have one.
        //
        if (RcvAO->ao_rcvdg != NULL) {
            PRcvDGEvent RcvEvent = RcvAO->ao_rcvdg;
            PVOID RcvContext = RcvAO->ao_rcvdgcontext;
            TDI_STATUS RcvStatus;
            ULONG Flags = TDI_RECEIVE_COPY_LOOKAHEAD;

            REF_AO(RcvAO);
            KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

            BuildTDIAddress(AddressBuffer, SrcIP, SrcScopeId, SrcPort);

            UStats.us_indatagrams++;
#if 0
            if (IsBCast) {
                Flags |= TDI_RECEIVE_BROADCAST;
            }
#endif
            RcvStatus  = (*RcvEvent)(RcvContext, TCP_TA_SIZE,
                                     (PTRANSPORT_ADDRESS)AddressBuffer,
                                     0, NULL, Flags,
                                     CurrentSize, Length, &BytesTaken,
                                     Current,
                                     &ERB);

            if (RcvStatus == TDI_MORE_PROCESSING) {
                PIO_STACK_LOCATION IrpSp;
                PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

                ASSERT(ERB != NULL);
                ASSERT(BytesTaken <= CurrentSize);

                //
                // For NT, ERBs are really IRPs.
                //
                IrpSp = IoGetCurrentIrpStackLocation(ERB);
                DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
                    &(IrpSp->Parameters);

                //
                // Copy data to the IRP, skipping the bytes
                // that were already taken.
                //
                (uchar *)Current += BytesTaken;
                Position += BytesTaken;
                Length -= BytesTaken;
                RcvdSize = CopyToBufferChain(ERB->MdlAddress, 0, Packet,
                                             Position, Current, Length);

                //
                // Update the return address info.
                //
                RcvStatus = UpdateConnInfo(
                    DatagramInformation->ReturnDatagramInformation,
                    SrcIP, SrcScopeId, SrcPort);

                //
                // Complete the IRP.
                //
                ERB->IoStatus.Information = RcvdSize;
                ERB->IoStatus.Status = RcvStatus;
                IoCompleteRequest(ERB, 2);
            } else {
                ASSERT((RcvStatus == TDI_SUCCESS) ||
                       (RcvStatus == TDI_NOT_ACCEPTED));
                ASSERT(ERB == NULL);
            }

            DELAY_DEREF_AO(RcvAO);

            return;

        } else
            UStats.us_inerrors++;

        //
        // When we get here, we didn't have a buffer to put this data into.
        // Fall through to the return case.
        //
    } else
        UStats.us_inerrors++;

    KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);
}


//* UDPReceive - Receive a UDP datagram.
//
//  The routine called by IP when a UDP datagram arrived.  We look up the
//  port/local address pair in our address table, and deliver the data to
//  a user if we find one.  For multicast frames we may deliver it to
//  multiple users.
//
//  Returns a pointer to the NextHeader field.  Since no other header is
//  allowed to follow the UDP header, this is always NULL.
//
uchar *
UDPReceive(
    IPv6Packet *Packet)         // Packet IP handed up to us.
{
    Interface *IF = Packet->NTEorIF->IF;
    UDPHeader UNALIGNED *UDP;
    KIRQL OldIrql;
    AddrObj *ReceivingAO;
    uint Length;
    uchar DType;
    ushort Checksum;
    AOSearchContext Search;
    AOMCastAddr *AMA, *PrevAMA;
    int MCastReceiverFound;
    uint SrcScopeId, DestScopeId;

    //
    // If an NTE is receiving the packet, check that it is valid.
    //
    if (IsNTE(Packet->NTEorIF) && !IsValidNTE(CastToNTE(Packet->NTEorIF)))
        return NULL; // Drop packet.

    //
    // Verify that we have enough contiguous data to overlay a UDPHeader
    // structure on the incoming packet.  Then do so.
    //
    if (Packet->ContigSize < sizeof(UDPHeader)) {
        if (Packet->TotalSize < sizeof(UDPHeader)) {
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
            return NULL;  // Drop packet.
        }
        if (PacketPullup(Packet, sizeof(UDPHeader)) == 0) {
            // Pullup failed.
            KdPrint(("UDPv6: data buffer too small to contain UDP header\n"));
            UStats.us_inerrors++;
            return NULL;  // Drop packet.
        }
    }
    UDP = (UDPHeader UNALIGNED *)Packet->Data;

    //
    // Verify IPSec was performed.
    //
    if (InboundSPLookup(Packet, IP_PROTOCOL_UDP, net_short(UDP->Source),
                        net_short(UDP->Dest), IF) != TRUE) {
        //
        // No SP was found or the SP found was to drop the packet.
        //
        KdPrint(("UDPReceive: IPSec Policy caused packet to be dropped\n"));
        return NULL;  // Drop packet.
    }

    //
    // Verify UDP length is reasonable.
    //
    // NB: If Length < PayloadLength, then UDP-Lite semantics apply.
    // We checksum only the UDP Length bytes, but we deliver
    // all the bytes to the application.
    //
    Length = (uint) net_short(UDP->Length);
    if ((Length > Packet->TotalSize) || (Length < sizeof *UDP)) {
        //
        // UDP jumbo-gram support: if the UDP length is zero,
        // then use the payload length from IP.
        //
        if (Length != 0) {
            KdPrint(("UDPv6: bogus UDP length (%u vs %u payload)\n",
                     Length, Packet->TotalSize));
            UStats.us_inerrors++;
            return NULL;  // Drop packet.
        }

        Length = Packet->TotalSize;
    }

    //
    // Verify checksum.  Note that the pseudo-header uses Length,
    // not PayloadLength (this was clarified on the IPng mailing list).
    //
    Checksum = ChecksumPacket(Packet->NdisPacket, Packet->Position,
                              Packet->Data, Length,
                              Packet->SrcAddr, &Packet->IP->Dest,
                              IP_PROTOCOL_UDP);
    if (Checksum != 0xffff) {
        KdPrint(("UDPReceive: Checksum failed %0x\n", Checksum));
        return NULL;  // Drop packet.
    }

    //
    // Skip over the UDP header.
    //
    (uchar *)Packet->Data += sizeof *UDP;
    Packet->ContigSize -= sizeof *UDP;
    Packet->TotalSize -= sizeof *UDP;
    Packet->Position += sizeof *UDP;

    //
    // For UPD-Lite: Deliver all the bytes.
    //
    Length = Packet->TotalSize;

    //
    // Set the source's scope id value as appropriate.
    //
    SrcScopeId = DetermineScopeId(Packet->SrcAddr, IF);

    //
    // At this point, we've decided it's okay to accept the packet.
    // Figure out who to give it to.
    //
    if (IsMulticast(&Packet->IP->Dest)) {
        //
        // This is a multicast packet, so we need to find all interested
        // AddrObj's.  We get the AddrObjTable lock, and then loop through
        // all AddrObj's and give the packet to any who are listening to
        // this multicast address, interface & port.
        // REVIEW: We match on interface, NOT scope id.  Multicast is weird.
        //
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);

        //
        // REVIEW: This code assumes that multicast address objects don't
        // REVIEW: have unicast addresses assigned to them (i.e. the socket
        // REVIEW: isn't bound to anything, it's in6addr_any).  It will only
        // REVIEW: find (and thus allow receiving on) AOs with an ao_addr
        // REVIEW: field set to the UnspecifiedAddr.
        //
        ReceivingAO = GetFirstAddrObj(&UnspecifiedAddr, 0, UDP->Dest,
                                      IP_PROTOCOL_UDP, &Search);
        MCastReceiverFound = FALSE;
        for (;ReceivingAO != NULL; ReceivingAO = GetNextAddrObj(&Search)) {
            if ((AMA = FindAOMCastAddr(ReceivingAO, &Packet->IP->Dest,
                                       IF->Index, &PrevAMA)) == NULL)
                continue;

            //
            // We have a matching address object.  Hand it the packet.
            // REVIEW: Change this to take a Packet directly?
            //
            UDPDeliver(ReceivingAO, Packet->SrcAddr, SrcScopeId, UDP->Source,
                       Packet->NdisPacket, Packet->Position, Packet->Data,
                       Packet->ContigSize, Length, OldIrql);
            //
            // UDPDeliver released the AddrObjTableLock, so grab it again.
            //
            KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
            MCastReceiverFound = TRUE;
        }

        if (!MCastReceiverFound)
            UStats.us_noports++;

        KeReleaseSpinLock(&AddrObjTableLock, OldIrql);

    } else {
        //
        // This is a unicast packet.  Try to find an AddrObj to give it to.
        // 
        DestScopeId = DetermineScopeId(&Packet->IP->Dest, IF);
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        ReceivingAO = GetBestAddrObj(&Packet->IP->Dest, DestScopeId, UDP->Dest,
                                     IP_PROTOCOL_UDP);
        if (ReceivingAO != NULL) {
            //
            // We have a matching address object.  Hand it the packet.
            // REVIEW: Change this to take a Packet directly?
            //
            UDPDeliver(ReceivingAO, Packet->SrcAddr, SrcScopeId, UDP->Source,
                       Packet->NdisPacket, Packet->Position, Packet->Data,
                       Packet->ContigSize, Length, OldIrql);

            // Note UDPDeliver released the AddrObjTableLock.

        } else {
            KeReleaseSpinLock(&AddrObjTableLock, OldIrql);

            // Send ICMP Destination Port Unreachable.
            KdPrint(("UDPReceive: No match for packet's address and port\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_DESTINATION_UNREACHABLE,
                            ICMPv6_PORT_UNREACHABLE, 0,
                            IP_PROTOCOL_NONE, FALSE);

            UStats.us_noports++;
        }
    }

    return NULL;
}


//* TCPControlReceive - handler for UDP control messages.
//
//  This routine is called if we receive an ICMPv6 error message that
//  was generated by some remote site as a result of receiving a UDP
//  packet from us.
//  
uchar *
UDPControlReceive(
    IPv6Packet *Packet,  // Packet handed to us by ICMPv6ErrorReceive.
    StatusArg *StatArg)  // Error Code, Argument, and invoking IP header.
{
    //
    // REVIEW - What should we do with the status/error information?
    //
    return NULL;
}


#if 0
//* UDPStatus - Handle a status indication.
//
//  This is the UDP status handler, called by IP when a status event
//  occurs.  For most of these we do nothing.  For certain severe status
//  events we will mark the local address as invalid.
//
void  // Returns: Nothing.
UDPStatus(
    uchar StatusType,      // Type of status (NET or HW).
    IP_STATUS StatusCode,  // Code identifying IP_STATUS.
    IPv6Addr OrigDest,     // For NET, orig dest of triggering datagram.
    IPv6Addr OrigSrc,      // For NET, orig src of triggering datagram.
    IPv6Addr Src,          // Addr of status originator (maybe remote).
    ulong Param,           // Additional info for status.
    void *Data)            // Data pertaining to status.  For NET, this
                           // is the first 8 bytes of the original DG.
{

    if (StatusType == IP_HW_STATUS) {
        //
        // An amazing number of things are grouped under the hardware
        // status designation.
        //

        if (StatusCode == IP_ADDR_DELETED) {
            //
            // An address has gone away.  OrigDest identifies the address.
            //
#ifndef _PNP_POWER
            //
            // This is done via TDI notifications in the PNP world.
            //
            InvalidateAddrs(OrigDest);

#endif  // _PNP_POWER

            return;
        }

        if (StatusCode == IP_ADDR_ADDED) {

            return;
        }
    }
}
#endif
