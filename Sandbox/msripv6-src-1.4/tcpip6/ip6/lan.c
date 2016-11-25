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
// Internet Protocol Version 6 link-level support for some common
// LAN types: Ethernet, Token Ring, etc.
//


//
// This manifest constant causes the NDIS_PROTOCOL_CHARACTERISTICS struct to 
// use the NDIS 5 format if compiled using the NT 5 ddk.  If using the NT4 ddk
// this has no effect.
//
#ifndef NDIS50
#define NDIS50 1
#endif

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "llip6if.h"
#include "lan.h"
#include "ntddip6.h"

#ifndef NDIS_API
#define NDIS_API
#endif


uint NdisVersion;  // The major NDIS version we actualy register with.

static ulong LanLookahead = LOOKAHEAD_SIZE;

static WCHAR LanName[] = TCPIPV6_NAME;

NDIS_HANDLE LanHandle;  // Our NDIS protocol handle.

typedef struct LanRequest {
    NDIS_REQUEST Request;
    KEVENT Event;
    NDIS_STATUS Status;
} LanRequest;

//* DoNDISRequest - Submit a request to an NDIS driver.
//
//  This is a utility routine to submit a general request to an NDIS
//  driver.  The caller specifes the request code (OID), a buffer and
//  a length.  This routine allocates a request structure, fills it in,
//  and submits the request.
//
NDIS_STATUS
DoNDISRequest(
    LanInterface *Adapter,  // Pointer to the LanInterface adapter strucuture.
    NDIS_REQUEST_TYPE RT,   // Type of request to be done (Set or Query).
    NDIS_OID OID,           // Value to be set/queried.
    void *Info,             // Pointer to the buffer to be passed.
    uint Length,            // Length of data in above buffer.
    uint *Needed)           // Location to fill in with bytes needed in buffer.
{
    LanRequest Request;
    NDIS_STATUS Status;

    // Now fill it in.
    Request.Request.RequestType = RT;
    if (RT == NdisRequestSetInformation) {
        Request.Request.DATA.SET_INFORMATION.Oid = OID;
        Request.Request.DATA.SET_INFORMATION.InformationBuffer = Info;
        Request.Request.DATA.SET_INFORMATION.InformationBufferLength = Length;
    } else {
        Request.Request.DATA.QUERY_INFORMATION.Oid = OID;
        Request.Request.DATA.QUERY_INFORMATION.InformationBuffer = Info;
        Request.Request.DATA.QUERY_INFORMATION.InformationBufferLength = Length;
    }

    //
    // Note that we can NOT use Adapter->ai_event and ai_status here.
    // There may be multiple concurrent DoNDISRequest calls.
    //

    // Initialize our event.
    KeInitializeEvent(&Request.Event, SynchronizationEvent, FALSE);

    if (!Adapter->ai_resetting) {
        // Submit the request.
        NdisRequest(&Status, Adapter->ai_handle, &Request.Request);

        // Wait for it to finish.
        if (Status == NDIS_STATUS_PENDING) {
            (void) KeWaitForSingleObject(&Request.Event, UserRequest,
                                         KernelMode, FALSE, NULL);
            Status = Request.Status;
        }
    } else
        Status = NDIS_STATUS_NOT_ACCEPTED;

    if (Needed != NULL)
        *Needed = Request.Request.DATA.QUERY_INFORMATION.BytesNeeded;

    return Status;
}


//* LanRequestComplete - Lan request complete handler.
//
//  This routine is called by the NDIS driver when a general request
//  completes.  Lan blocks on all requests, so we'll just wake up
//  whoever's blocked on this request.
//
void NDIS_API
LanRequestComplete(
    NDIS_HANDLE Handle,     // Binding handle (really our LanInterface).
    PNDIS_REQUEST Context,  // Request that completed.
    NDIS_STATUS Status)     // Final status of requested command.
{
    LanRequest *Request = (LanRequest *) Context;

    //
    // Signal the completion of a generic synchronous request.
    // See DoNDISRequest.
    //
    Request->Status = Status;
    KeSetEvent(&Request->Event, 0, FALSE);
}


//* LanTransmitComplete - Lan transmit complete handler.
//
//  This routine is called by the NDIS driver when a send completes.
//  This is a pretty time critical operation, we need to get through here
//  quickly.  We just take statistics and call the upper layer send
//  complete handler.
//
void NDIS_API
LanTransmitComplete(
    NDIS_HANDLE Handle,   // Binding handle (really LanInterface we sent on).
    PNDIS_PACKET Packet,  // Packet that was sent.
    NDIS_STATUS Status)   // Final status of send.
{
    LanInterface *Interface = (LanInterface *)Handle;

    Interface->ai_qlen--;

    //
    // Take statistics.
    //
    if (Status == NDIS_STATUS_SUCCESS) {
        UINT TotalLength;

        NdisQueryPacket(Packet, NULL, NULL, NULL, &TotalLength);
        Interface->ai_outoctets += TotalLength;
    } else {
        if (Status == NDIS_STATUS_RESOURCES)
            Interface->ai_outdiscards++;
        else
            Interface->ai_outerrors++;
    }

    UndoAdjustPacketBuffer(Packet);

    IPv6SendComplete(Interface->ai_context, Packet, Status);
}


//* LanTransmit - Send a frame.
//
//  The main Lan transmit routine, called by the upper layer.
//
void
LanTransmit(
    void *Context,        // A pointer to the LanInterface.
    PNDIS_PACKET Packet,  // Packet to send.
    uint Offset,          // Offset from start of packet to IP header.
    void *LinkAddress)    // Link-level address of destination.
{
    LanInterface *Interface = (LanInterface *)Context;
    void *BufAddr;
    NDIS_STATUS Status;

    //
    // Obtain a pointer to space for the link-level header.
    //
    BufAddr = AdjustPacketBuffer(Packet, Offset, Interface->ai_hdrsize);

    switch (Interface->ai_media) {
    case NdisMedium802_3: {
        EtherHeader *Ether;

        // This is an Ethernet.
        Ether = (EtherHeader *)BufAddr;
        RtlCopyMemory(Ether->eh_daddr, LinkAddress, IEEE_802_ADDR_LENGTH);
        RtlCopyMemory(Ether->eh_saddr, Interface->ai_addr,
                      IEEE_802_ADDR_LENGTH);
        Ether->eh_type = net_short(ETYPE_IPv6);

#if 0
        //
        // See if we're using SNAP here.
        //
        if (Interface->ai_hdrsize != sizeof(EtherHeader)) {
                ...
        }
#endif
        break;
    }

    case NdisMediumFddi: {
        FDDIHeader *FDDI;
        SNAPHeader *SNAP;

        // This is a FDDI link.
        FDDI = (FDDIHeader *)BufAddr;
        FDDI->fh_pri = FDDI_PRI;  // Default frame code.
        RtlCopyMemory(FDDI->fh_daddr, LinkAddress, IEEE_802_ADDR_LENGTH);
        RtlCopyMemory(FDDI->fh_saddr, Interface->ai_addr,
                      IEEE_802_ADDR_LENGTH);

        // FDDI always uses SNAP.
        SNAP = (SNAPHeader *)(FDDI + 1);
        SNAP->sh_dsap = SNAP_SAP;
        SNAP->sh_ssap = SNAP_SAP;
        SNAP->sh_ctl = SNAP_UI;
        SNAP->sh_protid[0] = 0;
        SNAP->sh_protid[1] = 0;
        SNAP->sh_protid[2] = 0;
        SNAP->sh_etype = net_short(ETYPE_IPv6);

        break;
    }

    // BUGBUG: Eventually we should support other media types here.

    default:
        KdPrint(("LanTransmit: Unknown media type\n"));
    }

    //
    // Send the packet down to NDIS.
    //

    (Interface->ai_outpcount[AI_UCAST_INDEX])++;
    Interface->ai_qlen++;

    if (!Interface->ai_resetting) {
        NdisSend(&Status, Interface->ai_handle, Packet);
    } else
        Status = NDIS_STATUS_NOT_ACCEPTED;

    if (Status != NDIS_STATUS_PENDING) {
        //
        // The send finished synchronously.
        // Call LanTransmitComplete, unifying our treatment
        // of the synchronous and asynchronous cases.
        //
        LanTransmitComplete((NDIS_HANDLE)Interface, Packet, Status);
    }
}


