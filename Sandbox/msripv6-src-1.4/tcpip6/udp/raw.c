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
// Raw IP interface code.  This file contains the code for the raw IP
// interface functions, principally send and receive datagram.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "raw.h"
#include "info.h"
#include "route.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

//
// REVIEW: Shouldn't this be in an include file somewhere?
//
#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'rPCT')

#endif // POOL_TAGGING


extern KSPIN_LOCK AddrObjTableLock;

extern void *RawProtInfo;


//* RawSend - Send a datagram.
//
//  The real send datagram routine.  We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send q.
//
void                     // Returns: Nothing
RawSend(
    AddrObj *SrcAO,      // Pointer to AddrObj doing the send.
    DGSendReq *SendReq)  // Pointer to sendreq describing send.
{
    KIRQL Irql0;
    RouteCacheEntry *RCE;
    NetTableEntryOrInterface *NTEorIF;
    NetTableEntry *NTE;
    Interface *IF;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER RawBuffer;
    void *Memory;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    TDI_STATUS ErrorValue;
    uint HeaderLength;

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
        if (IsUnspecified(&SrcAO->ao_addr)) {
            if (IsMulticast(&SendReq->dsr_addr) && (SrcAO->ao_mcast_if != 0)) {

                // 
                // This is a multicast packet and the user application has
                // specified an interface number to use for this socket.
                //
                IF = FindInterfaceFromIndex(SrcAO->ao_mcast_if);
                if (IF == NULL) {
                    KdPrint(("RawSend: Bad mcast interface number\n"));
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
                KdPrint(("RawSend: Bad source address\n"));
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
        if (NTE != NULL)
            ReleaseNTE(NTE);
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
            goto ReturnError;
        }

        //
        // Allocate a packet header to anchor the buffer list.
        //
        // REVIEW: We have a separate pool for datagram packets.
        // REVIEW: Should we get these from the IPv6PacketPool instead?
        //
        NdisAllocatePacket(&NdisStatus, &Packet, DGPacketPool);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrint(("RawSend: Couldn't allocate packet header!?!\n"));
            //
            // If we can't get a packet header from the pool, we push
            // the send request back on the queue and queue the address
            // object for when we get resources.
            //
          OutOfResources:
            ReleaseRCE(RCE);
            KeAcquireSpinLock(&SrcAO->ao_lock, &Irql0);
            PUSHQ(&SrcAO->ao_sendq, &SendReq->dsr_q);
            PutPendingQ(SrcAO);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
            return;
        }
        PC(Packet)->CompletionHandler = DGSendComplete;
        PC(Packet)->CompletionData = SendReq;

        //
        // Our header buffer contains only the link-level header.
        // The IPv6 header and any extension headers are expected to
        // be provided by the user.
        //
        HeaderLength = RCE->NCE->IF->LinkHeaderSize;
        Memory = ExAllocatePool(NonPagedPool, HeaderLength);
        if (Memory == NULL) {
            KdPrint(("RawSend: couldn't allocate header memory!?!\n"));
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        NdisAllocateBuffer(&NdisStatus, &RawBuffer, DGBufferPool,
                           Memory, HeaderLength);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrint(("RawSend: couldn't allocate buffer!?!\n"));
            ExFreePool(Memory);
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        //
        // Link the data buffers from the send request onto the buffer
        // chain headed by our header buffer.  Then attach this chain
        // to the packet.
        //
        NDIS_BUFFER_LINKAGE(RawBuffer) = SendReq->dsr_buffer;
        NdisChainBufferAtFront(Packet, RawBuffer);

        //
        // We now have all the resources we need to send.
        // Note that the header is supplied by the user.
        //

        UStats.us_outdatagrams++;

        //
        // Everything's ready.  Now send the packet.
        //
        IPv6Send0(Packet, HeaderLength, RCE->NCE, &(RCE->NTE->Address));

        //
        // Release the route.
        //
        ReleaseRCE(RCE);

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


//* RawDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a raw user.  We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer.  Otherwise we'll
//  call the receive datagram event handler, if there is one.  If that
//  fails we'll discard the datagram.
//
void  // Returns: Nothing.
RawDeliver(
    AddrObj *RcvAO,             // Address object to receive the datagram.
    IPv6Addr *SrcIP,            // Source IP address of datagram.
    uint SrcScopeId,            // Scope id for source address.
    PNDIS_PACKET Packet,        // Packet holding data (NULL if none).
    uint Position,              // Current offset into above Packet.
    void *Current,              // Pointer to first data area.
    uint CurrentSize,           // Size of first data area.
    uint Length,                // Amount of data to deliver.
    KIRQL Irql0)                // IRQL prior to taking AddrObj Table Lock.
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
            // If this request is a wildcard request (accept from anywhere),
            // or matches the source IP address and scope id, deliver it.
            //
            if (IsUnspecified(&RcvReq->drr_addr) ||
                (IP6_ADDR_EQUAL(&RcvReq->drr_addr, SrcIP) &&
                 (RcvReq->drr_scope_id == SrcScopeId))) {

                TDI_STATUS Status;

                // Remove this from the queue.
                REMOVEQ(&RcvReq->drr_q);

                // We're done. We can free the AddrObj lock now.
                KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

                // Copy the data and then complete the request.
                RcvdSize = CopyToBufferChain(RcvReq->drr_buffer, 0,
                                             Packet, Position, Current,
                                             MIN(Length, RcvReq->drr_size));

                ASSERT(RcvdSize <= RcvReq->drr_size);

                Status = UpdateConnInfo(RcvReq->drr_conninfo, SrcIP,
                                        SrcScopeId, 0);

                UStats.us_indatagrams++;

                (*RcvReq->drr_rtn)(RcvReq->drr_context, Status, RcvdSize);

                FreeDGRcvReq(RcvReq);

                return;  // All done.
            }

            // Not a matching request.  Get the next one off the queue.
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

            REF_AO(RcvAO);
            KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

            BuildTDIAddress(AddressBuffer, SrcIP, SrcScopeId, 0);

            UStats.us_indatagrams++;
            RcvStatus  = (*RcvEvent)(RcvContext, TCP_TA_SIZE,
                                     (PTRANSPORT_ADDRESS)AddressBuffer, 0,
                                     NULL, TDI_RECEIVE_COPY_LOOKAHEAD,
                                     CurrentSize, Length, &BytesTaken,
                                     Current, &ERB);

            if (RcvStatus == TDI_MORE_PROCESSING) {
                PIO_STACK_LOCATION IrpSp;
                PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

                //
                // We were passed back a receive buffer.  Copy the data in now.
                // Receive event handler can't have taken more than was in the
                // indicated buffer, but in debug builds we'll check this.
                //
                ASSERT(ERB != NULL);
                ASSERT(BytesTaken <= CurrentSize);

                //
                // For NT, ERBs are really IRPs.
                //
                IrpSp = IoGetCurrentIrpStackLocation(ERB);
                DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
                    &(IrpSp->Parameters);

                //
                // Copy data to the IRP, skipping the bytes that
                // were already taken.
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
                    SrcIP, SrcScopeId, 0);

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


//* RawReceive - Receive a Raw datagram.
//
//  This routine is called by IP when a Raw datagram arrived.  We
//  lookup the protocol/address pair in our address table, and deliver
//  the data to any users we find.
//
//  Note that we'll only get here if all headers in the packet
//  preceeding the one we're filtering on were acceptable.
//
//  We don't return anything.  We don't care if this fails.
//
void
RawReceive(
    IPv6Packet *Packet,  // Packet IP handed up to us.
    uint Protocol)       // Protocol we think we're handling.
{
    Interface *IF = Packet->NTEorIF->IF;
    KIRQL OldIrql;
    AddrObj *ReceivingAO;
    AOSearchContext Search;
    AOMCastAddr *AMA, *PrevAMA;
    uint SrcScopeId, DestScopeId;

    KdPrint(("RawReceive: Called\n"));

    //
    // If an NTE is receiving the packet, check that it is valid.
    //
    if (IsNTE(Packet->NTEorIF) && !IsValidNTE(CastToNTE(Packet->NTEorIF)))
        return;

    //
    // This being the raw receive routine, we perform no checks on
    // the packet data.
    //

    //
    // Set the source's scope id value as appropriate.
    //
    SrcScopeId = DetermineScopeId(Packet->SrcAddr, IF);

    //
    // Figure out who to give this packet to.
    //
    if (IsMulticast(&Packet->IP->Dest)) {
        //
        // This is a multicast packet, so we need to find all interested
        // AddrObj's.  We get the AddrObjTable lock, and then loop through
        // all AddrObj's and give the packet to any who are listening to
        // this multicast address, interface & protocol.
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
        ReceivingAO = GetFirstAddrObj(&UnspecifiedAddr, 0, 0, (uchar)Protocol,
                                      &Search);
        for (;ReceivingAO != NULL; ReceivingAO = GetNextAddrObj(&Search)) {
            if ((AMA = FindAOMCastAddr(ReceivingAO, &Packet->IP->Dest,
                                       IF->Index, &PrevAMA)) == NULL)
                continue;

            //
            // We have a matching address object.  Hand it the packet.
            // REVIEW: Change this to take a Packet directly?
            //
            RawDeliver(ReceivingAO, Packet->SrcAddr, SrcScopeId,
                       Packet->NdisPacket, Packet->Position, Packet->Data,
                       Packet->ContigSize, Packet->TotalSize, OldIrql);
            //
            // RawDeliver released the AddrObjTableLock, so grab it again.
            //
            KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        }

    } else {
        //
        // This is a unicast packet.  Try to find some AddrObj(s) to
        // give it to.  We deliver to all matches, not just the first.
        //
        DestScopeId = DetermineScopeId(&Packet->IP->Dest, IF);
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        ReceivingAO = GetFirstAddrObj(&Packet->IP->Dest, DestScopeId, 0,
                                      (uchar)Protocol, &Search);
        for (; ReceivingAO != NULL; ReceivingAO = GetNextAddrObj(&Search)) {
            //
            // We have a matching address object.  Hand it the packet.
            // REVIEW: Change this to take a Packet directly?
            //
            RawDeliver(ReceivingAO, Packet->SrcAddr, SrcScopeId,
                       Packet->NdisPacket, Packet->Position, Packet->Data,
                       Packet->ContigSize, Packet->TotalSize, OldIrql);

            //
            // RawDeliver released the AddrObjTableLock, so grab it again.
            //
            KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        }
    }

    KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
    return;
}


#if 0

//* RawStatus - Handle a status indication.
//
//  This is the Raw status handler, called by IP when a status event
//  occurs.  For most of these we do nothing.  For certain severe status
//  events we will mark the local address as invalid.
//
//  Note that NET status is usually caused by a received ICMP message.
//  HW status indicates a HW problem.
//
void  // Returns: Nothing
RawStatus(
    uchar StatusType,      // Type of status (NET or HW).
    IP_STATUS StatusCode,  // Code identifying IP_STATUS.
    IPv6Addr OrigDest,     // For NET, orig dest of triggering datagram.
    IPv6Addr OrigSrc,      // For NET, orig src of triggering datagram.
    IPv6Addr Src,          // Address of status originator (maybe remote).
    ulong Param,           // Additional info for status.
    void *Data)            // Data pertaining to status.  For NET, this
                           // is the first 8 bytes of the original datagram.
{

    // If this is a HW status, it could be because we've had an address go
    // away.
    if (StatusType == IP_HW_STATUS) {

        if (StatusCode == IP_ADDR_DELETED) {

            // An address has gone away.  OrigDest identifies the address.

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