//* LanOpenAdapterComplete - LanOpen completion handler.
//
//  This routine is called by the NDIS driver when an open adapter
//  call completes.  Wakeup anyone who is waiting for this event.
//
void NDIS_API
LanOpenAdapterComplete(
    NDIS_HANDLE Handle,       // Binding handle (really our LanInterface).
    NDIS_STATUS Status,       // Final status of command.
    NDIS_STATUS ErrorStatus)  // Final error status.
{
    LanInterface *ai = (LanInterface *)Handle;

    UNREFERENCED_PARAMETER(ErrorStatus);

    //
    // Signal whoever is waiting and pass the final status.
    //
    ai->ai_status = Status;
    KeSetEvent(&ai->ai_event, 0, FALSE);
}


//* LanCloseAdapterComplete - Lan close adapter complete handler.
//
//  This routine is called by the NDIS driver when a close adapter
//  call completes.
//
void NDIS_API
LanCloseAdapterComplete(
    NDIS_HANDLE Handle,  // Binding handle (really our LanInterface).
    NDIS_STATUS Status)  // Final status of command.
{
    LanInterface *ai = (LanInterface *)Handle;

    //
    // Signal whoever is waiting and pass the final status.
    //
    ai->ai_status = Status;
    KeSetEvent(&ai->ai_event, 0, FALSE);
}


//* LanTDComplete - Lan transfer data complete handler.
//
//  This routine is called by the NDIS driver when a transfer data
//  call completes.  Hopefully we now have a complete packet we can
//  pass up to IP.  Recycle our TD packet descriptor in any event.
//
void NDIS_API
LanTDComplete(
    NDIS_HANDLE Handle,   // Binding handle (really our LanInterface).
    PNDIS_PACKET Packet,  // The packet used for the Transfer Data (TD).
    NDIS_STATUS Status,   // Final status of command.
    uint BytesCopied)     // Number of bytes copied.
{
    LanInterface *Interface = (LanInterface *)Handle;

    //
    // If things went well, pass TD packet up to IP.
    //
    if (Status == NDIS_STATUS_SUCCESS) {
        PNDIS_BUFFER Buffer;
        IPv6Packet IPPacket;

        RtlZeroMemory(&IPPacket, sizeof IPPacket);

        NdisGetFirstBufferFromPacket(Packet, &Buffer, &IPPacket.Data,
                                     &IPPacket.ContigSize,
                                     &IPPacket.TotalSize);
        ASSERT(IPPacket.ContigSize == IPPacket.TotalSize);

        if (PC(Packet)->pc_nucast)
            IPPacket.Flags |= PACKET_NOT_LINK_UNICAST;

        (void) IPv6Receive(Interface->ai_context, &IPPacket);
    }

    //
    // In any case, put the packet back on the list.
    //
    KeAcquireSpinLockAtDpcLevel(&Interface->ai_lock); 
    PC(Packet)->pc_link = Interface->ai_tdpacket;
    Interface->ai_tdpacket = Packet;
    KeReleaseSpinLockFromDpcLevel(&Interface->ai_lock);
}


//* LanResetComplete - Lan reset complete handler.
//
//  This routine is called by the NDIS driver when a reset completes.
//
void NDIS_API
LanResetComplete(
    NDIS_HANDLE Handle,  // Binding handle (really LanInterface which reset)
    NDIS_STATUS Status)  // Final status of command.
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Status);

    // REVIEW: Do anything here?  Axe this routine?
}


//* LanReceive - Lan receive data handler.
//
//  This routine is called when data arrives from the NDIS driver.
//  Note that newer NDIS drivers are likely to call LanReceivePacket to
//  indicate data arrival instead of this routine.
//
NDIS_STATUS // Indication of whether or not we took the packet.
NDIS_API
LanReceive(
    NDIS_HANDLE Handle,   // The binding handle we gave NDIS earlier.
    NDIS_HANDLE Context,  // NDIS Context for TransferData operations.
    void *Header,         // Pointer to packet link-level header.
    uint HeaderSize,      // Size of above header (in bytes).
    void *Data,           // Pointer to look-ahead received data buffer.
    uint Size,            // Size of above data (in bytes).
    uint TotalSize)       // Total received data size (in bytes).
{
    LanInterface *Interface = Handle;  // Interface for this driver.
    ushort Type;                       // Protocol type.
    uint ProtOffset;                   // Offset in Data to non-media info.
    uint NUCast;                       // TRUE if the frame is not unicast.
    IPv6Packet IPPacket;

    if (Interface->ai_state != INTERFACE_UP) {
        // 
        // Interface is marked as down.
        //
        KdPrint(("IPv6 LanReceive: Interface down\n"));
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    Interface->ai_inoctets += TotalSize;

    switch (Interface->ai_media) {

    case NdisMedium802_3: {
        EtherHeader UNALIGNED *Ether = (EtherHeader UNALIGNED *)Header;

        if (HeaderSize < sizeof(*Ether)) {
            //
            // Header region too small to contain Ethernet header.
            //
            KdPrint(("IPv6 LanReceive: Bogus header size (%d bytes)\n",
                     HeaderSize));
            return NDIS_STATUS_NOT_RECOGNIZED;
        }
        if ((Type = net_short(Ether->eh_type)) >= ETYPE_MIN) {
            //
            // Classic Ethernet, no SNAP header.
            //
            ProtOffset = 0;
            break;
        }

        //
        // 802.3 Ethernet w/ SNAP header.  Protocol type is in
        // different spot.  This is handled the same as FDDI, so
        // just fall into that code...
        //
    }

    case NdisMediumFddi: {
        SNAPHeader UNALIGNED *SNAP = (SNAPHeader UNALIGNED *)Data;

        //
        // If we have a SNAP header that's all we need to look at.
        //
        if (Size >= sizeof(SNAPHeader) && SNAP->sh_dsap == SNAP_SAP &&
            SNAP->sh_ssap == SNAP_SAP && SNAP->sh_ctl == SNAP_UI) {
    
            Type = net_short(SNAP->sh_etype);
            ProtOffset = sizeof(SNAPHeader);
        } else {
            // BUGBUG handle XID/TEST here.
            Interface->ai_uknprotos++;
            return NDIS_STATUS_NOT_RECOGNIZED;
        }
        break;
    }

    case NdisMediumArcnet878_2: {
        //
        // ARCNet has different protocol type encoding than Ethernet,
        // convert the type value here.
        //
        // BUGBUG: Need to add IPv6 values for ARCNet here.
        //
        ARCNetHeader UNALIGNED *AH = (ARCNetHeader UNALIGNED *)Header;

        if (HeaderSize < sizeof(*AH)) {
            //
            // Header region too small to contain ARCNet header.
            //
            KdPrint(("IPv6 LanReceive: Bogus header size (%d bytes)\n",
                     HeaderSize));
            return NDIS_STATUS_NOT_RECOGNIZED;
        }
        ProtOffset = 0;
        if (AH->ah_prot == ARCPROT_IPv6)
            Type = ETYPE_IPv6;
        else
            Type = 0;
        break;
    }

    default:
        // Should never happen.
        KdPrint(("IPv6 LanReceive: Got a packet from an unknown media!?!\n"));
    }

    //
    // See if the packet is for a protocol we handle.
    //
    if (Type != ETYPE_IPv6) {
        Interface->ai_uknprotos++;
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    //
    // Notice if this packet wasn't received in a unicast frame.
    // REVIEW: Is this really a media independent solution?  Do we care?
    //
    NUCast = ((*((uchar UNALIGNED *)Header + Interface->ai_bcastoff) &
               Interface->ai_bcastmask) == Interface->ai_bcastval) ?
             AI_NONUCAST_INDEX : AI_UCAST_INDEX;

    (Interface->ai_inpcount[NUCast])++;

    //
    // Check to see if we have the entire packet.
    //
    if (Size < TotalSize) {
        uint Transferred;
        NDIS_STATUS Status;
        PNDIS_PACKET TdPacket;  // Packet used by NdisTransferData.

        //
        // We need to issue a Transfer Data request to get the
        // portion of the packet we're missing, so we might as well
        // get the whole packet this way and have it be contiguous.
        //

        //
        // Pull a packet to use for the Transfer Data off the queue.
        //
        KeAcquireSpinLockAtDpcLevel(&Interface->ai_lock); 
        TdPacket = Interface->ai_tdpacket;
        if (TdPacket == (PNDIS_PACKET)NULL) {
            // Don't have a packet to put it in.
            // Have to drop it, but let NDIS know we recognized it.
            KeReleaseSpinLockFromDpcLevel(&Interface->ai_lock);
            return NDIS_STATUS_SUCCESS;
        }
        Interface->ai_tdpacket = PC(TdPacket)->pc_link;
        KeReleaseSpinLockFromDpcLevel(&Interface->ai_lock);

        //
        // Remember NUCast in a handy field in the packet context.
        //
        PC(TdPacket)->pc_nucast = NUCast;

        //
        // Issue the TD.  Start transfer at the IP header.
        //
        NdisTransferData(&Status, Interface->ai_handle, Context,
                         ProtOffset, TotalSize - ProtOffset,
                         TdPacket, &Transferred);

        if (Status != NDIS_STATUS_PENDING) {
            //
            // TD completed synchronously,
            // so call the completion function directly.
            //
            LanTDComplete(Handle, TdPacket, Status, Transferred);
        }

        return NDIS_STATUS_SUCCESS;
    }

    //
    // We were given all the data directly.  Just need to skip
    // over any link level headers.
    //
    (uchar *)Data += ProtOffset;
    ASSERT(Size == TotalSize);
    TotalSize -= ProtOffset;

    //
    // Pass incoming data up to IPv6.
    //
    RtlZeroMemory(&IPPacket, sizeof IPPacket);

    IPPacket.Data = Data;
    IPPacket.ContigSize = TotalSize;
    IPPacket.TotalSize = TotalSize;

    if (NUCast)
        IPPacket.Flags |= PACKET_NOT_LINK_UNICAST;

    (void) IPv6Receive(Interface->ai_context, &IPPacket);

    return NDIS_STATUS_SUCCESS;
}


//* LanReceiveComplete - Lan receive complete handler.
//
//  This routine is called by the NDIS driver after some number of
//  receives.  In some sense, it indicates 'idle time'.
//
void NDIS_API
LanReceiveComplete(
    NDIS_HANDLE Handle)  // Binding handle (really our LanInterface).
{
    UNREFERENCED_PARAMETER(Handle);

    IPv6ReceiveComplete();
}


//* LanReceivePacket - Lan receive data handler.
//
//  This routine is called when data arrives from the NDIS driver.
//  Note that older NDIS drivers are likely to call LanReceive to
//  indicate data arrival instead of this routine.
//
int  // Returns: number of references we hold to Packet upon return.
LanReceivePacket(
    NDIS_HANDLE Handle,   // The binding handle we gave NDIS earlier.
    PNDIS_PACKET Packet)  // Packet descriptor for incoming packet.
{
    LanInterface *Interface = Handle;  // Interface for this driver.
    PNDIS_BUFFER Buffer;               // Buffer in packet chain.
    void *Address;                     // Address of above Buffer.
    uint Length, TotalLength;          // Length of Buffer, Packet.
    EtherHeader UNALIGNED *Ether;      // Header for Ethernet media.
    ushort Type;                       // Protocol type.
    uint Position;                     // Offset to non-media info.
    uint NUCast;                       // TRUE if the frame is not unicast.
    IPv6Packet IPPacket;

    if (Interface->ai_state != INTERFACE_UP) {
        // Interface is marked as down.
        return 0;
    }

    //
    // Find out about the packet we've been handed.
    //
    NdisGetFirstBufferFromPacket(Packet, &Buffer, &Address, &Length,
                                 &TotalLength);

    Interface->ai_inoctets += TotalLength;  // Take statistic.

    //
    // Check for obviously bogus packets.
    //
    if (TotalLength < (uint)Interface->ai_hdrsize) {
        // Packet too small to hold media header, drop it.
        return 0;
    }        

    if (Length < (uint)Interface->ai_hdrsize) {
        //
        // First buffer in chain too small to hold header.
        // REVIEW: Are we guaranteed that this will never happen?
        // REVIEW: If so, we can do away with this check.
#if 0
        Insert code to do a packet pullup here
#else
        KdPrint(("IPv6: Yikesahooty!  Got a packet with tiny 1st buffer!\n"));
        KdPrint(("IPv6: 1st buffer length = %d, total length = %d\n",
                 Length, TotalLength));
        return 0;
#endif
    }        

    //
    // Figure out what protocol type this packet is by looking in the
    // media-specific header field for this type of media.
    //
    switch (Interface->ai_media) {

    case NdisMedium802_3: {
        Ether = (EtherHeader UNALIGNED *)Address;

        if ((Type = net_short(Ether->eh_type)) >= ETYPE_MIN) {
            //
            // Classic Ethernet, no SNAP header.
            //
            Position = sizeof(EtherHeader);
        } else {
            //
            // 802.3 Ethernet w/ SNAP header.  Protocol type is in
            // different spot and we have to remember to skip over it.
            // The great thing about standards is that there are so
            // many to choose from.
            //
            SNAPHeader UNALIGNED *SNAP = (SNAPHeader UNALIGNED *)
                ((char *)Address + sizeof(EtherHeader));

            if (Length >= (sizeof(EtherHeader) + sizeof(SNAPHeader))
                && SNAP->sh_dsap == SNAP_SAP && SNAP->sh_ssap == SNAP_SAP
                && SNAP->sh_ctl == SNAP_UI) {

                Type = net_short(SNAP->sh_etype);
                Position = sizeof(EtherHeader) + sizeof(SNAPHeader);
            } else {
                // BUGBUG: Again, do we need to check for pullup issues?
                // BUGBUG: handle XID/TEST here.
                Interface->ai_uknprotos++;
                return 0;
            }
        }
        break;
    }

    case NdisMediumFddi: {
        SNAPHeader UNALIGNED *SNAP = (SNAPHeader UNALIGNED *)
            ((char *)Address + sizeof(FDDIHeader));

        if (Length >= (sizeof(FDDIHeader) + sizeof(SNAPHeader))
            && SNAP->sh_dsap == SNAP_SAP && SNAP->sh_ssap == SNAP_SAP
            && SNAP->sh_ctl == SNAP_UI) {

            Type = net_short(SNAP->sh_etype);
            Position = sizeof(FDDIHeader) + sizeof(SNAPHeader);
        } else {
            // BUGBUG: Again, do we need to check for pullup issues?
            // BUGBUG: handle XID/TEST here.
            Interface->ai_uknprotos++;
            return 0;
        }
        break;
    }

    case NdisMediumArcnet878_2: {
        //
        // ARCNet has different protocol type encoding than Ethernet,
        // convert the type value here.
        //
        // BUGBUG: Need to add IPv6 values for ARCNet here.
        //
        ARCNetHeader UNALIGNED *AH = (ARCNetHeader UNALIGNED *)Address;

        Position = sizeof(ARCNetHeader);
        if (AH->ah_prot == ARCPROT_IPv6)
            Type = ETYPE_IPv6;
        else
            Type = 0;
        break;
    }

    default:
        // Should never happen.
        KdPrint(("IPv6: Got a packet from an unknown media!?!\n"));
    }

    //
    // Notice if this packet wasn't received in a unicast frame.
    // REVIEW: Is this really a media independent solution?
    //
    NUCast = ((*((uchar UNALIGNED *)Address + Interface->ai_bcastoff) &
               Interface->ai_bcastmask) == Interface->ai_bcastval) ?
               AI_NONUCAST_INDEX : AI_UCAST_INDEX;

    //
    // See if the packet is for a protocol we handle.
    //
    if (Type == ETYPE_IPv6) {

        (Interface->ai_inpcount[NUCast])++;

        //
        // Skip over any link level headers.
        //
        (uchar *)Address += Position;
        Length -= Position;
        TotalLength -= Position;

        //
        // Pass incoming data up to IPv6.
        //
        RtlZeroMemory(&IPPacket, sizeof IPPacket);

        IPPacket.Position = Position;
        IPPacket.Data = Address;
        IPPacket.ContigSize = Length;
        IPPacket.TotalSize = TotalLength;
        IPPacket.NdisPacket = Packet;

        if (NUCast)
            IPPacket.Flags |= PACKET_NOT_LINK_UNICAST;

        return IPv6Receive(Interface->ai_context, &IPPacket);

    } else {
        //
        // Not a protocol we handle.
        //
        Interface->ai_uknprotos++;
        return 0;
    }
}


//* LanStatus - Lan status handler.
//
//  Called by the NDIS driver when some sort of status change occurs.
//  We take action depending on the type of status.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      GStatus - General type of status that caused the call.
//      Status - Pointer to a buffer of status specific information.
//      StatusSize - Size of the status buffer.
//
//  Exit: Nothing.
//
void NDIS_API
LanStatus(
    NDIS_HANDLE Handle,   // Binding handle (really our LanInterface).
    NDIS_STATUS GStatus,  // General status type which caused the call.
    void *Status,         // Pointer to buffer of status specific info.
    uint StatusSize)      // Size of the above status buffer.
{
    LanInterface *Interface = Handle;  // Interface for this driver.
    uint Index;

    switch (GStatus) {
    case NDIS_STATUS_RESET_START:
        //
        // While the interface is resetting, we must avoid calling
        // NdisSendPackets, NdisSend, and NdisRequest.
        //
        KdPrint(("LanStatus(%x) - start reset\n", Interface));
        Interface->ai_resetting = TRUE;
        break;
    case NDIS_STATUS_RESET_END:
        KdPrint(("LanStatus(%x) - end reset\n", Interface));
        Interface->ai_resetting = FALSE;
        break;
    case NDIS_STATUS_MEDIA_CONNECT:
        KdPrint(("LanStatus(%x) - media connect\n", Interface));
        SetInterfaceLinkStatus(Interface->ai_context, TRUE);
        break;
    case NDIS_STATUS_MEDIA_DISCONNECT:
        KdPrint(("LanStatus(%x) - media disconnect\n", Interface));
        SetInterfaceLinkStatus(Interface->ai_context, FALSE);
        break;
    default:
        KdPrint(("IPv6: LanStatus(%x) - status %x\n", Interface, GStatus));
        for (Index = 0; Index < StatusSize/4; Index++)
            KdPrint((" status %08x\n", ((uint *)Status)[Index]));
        break;
    }
}


//* LanStatusComplete - Lan status complete handler.
//
//  A routine called by the NDIS driver so that we can do postprocessing
//  after a status event.
//
void NDIS_API
LanStatusComplete(
    NDIS_HANDLE Handle)  // Binding handle (really our LanInterface).
{
    UNREFERENCED_PARAMETER(Handle);

    // REVIEW: Do anything here?
}

extern void NDIS_API
LanBindAdapter(PNDIS_STATUS RetStatus, NDIS_HANDLE BindContext,
               PNDIS_STRING AdapterName, PVOID SS1, PVOID SS2);

extern void NDIS_API
LanUnbindAdapter(PNDIS_STATUS RetStatus, NDIS_HANDLE ProtBindContext,
                 NDIS_HANDLE UnbindContext);

extern NDIS_STATUS NDIS_API
LanPnPEvent(NDIS_HANDLE ProtocolBindingContext,
            PNET_PNP_EVENT NetPnPEvent);

//
// Structure passed to NDIS to tell it how to call Lan Interfaces.
//
// This is carefully arranged so that it can build
// with either the NT 4 or NT 5 DDK, and then in either case
// run on NT 4 (registering with NDIS 4) and
// run on NT 5 (registering with NDIS 5).
//
NDIS50_PROTOCOL_CHARACTERISTICS LanCharacteristics = {
    0,  // NdisMajorVersion
    0,  // NdisMinorVersion
    // This field was added in NT 5. (Previously it was just a hole.)
#ifdef NDIS_FLAGS_DONT_LOOPBACK
    0,  // Filler
#endif
    0,  // Flags
    LanOpenAdapterComplete,
    LanCloseAdapterComplete,
    LanTransmitComplete,
    LanTDComplete,
    LanResetComplete,
    LanRequestComplete,
    LanReceive,
    LanReceiveComplete,
    LanStatus,
    LanStatusComplete,
    { 0, 0, 0 },        // Name
    LanReceivePacket,
    LanBindAdapter,
    LanUnbindAdapter,
    // The type of this field changed between NT 4 and NT 5.
#ifdef NDIS_FLAGS_DONT_LOOPBACK
    LanPnPEvent,
#else
    (TRANSLATE_HANDLER) LanPnPEvent,
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};



#pragma BEGIN_INIT
//* LanInit
//
//  This functions intializes the Lan module.
//  In particular, it registers with NDIS.
//
//  Returns FALSE to indicate failure to initialize.
//
int
LanInit(
    WCHAR *RegKeyNameParam)
{
    NDIS_STATUS Status;

    RtlInitUnicodeString(&LanCharacteristics.Name, LanName);

    //
    // We try to register with NDIS major version = 5.  If this fails we try
    // again for NDIS major version = 4.  If this also fails we exit without
    // any further attempts to register with NDIS.
    //
    LanCharacteristics.MajorNdisVersion = 5;
    NdisRegisterProtocol(&Status, &LanHandle,
                         (NDIS_PROTOCOL_CHARACTERISTICS *) &LanCharacteristics,
                         sizeof(NDIS50_PROTOCOL_CHARACTERISTICS));
    if (Status != NDIS_STATUS_SUCCESS) {
        LanCharacteristics.MajorNdisVersion = 4;
        //
        // NDIS 4 has a different semantics - it has TranslateHandler
        // instead of PnPEventHandler. So do not supply that handler.
        //
#ifdef NDIS_FLAGS_DONT_LOOPBACK
        LanCharacteristics.PnPEventHandler = NULL;
#else
        LanCharacteristics.TranslateHandler = NULL;
#endif
        NdisRegisterProtocol(&Status, &LanHandle,
                        (NDIS_PROTOCOL_CHARACTERISTICS *) &LanCharacteristics,
                        sizeof(NDIS40_PROTOCOL_CHARACTERISTICS));
        if (Status != NDIS_STATUS_SUCCESS) {
            //
            // Can't register at all. Just bail out...
            //
            KdPrint(("LanInit: could not register -> %x\n", Status));
            return FALSE;
        }
    }

    //
    // We've registered OK using NDIS.
    //
    NdisVersion = LanCharacteristics.MajorNdisVersion;
    KdPrint(("LanInit: registered with NDIS %u.\n", NdisVersion));
    return TRUE;
}
#pragma END_INIT


//* LanUnload
//
//  Called when then IPv6 stack is unloading.
//  We need to disconnect from NDIS.
//
void
LanUnload(void)
{
    NDIS_STATUS Status;

    //
    // At this point, the adapters should already all be closed
    // because IPUnload is called first and does that.
    //

    NdisDeregisterProtocol(&Status, LanHandle);
}


#pragma BEGIN_INIT

//* FreeLanInterface - Free a Lan interface
//
//  Called in the event of some sort of initialization failure. We free all
//  the memory associated with an Lan interface.
//
//  REVIEW: rename this LanFreeInterface?
//
void
FreeLanInterface(
    LanInterface *Interface)  // Interface structure to be freed.
{
    NDIS_STATUS Status;
    KIRQL OldIrql;
    NDIS_HANDLE Handle;

    //
    // If we're bound to the adapter, close it now.
    //

    KeAcquireSpinLock(&Interface->ai_lock, &OldIrql);
    if (Interface->ai_handle != (NDIS_HANDLE)NULL) {
        Handle = Interface->ai_handle;
        Interface->ai_handle = NULL;
        KeReleaseSpinLock(&Interface->ai_lock, OldIrql);

        KeInitializeEvent(&Interface->ai_event, SynchronizationEvent, FALSE);

        NdisCloseAdapter(&Status, Handle);

        if (Status == NDIS_STATUS_PENDING) {
            (void) KeWaitForSingleObject(&Interface->ai_event, UserRequest,
                                         KernelMode, FALSE, NULL);
            Status = (NDIS_STATUS)Interface->ai_status;
        }
    } else {
        KeReleaseSpinLock(&Interface->ai_lock, OldIrql);
    }

    //
    // Free the interface structure itself.
    //
    ExFreePool(Interface);
}


//* LanOpen - Open an adapter for reception.
//
//  This routine is called when the upper layer is done initializing and
//  wishes to begin receiveing packets.  The adapter is actually 'open',
//  we just set the packet filter and lookahead size.
//
//  LanSetMulticastAddressList can execute concurrently with LanOpen.
//
void
LanOpen(LanInterface *Adapter)
{
    uint MediaStatus;
    NDIS_STATUS Status;

    Status = DoNDISRequest(Adapter,
                           NdisRequestSetInformation,
                           OID_GEN_CURRENT_LOOKAHEAD,
                           &LanLookahead,
                           sizeof LanLookahead,
                           NULL);
    if (Status != NDIS_STATUS_SUCCESS) {
        Adapter->ai_state = INTERFACE_DOWN;
        return;
    }

    Status = DoNDISRequest(Adapter,
                           NdisRequestSetInformation,
                           OID_GEN_CURRENT_PACKET_FILTER,
                           &Adapter->ai_pfilter,
                           sizeof Adapter->ai_pfilter,
                           NULL);
    if (Status != NDIS_STATUS_SUCCESS) {
        Adapter->ai_state = INTERFACE_DOWN;
        return;
    }

    Adapter->ai_state = INTERFACE_UP;

    Status = DoNDISRequest(Adapter,
                           NdisRequestQueryInformation,
                           OID_GEN_MEDIA_CONNECT_STATUS,
                           &MediaStatus,
                           sizeof MediaStatus,
                           NULL);
    if (Status == NDIS_STATUS_SUCCESS) {
        if (MediaStatus == NdisMediaStateDisconnected) {
            KdPrint(("LanOpen(%x) - media disconnect\n", Adapter));
            SetInterfaceLinkStatus(Adapter->ai_context, FALSE);
        }
    }
}


//* LanAllocateTDPacket
//
//  Allocate a packet for NdisTransferData.
//  We always allocate contiguous space for a full MTU of data.
//
PNDIS_PACKET
LanAllocateTDPacket(
    LanInterface *Interface)  // Interface for which to allocate TD packet.
{
    PNDIS_PACKET Packet;
    void *Mem;
    NDIS_STATUS Status;

    Status = IPv6AllocatePacket(Interface->ai_mtu, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS)
        return NULL;

    return Packet;
}

extern uint UseEtherSNAP(PNDIS_STRING Name);


//* LanRegister - Register a protocol with the Lan module.
//
//  We register an adapter for Lan processing and create a LanInterface
//  structure to represent it.  We also open the NDIS adapter here.
//
//  REVIEW: Should we set the packet filter to NOT accept broadcast packets?
//  REVIEW: Broadcast isn't used in IPv6.  Junk bcast* stuff as well?  Switch
//  REVIEW: this to keeping track of multicasts?
//
int
LanRegister(
    PNDIS_STRING Adapter,             // Name of the adapter to bind to.
    struct LanInterface **Interface)  // Where to return new interace.
{
    LanInterface *ai;  // Pointer to interface struct for this interface.
    NDIS_STATUS Status, OpenStatus;     // Status values.
    uint i = 0;                         // Medium index.
    NDIS_MEDIUM MediaArray[2];
    uchar *buffer;                      // Pointer to our buffers.
    uint mss;
    uint speed;
    uint Needed;
    uint MacOpts;
    uchar bcastmask, bcastval, bcastoff, addrlen, hdrsize, snapsize;
    NDIS_OID OID;
    uint PF;
    PNDIS_BUFFER Buffer;

    //
    // Allocate memory to hold new interface.
    //
    ai = (LanInterface *) ExAllocatePool(NonPagedPool, sizeof(LanInterface));
    if (ai == NULL)
        return FALSE;  // Couldn't allocate memory for this one.
    RtlZeroMemory(ai, sizeof(LanInterface));

    //
    // In actual practice, we've only tested Ethernet and FDDI.
    // So disallow other media for now.
    //
    MediaArray[0] = NdisMedium802_3;
    MediaArray[1] = NdisMediumFddi;
#if 0
    MediaArray[2] = NdisMedium802_5;
    MediaArray[3] = NdisMediumArcnet878_2;
#endif

    // Initialize this adapter interface structure.
    ai->ai_state = INTERFACE_INIT;

    // Initialize the locks.
    KeInitializeSpinLock(&ai->ai_lock);

    KeInitializeEvent(&ai->ai_event, SynchronizationEvent, FALSE);

    // Open the NDIS adapter.
    NdisOpenAdapter(&Status, &OpenStatus, &ai->ai_handle,
                    &i, MediaArray, 2,
                    LanHandle, ai, Adapter, 0, NULL);

    // Block for open to complete.
    if (Status == NDIS_STATUS_PENDING) {
        (void) KeWaitForSingleObject(&ai->ai_event, UserRequest, KernelMode,
                                     FALSE, NULL);
        Status = ai->ai_status;
    }

    ai->ai_media = MediaArray[i];   // Fill in media type.

    //
    // Open adapter completed.  If it succeeded, we'll finish our
    // intialization.  If it failed, bail out now.
    //
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("LanRegister: Adapter failed to initialize.  Status = 0x%x\n",
                 Status));
        ai->ai_handle = NULL;
        FreeLanInterface(ai);
        return FALSE;
    }

    //
    // Read the local link-level address from the adapter.
    //
    switch (ai->ai_media) {
    case NdisMedium802_3:
        addrlen = IEEE_802_ADDR_LENGTH;
        bcastmask = ETHER_BCAST_MASK;
        bcastval = ETHER_BCAST_VAL;
        bcastoff = ETHER_BCAST_OFF;
        OID = OID_802_3_CURRENT_ADDRESS;
        hdrsize = sizeof(EtherHeader);
        if (UseEtherSNAP(Adapter)) {
            hdrsize += sizeof(SNAPHeader);
        }

        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED |
            NDIS_PACKET_TYPE_MULTICAST;
        break;

    case NdisMedium802_5:
        addrlen = IEEE_802_ADDR_LENGTH;
        bcastmask = TR_BCAST_MASK;
        bcastval = TR_BCAST_VAL;
        bcastoff = TR_BCAST_OFF;
        OID = OID_802_5_CURRENT_ADDRESS;
        hdrsize = sizeof(TRHeader) + sizeof(SNAPHeader);
        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED;
        break;

    case NdisMediumFddi:
        addrlen = IEEE_802_ADDR_LENGTH;
        bcastmask = FDDI_BCAST_MASK;
        bcastval = FDDI_BCAST_VAL;
        bcastoff = FDDI_BCAST_OFF;
        OID = OID_FDDI_LONG_CURRENT_ADDR;
        hdrsize = sizeof(FDDIHeader) + sizeof(SNAPHeader);
        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED |
            NDIS_PACKET_TYPE_MULTICAST;
        break;

    case NdisMediumArcnet878_2:
        addrlen = 1;
        bcastmask = ARC_BCAST_MASK;
        bcastval = ARC_BCAST_VAL;
        bcastoff = ARC_BCAST_OFF;
        OID = OID_ARCNET_CURRENT_ADDRESS;
        hdrsize = sizeof(ARCNetHeader);
        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED;
        break;

    default:
        KdBreakPoint();
        FreeLanInterface(ai);
        return FALSE;
    }

    ai->ai_bcastmask = bcastmask;
    ai->ai_bcastval = bcastval;
    ai->ai_bcastoff = bcastoff;
    ai->ai_addrlen = addrlen;
    ai->ai_hdrsize = hdrsize;
    ai->ai_pfilter = PF;

    Status = DoNDISRequest(ai, NdisRequestQueryInformation, OID,
                           ai->ai_addr, addrlen, NULL);

    if (Status != NDIS_STATUS_SUCCESS) {
        //
        // Failed to get link-level address.  Bail.
        //
        KdPrint(("LanRegister: Failed to get link-level address.  "
                 "Status = 0x%x\n", Status));
        FreeLanInterface(ai);
        return FALSE;
    }

    //
    // Read the maximum frame size.
    //
    Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                           OID_GEN_MAXIMUM_FRAME_SIZE, &mss,
                           sizeof(mss), NULL);

    if (Status != NDIS_STATUS_SUCCESS) {
        //
        // Failed to get maximum frame size.  Bail.
        //
        KdPrint(("LanRegister: Failed to get maximum frame size.  "
                 "Status = 0x%x\n", Status));
        FreeLanInterface(ai);
        return FALSE;
    }

    //
    // If this is token ring, figure out the RC len stuff now.
    //
    if (ai->ai_media == NdisMedium802_5) {
        mss -= (sizeof(RC) + (MAX_RD * sizeof(ushort)));
    } else {
        if (ai->ai_media == NdisMediumFddi) {
            mss = MIN(mss, FDDI_MSS);
        }
    }

    ai->ai_mtu = (ushort)mss;

    //
    // Read the speed for local purposes.
    //
    Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                           OID_GEN_LINK_SPEED, &speed, sizeof(speed), NULL);

    if (Status == NDIS_STATUS_SUCCESS) {
        ai->ai_speed = speed * 100L;
    }

    //
    // Read and save the options.
    //
    (void) DoNDISRequest(ai, NdisRequestQueryInformation, OID_GEN_MAC_OPTIONS,
                         &MacOpts, sizeof(MacOpts), NULL);

    //
    // Read and store the vendor description string.
    //
    Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                           OID_GEN_VENDOR_DESCRIPTION, &ai->ai_desc,
                           0, &Needed);

    if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
        (Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {
        //
        // We now know the size we need.  Allocate a buffer.
        //
        buffer = ExAllocatePool(NonPagedPool, Needed);
        if (buffer != NULL) {
            Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                OID_GEN_VENDOR_DESCRIPTION, buffer, Needed, NULL);
            if (Status == NDIS_STATUS_SUCCESS) {
                ai->ai_desc = buffer;
                ai->ai_desclen = Needed;
            }
        }
    }

    //
    // Allocate a Tranfer Data packet for this interface.
    //
    ai->ai_tdpacket = LanAllocateTDPacket(ai);

    *Interface = ai;

    return TRUE;
}

#pragma END_INIT


//* LanCreateToken
//
//  Given a link-layer address, creates a 64-bit "interface identifier"
//  in the low eight bytes of an IPv6 address.
//  Does not modify the other bytes in the IPv6 address.
//
void
LanCreateToken(
    void *Context,      // Unused (nominally, our LanInterface).
    void *LinkAddress,  // Link-layer address.
    IPv6Addr *Address)  // IPv6 address to place token into.
{
    uchar *IEEEAddress = (uchar *)LinkAddress;

    //
    // This is formed the same way for Ethernet and FDDI.
    //
    Address->u.Byte[8] = IEEEAddress[0] ^ 0x2;
    Address->u.Byte[9] = IEEEAddress[1];
    Address->u.Byte[10] = IEEEAddress[2];
    Address->u.Byte[11] = 0xff;
    Address->u.Byte[12] = 0xfe;
    Address->u.Byte[13] = IEEEAddress[3];
    Address->u.Byte[14] = IEEEAddress[4];
    Address->u.Byte[15] = IEEEAddress[5];
}


//* LanReadLinkLayerAddressOption - Parse a ND link-layer address option.
//
//  Parses a Neighbor Discovery link-layer address option
//  and if valid, returns a pointer to the link-layer address.
//
void *
LanReadLinkLayerAddressOption(
    void *Context,      // Interface for which ND option applies.
    uchar *OptionData)  // Option data to parse.
{
    LanInterface *Interface = (LanInterface *)Context;

    //
    // Check that the option length is correct,
    // allowing for the option type/length bytes
    // and rounding up to 8-byte units.
    //
    if (((Interface->ai_addrlen + 2 + 7)/8) != OptionData[1])
        return NULL;

    //
    // Skip over the option type and length bytes,
    // and return a pointer to the option data.
    //
    return OptionData + 2;
}


//* LanWriteLinkLayerAddressOption - Create a ND link-layer address option.
//
//  Creates a Neighbor Discovery link-layer address option.
//  Our caller takes care of the option type & length fields.
//  We handle the padding/alignment/placement of the link address
//  into the option data.
//
//  (Our caller allocates space for the option by adding 2 to the
//  link address length and rounding up to a multiple of 8.)
//
void
LanWriteLinkLayerAddressOption(
    void *Context,      // Interface to create option regarding.
    uchar *OptionData,  // Where the option data resides.
    void *LinkAddress)  // Link-level address.
{
    LanInterface *Interface = (LanInterface *)Context;

    //
    // Place the address after the option type/length bytes.
    //
    RtlCopyMemory(OptionData + 2, LinkAddress, Interface->ai_addrlen);
}


//* LanConvertMulticastAddress
//
//  Converts an IPv6 multicast address to a link-layer address.
//  Generally this requires hashing the IPv6 address into a set
//  of link-layer addresses, in a link-layer-specific way.
//
void
LanConvertMulticastAddress(
    void *Context,      // Unused (nominally, our LanInterface).
    IPv6Addr *Address,  // IPv6 multicast address.
    void *LinkAddress)  // Where link-level address to be filled resides.
{
    uchar *IEEEAddress = (uchar *)LinkAddress;

    //
    // This is formed the same way for Ethernet and FDDI.
    //
    IEEEAddress[0] = 0x33;
    IEEEAddress[1] = 0x33;
    IEEEAddress[2] = Address->u.Byte[12];
    IEEEAddress[3] = Address->u.Byte[13];
    IEEEAddress[4] = Address->u.Byte[14];
    IEEEAddress[5] = Address->u.Byte[15];
}


//* LanSetMulticastAddressList
//
//  Takes an array of link-layer multicast addresses
//  (from LanConvertMulticastAddress) from which we should
//  receive packets.  Passes them to NDIS.
//
NDIS_STATUS
LanSetMulticastAddressList(
    void *Context,
    void *LinkAddresses,
    uint NumKeep,
    uint NumAdd,
    uint NumDel)
{
    LanInterface *Interface = (LanInterface *)Context;
    NDIS_STATUS Status;
    NDIS_OID OID;

    //
    // Set the multicast address list to the current list.
    // The OID to do this depends upon the media type.
    //
    switch (Interface->ai_media) {
    case NdisMedium802_3:
        OID = OID_802_3_MULTICAST_LIST;
        break;
    case NdisMediumFddi:
        OID = OID_FDDI_LONG_MULTICAST_LIST;
        break;
    default:
        KdPrint(("LanSetMulticastAddressList: Unknown media type\n"));
        return NDIS_STATUS_FAILURE;
    }
    Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                           OID, (char *)LinkAddresses,
                           (NumKeep + NumAdd) * Interface->ai_addrlen, NULL);

    //
    // If the above request was successful, then turn off the all-multicast 
    // or all-packets filter if we had previously set one of them.
    //
    if (Status == NDIS_STATUS_SUCCESS) {
        if (Interface->ai_pfilter & NDIS_PACKET_TYPE_ALL_MULTICAST ||
            Interface->ai_pfilter & NDIS_PACKET_TYPE_PROMISCUOUS) {
                
            Interface->ai_pfilter &= ~(NDIS_PACKET_TYPE_ALL_MULTICAST | 
                NDIS_PACKET_TYPE_PROMISCUOUS);
            DoNDISRequest(Interface, NdisRequestSetInformation,
                OID_GEN_CURRENT_PACKET_FILTER,  &Interface->ai_pfilter,
                sizeof(uint), NULL);
        }

        return Status;
    }

    // 
    // We get here only if the NDIS request to set the multicast list fails.
    // First we try to set the packet filter for all multicast packets, and if
    // this fails, we try to set the packet filter for all packets.
    //

    // This code was swiped from the V4 stack: arp.c
    Interface->ai_pfilter |= NDIS_PACKET_TYPE_ALL_MULTICAST;
    Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                      OID_GEN_CURRENT_PACKET_FILTER,  &Interface->ai_pfilter,
                      sizeof(uint), NULL);

    if (Status != NDIS_STATUS_SUCCESS) {
        // All multicast failed, try all packets.
        Interface->ai_pfilter &= ~(NDIS_PACKET_TYPE_ALL_MULTICAST);
        Interface->ai_pfilter |= NDIS_PACKET_TYPE_PROMISCUOUS;
        Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                      OID_GEN_CURRENT_PACKET_FILTER,  &Interface->ai_pfilter,
                      sizeof(uint), NULL);
    }

    return Status;
}


//* LanCloseAdapter
//
//  The IPv6 layer calls this function to close a connection to an adapter.
//
void
LanCloseAdapter(void *Context)
{
    LanInterface *Interface = (LanInterface *)Context;
    NDIS_STATUS Status;

    //
    // Mark adapter down.
    //
    Interface->ai_state = INTERFACE_DOWN;

    //
    // Shut adapter up, so we don't get any more frames.
    //
    Interface->ai_pfilter = 0;
    DoNDISRequest(Interface, NdisRequestSetInformation,
                  OID_GEN_CURRENT_PACKET_FILTER,
                  &Interface->ai_pfilter, sizeof(uint), NULL);

    KeInitializeEvent(&Interface->ai_event, SynchronizationEvent, FALSE);

    //
    // Close the connection to NDIS.
    //
    NdisCloseAdapter(&Status, Interface->ai_handle);

    //
    // Block for close to complete.
    //
    if (Status == NDIS_STATUS_PENDING) {
        (void) KeWaitForSingleObject(&Interface->ai_event,
                                     UserRequest, KernelMode,
                                     FALSE, NULL);
        Status = Interface->ai_status;
    }

    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("LanCloseAdapter(%x) - NdisCloseAdapter -> %x\n",
                 Interface, Status));
    }

    //
    // Release our reference for the interface.
    //
    ReleaseInterface(Interface->ai_context);
}


//* LanBindAdapter - Bind and initialize an adapter.
//
//  Called in a PNP environment to initialize and bind an adapter. We open
//  the adapter and get it running, and then we call up to IP to tell him
//  about it. IP will initialize, and if all goes well call us back to start
//  receiving.
//
void NDIS_API
LanBindAdapter(
    PNDIS_STATUS RetStatus,    // Where to return status of this call.
    NDIS_HANDLE BindContext,   // Handle for calling BindingAdapterComplete.
    PNDIS_STRING AdapterName,  // Name of adapeter.
    PVOID SS1,                 // System specific parameter 1.
    PVOID SS2)                 // System specific parameter 2.
{
    LanInterface *Interface;  // Newly created interface.
    IP_STATUS Status;         // State of IPAddInterface call.
    LLIPBindInfo BindInfo;    // Binding information for IP.
    NDIS_HANDLE Handle;

    //
    // First, open the adapter and get the info.
    //
    if (!LanRegister(AdapterName, &Interface)) {
        *RetStatus = NDIS_STATUS_FAILURE;
        return;
    }

    //
    // OK, we're opened the adapter.  Notify IP about it.
    //
    BindInfo.lip_context = Interface;
    BindInfo.lip_transmit = LanTransmit;
    BindInfo.lip_token = LanCreateToken;
    BindInfo.lip_rdllopt = LanReadLinkLayerAddressOption;
    BindInfo.lip_wrllopt = LanWriteLinkLayerAddressOption;
    BindInfo.lip_mcaddr = LanConvertMulticastAddress;
    BindInfo.lip_mclist = LanSetMulticastAddressList;
    BindInfo.lip_close = LanCloseAdapter;
    BindInfo.lip_flags = LIP_MCLOOP_FLAG;
    BindInfo.lip_defmtu = BindInfo.lip_maxmtu = Interface->ai_mtu;
    BindInfo.lip_hdrsize = Interface->ai_hdrsize;
    BindInfo.lip_addrlen = Interface->ai_addrlen;
    BindInfo.lip_addr = Interface->ai_addr;

    if (CreateInterface(&BindInfo, &Interface->ai_context) !=
                                                NDIS_STATUS_SUCCESS) {
        //
        // Attempt to create IP interface failed.  Need to close the binding.
        // FreeLanInterface will do that, as well as freeing resources.
        //
        FreeLanInterface(Interface);
        *RetStatus = NDIS_STATUS_FAILURE;
    } else {
        LanOpen(Interface);
        *RetStatus = NDIS_STATUS_SUCCESS;
    }
}


//* LanUnbindAdapter - Unbind from an adapter.
//
//  Called when we need to unbind from an adapter.
//  We'll notify IP, then free our memory and return.
//
void NDIS_API  // Returns: Nothing.
LanUnbindAdapter(
    PNDIS_STATUS RetStatus,       // Where to return status from this call.
    NDIS_HANDLE ProtBindContext,  // Context we gave NDIS earlier.
    NDIS_HANDLE UnbindContext)    // Context for completing this request.
{
    LanInterface *Interface = (LanInterface *)ProtBindContext;

    //
    // Call IP to destroy the interface.
    // IP will call LanCloseAdapter.
    //
    DestroyInterface(Interface->ai_context);

    *RetStatus = NDIS_STATUS_SUCCESS;
}


//* LanPnPEvent
//
//  Gets called for plug'n'play and power-management events.
//
NDIS_STATUS NDIS_API
LanPnPEvent(
    NDIS_HANDLE ProtocolBindingContext,
    PNET_PNP_EVENT NetPnPEvent)
{
    LanInterface *Interface = (LanInterface *) ProtocolBindingContext;
    NET_DEVICE_POWER_STATE PowerState;

    if ((NetPnPEvent->NetEvent == NetEventSetPower) &&
        (NetPnPEvent->BufferLength >= sizeof PowerState)) {
        //
        // Get the power state of the interface.
        //
        PowerState = * (NET_DEVICE_POWER_STATE *) NetPnPEvent->Buffer;

        KdPrint(("LanPnPEvent(%x) - set power %u\n",
                 Interface, PowerState));

        //
        // We ignore the events that tell us about power going away.
        // But when power comes back, we query for connect status.
        // NDIS does not report connect/disconnect events that occur
        // while there is no power.
        //
        if (PowerState == NetDeviceStateD0) {
            uint MediaStatus;
            NDIS_STATUS Status;

            Status = DoNDISRequest(Interface,
                                   NdisRequestQueryInformation,
                                   OID_GEN_MEDIA_CONNECT_STATUS,
                                   &MediaStatus,
                                   sizeof MediaStatus,
                                   NULL);
            if (Status == NDIS_STATUS_SUCCESS) {
                //
                // Note that we may be redundantly setting the link status.
                // For example saying that it is disconnected when the
                // IPv6 interface status is already disconnected,
                // or vice-versa. The IPv6 code must deal with this.
                //
                SetInterfaceLinkStatus(Interface->ai_context,
                                MediaStatus != NdisMediaStateDisconnected);
            }
        }
    }
    else {
        KdPrint(("LanPnPEvent(%x) - code %u length %u\n",
                 Interface,
                 NetPnPEvent->NetEvent,
                 NetPnPEvent->BufferLength));
    }

    return NDIS_STATUS_SUCCESS;
}
