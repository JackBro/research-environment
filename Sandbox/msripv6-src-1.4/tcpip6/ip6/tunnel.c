// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Generic support for running IPv6 over IPv4.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "tdi.h"
#include "tdiinfo.h"
#include "tdikrnl.h"
#include "tunnel.h"
#include "ntddtcp.h"
#include "tcpinfo.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "ntreg.h"
#include "security.h"

//
// Our globals are all in one structure.
//

TunnelGlobals Tunnel;

//
// Forward definitions of internal functions.
//

extern void
TunnelContextCleanup(TunnelContext *tc);

extern void
TunnelCreateP2PTunnels(TunnelAddress *ta);

//* TunnelOpenAddressObject
//
//  Opens a raw IPv4 address object,
//  returning a handle (or NULL on error).
//
HANDLE
TunnelOpenAddressObject(IPAddr Address)
{
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    PTRANSPORT_ADDRESS transportAddress;
    TA_IP_ADDRESS taIPAddress;
    union { // get correct alignment
        FILE_FULL_EA_INFORMATION ea;
        char bytes[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                  sizeof taIPAddress];
    } eaBuffer;
    PFILE_FULL_EA_INFORMATION ea = &eaBuffer.ea;
    HANDLE tdiHandle;
    NTSTATUS status;

    //
    // Initialize an IPv4 address.
    //
    taIPAddress.TAAddressCount = 1;
    taIPAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    taIPAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    taIPAddress.Address[0].Address[0].sin_port = 0;
    taIPAddress.Address[0].Address[0].in_addr = Address;

    //
    // Initialize the extended-attributes information,
    // to indicate that we are opening an address object
    // with the specified IPv4 address.
    //
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = (USHORT)sizeof taIPAddress;

    RtlMoveMemory(ea->EaName, TdiTransportAddress, ea->EaNameLength + 1);

    transportAddress = (PTRANSPORT_ADDRESS)(&ea->EaName[ea->EaNameLength + 1]);

    RtlMoveMemory(transportAddress, &taIPAddress, sizeof taIPAddress);

    //
    // Open a raw IP address object.
    //

    RtlInitUnicodeString(&objectName, TUNNEL_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes,
                               &objectName,
                               OBJ_CASE_INSENSITIVE,    // Attributes
                               NULL,                    // RootDirectory
                               NULL);                   // SecurityDescriptor

    status = ZwCreateFile(&tdiHandle,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &objectAttributes,
                          &iosb,
                          NULL,                         // AllocationSize
                          0,                            // FileAttributes
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_CREATE,
                          0,                            // CreateOptions
                          ea,
                          sizeof eaBuffer);
    if (!NT_SUCCESS(status))
        return NULL;

    return tdiHandle;
}

typedef struct TunnelCloseAddressObjectContext {
    WORK_QUEUE_ITEM WQItem;
    HANDLE Handle;
} TunnelCloseAddressObjectContext;

//* TunnelCloseAddressObjectWorker
//
//  Executes the close operation in a worker thread context.
//
void
TunnelCloseAddressObjectWorker(void *Context)
{
    TunnelCloseAddressObjectContext *chc =
        (TunnelCloseAddressObjectContext *) Context;

    ZwClose(chc->Handle);
    ExFreePool(chc);
}

//* TunnelCloseAddressObject
//
//  Because the address object handles are opened in the kernel process
//  context, we must always close them in the kernel process context.
//
void
TunnelCloseAddressObject(HANDLE Handle)
{
    if (IoGetCurrentProcess() != Tunnel.KernelProcess) {
        TunnelCloseAddressObjectContext *chc;

        //
        // Punt this operation to a worker thread.
        //

        chc = ExAllocatePool(NonPagedPool, sizeof *chc);
        if (chc == NULL) {
            KdPrint(("TunnelCloseAddressObject - no pool\n"));
            return;
        }

        ExInitializeWorkItem(&chc->WQItem,
                             TunnelCloseAddressObjectWorker, chc);
        chc->Handle = Handle;
        ExQueueWorkItem(&chc->WQItem, CriticalWorkQueue);
    }
    else {
        //
        // It's safe for us to close the handle directly.
        //
        ZwClose(Handle);
    }
}

//* TunnelObjectFromHandle
//
//  Converts a handle to an object pointer.
//
PVOID
TunnelObjectFromHandle(HANDLE Handle)
{
    PVOID Object;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(
                    Handle,
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                    NULL,           // object type
                    KernelMode,
                    &Object,
                    NULL);          // handle info
    ASSERT(NT_SUCCESS(Status));

    return Object;
}

//* TunnelSetAddressObjectInformation
//
//  Set information on the TDI address object.
//
//  Our caller should initialize the ID.toi_id, BufferSize, Buffer
//  fields of the SetInfo structure, but we initialize the rest.
//
NTSTATUS
TunnelSetAddressObjectInformation(
    PFILE_OBJECT AO,
    PTCP_REQUEST_SET_INFORMATION_EX SetInfo,
    ULONG SetInfoSize)
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    //
    // Finish initialization of the request structure for this IOCTL.
    //
    SetInfo->ID.toi_entity.tei_entity = CL_TL_ENTITY;
    SetInfo->ID.toi_entity.tei_instance = 0;
    SetInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
    SetInfo->ID.toi_type = INFO_TYPE_ADDRESS_OBJECT;

    //
    // Initialize the event that we use to wait.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Create and initialize the IRP for this operation.
    //
    irp = IoBuildDeviceIoControlRequest(IOCTL_TCP_SET_INFORMATION_EX,
                                        AO->DeviceObject,
                                        SetInfo,
                                        SetInfoSize,
                                        NULL,   // output buffer
                                        0,      // output buffer length
                                        FALSE,  // internal device control?
                                        &event,
                                        &iosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = (ULONG)-1;

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = AO;

    //
    // Make the IOCTL, waiting for it to finish if necessary.
    //
    status = IoCallDriver(AO->DeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode,
                              FALSE, NULL);
        status = iosb.Status;
    }

    return status;
}

//* TunnelSetAddressObjectTTL
//
//  Set the unicast TTL on a TDI address object.
//  This sets the v4 header TTL that will be used
//  for unicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectTTL(PFILE_OBJECT AO, uchar TTL)
{
    TCP_REQUEST_SET_INFORMATION_EX setInfo;

    setInfo.ID.toi_id = AO_OPTION_TTL;
    setInfo.BufferSize = 1;
    setInfo.Buffer[0] = TTL;

    return TunnelSetAddressObjectInformation(AO, &setInfo, sizeof setInfo);
}

//* TunnelSetAddressObjectMCastTTL
//
//  Set the multicast TTL on a TDI address object.
//  This sets the v4 header TTL that will be used
//  for multicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectMCastTTL(PFILE_OBJECT AO, uchar TTL)
{
    TCP_REQUEST_SET_INFORMATION_EX setInfo;

    setInfo.ID.toi_id = AO_OPTION_MCASTTTL;
    setInfo.BufferSize = 1;
    setInfo.Buffer[0] = TTL;

    return TunnelSetAddressObjectInformation(AO, &setInfo, sizeof setInfo);
}

//* TunnelSetAddressObjectMCastIF
//
//  Set the multicast interface on a TDI address object.
//  This sets the v4 source address that will be used
//  for multicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectMCastIF(PFILE_OBJECT AO, IPAddr Address)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastIFReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_MCASTIF;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastIFReq *) setInfo->Buffer;
    req->umi_addr = Address;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelAddMulticastAddress
//
//  Indicate to the v4 stack that we would like to receive
//  on a multicast address.
//
NTSTATUS
TunnelAddMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_ADD_MCAST;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastReq *) setInfo->Buffer;
    req->umr_if = IfAddress;
    req->umr_addr = MCastAddress;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelDelMulticastAddress
//
//  Indicate to the v4 stack that we are no longer
//  interested in a multicast address.
//
NTSTATUS
TunnelDelMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_DEL_MCAST;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastReq *) setInfo->Buffer;
    req->umr_if = IfAddress;
    req->umr_addr = MCastAddress;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelCreateV4CompatibleNTE
//
//  Create an NTE for a new v4-compatible address
//  on the automatic tunneling pseudo-interface.
//
NetTableEntry *
TunnelCreateV4CompatibleNTE(
    IPAddr Address)
{
    Interface *IF = Tunnel.Pseudo.IF;
    NetTableEntry *NTE;
    IPv6Addr V4CompatibleAddress;
    KIRQL OldIrql;

    V4CompatibleAddress.u.DWord[0] = 0;
    V4CompatibleAddress.u.DWord[1] = 0;
    V4CompatibleAddress.u.DWord[2] = 0;
    V4CompatibleAddress.u.DWord[3] = Address;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    NTE = CreateNTE(IF, &V4CompatibleAddress, FALSE,
                    INFINITE_LIFETIME, INFINITE_LIFETIME);
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return NTE;
}

//* TunnelCreateAddress
//
//  Creates a TunnelAddress structure that contains
//  data structures associated with the IPv4 address.
//
//  The returned TunnelAddress pointer should only
//  be compared against NULL as a failure indication.
//  It must not be dereferenced.
//
TunnelAddress *
TunnelCreateAddress(IPAddr Address)
{
    TunnelAddress *ta;
    KIRQL OldIrql;

    ta = ExAllocatePool(NonPagedPool, sizeof *ta);
    if (ta == NULL)
        goto ErrorReturn;

    ta->Link.DstAddr = 0;
    ta->Link.SrcAddr = Address;

    //
    // Open an IPv4 TDI Address Object that is bound
    // to this address. Packets sent with this AO
    // will use this address as the v4 source.
    //
    ta->Link.AOHandle = TunnelOpenAddressObject(Address);
    if (ta->Link.AOHandle == NULL) {
        KdPrint(("TunnelOpenAddressObject(%s) failed\n",
                 FormatV4Address(Address)));
        goto ErrorReturnFreeMem;
    }
    ta->Link.AOFile = TunnelObjectFromHandle(ta->Link.AOHandle);
    ASSERT(ta->Link.AOFile->DeviceObject == Tunnel.V4Device);

    //
    // Is it really safe to do these synchronously,
    // or are we risking deadlock here?
    //
    (void) TunnelSetAddressObjectTTL(ta->Link.AOFile, TUNNEL_6OVER4_TTL);
    (void) TunnelSetAddressObjectMCastTTL(ta->Link.AOFile, TUNNEL_6OVER4_TTL);
    (void) TunnelSetAddressObjectMCastIF(ta->Link.AOFile, Address);

    //
    // Create the corresponding v4-compatible address
    // on our automatic tunneling pseudo-interface.
    //
    ta->AutoNTE = TunnelCreateV4CompatibleNTE(Address);
    if (ta->AutoNTE == NULL)
        goto ErrorReturnReleaseAO;

    //
    // Create a 6-over-4 virtual interface.
    //
    if (TunnelCreateInterface(ta) != NDIS_STATUS_SUCCESS)
        goto ErrorReturnDestroyNTE;

    //
    // Check the registry and create any point-to-point tunnels
    // that have this IPv4 address as their source address.
    //
    TunnelCreateP2PTunnels(ta);

    //
    // Put this address on our global list.
    // Note that once we unlock, it could be immediately deleted.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    ta->Next = Tunnel.Addresses;
    Tunnel.Addresses = ta;
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    return ta;

  ErrorReturnDestroyNTE:
    // AddrConfTimeout will take care of it.
    ta->AutoNTE->ValidLifetime = 0;
  ErrorReturnReleaseAO:
    ObDereferenceObject(ta->Link.AOFile);
    TunnelCloseAddressObject(ta->Link.AOHandle);
  ErrorReturnFreeMem:
    ExFreePool(ta);
  ErrorReturn:
    return NULL;
}

//* TunnelTransmitComplete
//
//  Completion function for TunnelTransmit,
//  called when the IPv4 stack completes our IRP.
//
NTSTATUS
TunnelTransmitComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    TunnelTransmitContext *tc = (TunnelTransmitContext *) Context;
    Interface *IF = tc->IF;
    PNDIS_PACKET packet = tc->packet;
    NDIS_STATUS status = Irp->IoStatus.Status;

    //
    // Free the state that we allocated in TunnelTransmit.
    //
    ExFreePool(tc);
    IoFreeIrp(Irp);

    //
    // Undo our adjustment before letting upper-layer code
    // see the packet.
    //
    UndoAdjustPacketBuffer(packet);

    //
    // Let IPv6 know that the send completed.
    //
    IPv6SendComplete(IF, packet, status);

    //
    // Tell IoCompleteRequest to stop working on the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelTransmitPacket
//
//  Encapsulate a v6 packet in a v4 packet and send it
//  to the specified v4 address, using the specified
//  file object. The file object implicitly specifies
//  the v4 source address.
//
void
TunnelTransmitPacket(
    Interface *IF,              // Pointer to tunnel interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    IPAddr Address,             // Link-level (IPv4) destination address.
    PFILE_OBJECT File)          // IPv4 TDI Address Object.
{
    TunnelTransmitContext *tc;
    PIRP irp;
    PMDL mdl;
    ULONG SendLen;
    NDIS_STATUS status;

    //
    // We do not need any space for a link-level header,
    // because the IPv4 code takes care of that transparently.
    //
    (void) AdjustPacketBuffer(Packet, Offset, 0);

    //
    // TdiBuildSendDatagram needs an MDL and the total amount
    // of data that the MDL represents.
    //
    NdisQueryPacket(Packet, NULL, NULL, &mdl, &SendLen);

    //
    // Allocate the context that we will pass to the IPv4 stack.
    //
    tc = ExAllocatePool(NonPagedPool, sizeof *tc);
    if (tc == NULL) {
        status = NDIS_STATUS_RESOURCES;
      ErrorReturn:
        UndoAdjustPacketBuffer(Packet);
        IPv6SendComplete(IF, Packet, status);
        return;
    }

    //
    // Allocate an IRP that we will pass to the IPv4 stack.
    //
    irp = IoAllocateIrp(File->DeviceObject->StackSize, FALSE);
    if (irp == NULL) {
        ExFreePool(tc);
        status = NDIS_STATUS_RESOURCES;
        goto ErrorReturn;
    }

    //
    // Initialize the context information.
    // Note that many fields of the "connection info" are unused.
    //
    tc->IF = IF;
    tc->packet = Packet;

    tc->taIPAddress.TAAddressCount = 1;
    tc->taIPAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    tc->taIPAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    tc->taIPAddress.Address[0].Address[0].sin_port = 0;
    tc->taIPAddress.Address[0].Address[0].in_addr = Address;

    tc->tdiConnInfo.RemoteAddressLength = sizeof tc->taIPAddress;
    tc->tdiConnInfo.RemoteAddress = &tc->taIPAddress;

    //
    // Initialize the IRP.
    //
    TdiBuildSendDatagram(irp,
                         File->DeviceObject, File,
                         TunnelTransmitComplete, tc,
                         mdl, SendLen, &tc->tdiConnInfo);

    //
    // Pass the IRP to the IPv4 stack.
    // Note that unlike NDIS's asynchronous operations,
    // our completion routine will always be called,
    // no matter what the return code from IoCallDriver.
    //
    (void) IoCallDriver(File->DeviceObject, irp);
}

//* TunnelTransmit
//
//  Translates the arguments of our link-layer transmit function
//  to the internal TunnelTransmitPacket.
//
void
TunnelTransmit(
    void *Context,              // Pointer to tunnel interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    void *LinkAddress)          // Link-level address.
{
    TunnelContext *tc = (TunnelContext *) Context;
    IPAddr Address = * (IPAddr *) LinkAddress;

    //
    // Suppress packets sent to various illegal destination types.
    //
    if ((Address == INADDR_ANY) ||
        IsV4Broadcast(Address) ||
        IsV4Multicast(Address)) {

        KdPrint(("TunnelTransmit: illegal destination %s\n",
                 FormatV4Address(Address)));
        IPv6SendAbort(CastFromIF(tc->IF), Packet, Offset,
                      ICMPv6_DESTINATION_UNREACHABLE,
                      ICMPv6_COMMUNICATION_PROHIBITED,
                      0,
                      FALSE);
        return;
    }

    //
    // It would be nice to suppress transmission of packets
    // that will result in v4 loopback, but we don't have a
    // convenient way of doing that here. We could check
    // if Address == tc->SrcAddr, but that won't catch most cases.
    // Instead TunnelReceivePacket checks for this.
    //

    //
    // Encapsulate and transmit the packet.
    //
    TunnelTransmitPacket(tc->IF, Packet, Offset,
                         Address, tc->AOFile);
}

//* TunnelTransmitND
//
//  Translates the arguments of our link-layer transmit function
//  to the internal TunnelTransmitPacket.
//
//  This is just like TunnelTransmit, except it doesn't have
//  the checks for bad destination addresses. 6over4 destination
//  addresses are handled via Neighbor Discovery and
//  multicast is needed.
//
void
TunnelTransmitND(
    void *Context,              // Pointer to tunnel interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    void *LinkAddress)          // Link-level address.
{
    TunnelContext *tc = (TunnelContext *) Context;
    IPAddr Address = * (IPAddr *) LinkAddress;

    //
    // Encapsulate and transmit the packet.
    //
    TunnelTransmitPacket(tc->IF, Packet, Offset,
                         Address, tc->AOFile);
}

//* TunnelCreateReceiveIrp
//
//  Creates an IRP for TunnelReceive/TunnelReceiveComplete.
//
PIRP
TunnelCreateReceiveIrp()
{
    PIRP irp;
    PMDL mdl;
    void *buffer;

    irp = IoAllocateIrp(Tunnel.V4Device->StackSize, FALSE);
    if (irp == NULL)
        goto ErrorReturn;

    buffer = ExAllocatePool(NonPagedPool, TUNNEL_RECEIVE_BUFFER);
    if (buffer == NULL)
        goto ErrorReturnFreeIrp;

    mdl = IoAllocateMdl(buffer, TUNNEL_RECEIVE_BUFFER,
                        FALSE, // This is the irp's primary MDL
                        FALSE, // Do not charge quota
                        irp);
    if (mdl == NULL)
        goto ErrorReturnFreeBuffer;

    MmBuildMdlForNonPagedPool(mdl);

    return irp;

  ErrorReturnFreeBuffer:
    ExFreePool(buffer);
  ErrorReturnFreeIrp:
    IoFreeIrp(irp);
  ErrorReturn:
    return NULL;
}

//* TunnelReceivePacket
//
//  Called when we receive an encapsulated IPv6 packet.
//  Called at DPC level.
//
//  We select a single tunnel interface to receive the packet.
//  It's difficult to select the correct interface in all situations.
//  The current algorithm does not allow forwarding
//  on the tunnel pseudo-interface. It gives point-to-point interfaces
//  priority over 6-over-4 interfaces.
//
void
TunnelReceivePacket(
    void *Data,
    uint Length
    )
{
    IPHeader UNALIGNED *IPv4H;
    IPv6Header UNALIGNED *IPv6H;
    uint HeaderLength;
    TunnelAddress *ta;
    IPv6Packet IPPacket;
    uint Flags;
    NetTableEntryOrInterface *NTEorIF;
    ushort Type;
    int PktRefs;

    //
    // The incoming data includes the IPv4 header.
    // We should only get properly-formed IPv4 packets.
    //
    ASSERT(Length >= sizeof *IPv4H);
    IPv4H = (IPHeader UNALIGNED *) Data;
    HeaderLength = ((IPv4H->iph_verlen & 0xf) << 2);
    ASSERT(Length >= HeaderLength);
    ASSERT(IPv4H->iph_protocol == TUNNEL_PROTOCOL);

    Length -= HeaderLength;
    (char *)Data += HeaderLength;

    //
    // If the packet does not contain a full IPv6 header,
    // just ignore it. We need to look at the IPv6 header
    // below to demultiplex the packet to the proper
    // tunnel interface.
    //
    if (Length < sizeof *IPv6H) {
        KdPrint(("TunnelReceivePacket: too small to contain IPv6 hdr\n"));
        return;
    }
    IPv6H = (IPv6Header UNALIGNED *) Data;

    //
    // Check if the packet was received as a link-layer multicast/broadcast.
    //
    if (IsV4Broadcast(IPv4H->iph_dest) ||
        IsV4Multicast(IPv4H->iph_dest))
        Flags = PACKET_NOT_LINK_UNICAST;
    else
        Flags = 0;

    RtlZeroMemory(&IPPacket, sizeof IPPacket);
    IPPacket.Data = Data;
    IPPacket.ContigSize = Length;
    IPPacket.TotalSize = Length;
    IPPacket.Flags = Flags;

    //
    // We want to prevent any loopback in the v4 stack.
    // Loopback should be handled in our v6 routing table.
    // For example, we want to prevent loops where 6to4
    // addresses are routed around and around and around.
    // Without this code, the hop count would eventually
    // catch the loop and report a strange ICMP error.
    //
    if (IPv4H->iph_dest == IPv4H->iph_src) {
        KdPrint(("TunnelReceivePacket: suppressed loopback\n"));

        //
        // Send an ICMP error. This requires some setup.
        //
        IPPacket.IP = IPv6H;
        IPPacket.SrcAddr = &IPv6H->Source;
        IPPacket.IPPosition = IPPacket.Position;
        AdjustPacketParams(&IPPacket, sizeof(IPv6Header));
        IPPacket.NTEorIF = CastFromIF(Tunnel.Pseudo.IF);

        ICMPv6SendError(&IPPacket,
                        ICMPv6_DESTINATION_UNREACHABLE,
                        ICMPv6_NO_ROUTE_TO_DESTINATION,
                        0, IPv6H->NextHeader, FALSE);
        return;
    }

    //
    // First try to receive the packet using the tunnel pseudo-interface.
    //
    NTEorIF = FindAddressOnInterface(Tunnel.Pseudo.IF, &IPv6H->Dest, &Type);
    if (NTEorIF != NULL) {
        PktRefs = IPv6Receive(NTEorIF, &IPPacket);
        ASSERT(PktRefs == 0);
        if (IsNTE(NTEorIF))
            ReleaseNTE(CastToNTE(NTEorIF));
        return;
    }

    //
    // So we can access Tunnel.P2PTunnels and Tunnel.Addresses.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);

    //
    // Next try to receive the packet on the P2P interfaces.
    //
    for (ta = Tunnel.P2PTunnels; ta != NULL; ta = ta->Next) {
        Interface *IF = ta->Link.IF;

        //
        // Restrict the point-to-point tunnel to only receiving
        // packets that are sent from & to the proper link-layer
        // addresses. That is, make it really point-to-point.
        //
        if ((IF != NULL) &&
            (IPv4H->iph_src == ta->Link.DstAddr) &&
            (IPv4H->iph_dest == ta->Link.SrcAddr)) {

            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);
            PktRefs = IPv6Receive(IF, &IPPacket);
            ASSERT(PktRefs == 0);
            ReleaseIF(IF);
            return;
        }
    }

    //
    // Next try to receive the packet on the 6-over-4 interfaces.
    // If additional TunnelAddress's are created during
    // the loop, we'll just ignore the newer ones.
    // So no need to acquire/hold Tunnel.Lock.
    //
    for (ta = Tunnel.Addresses; ta != NULL; ta = ta->Next) {
        Interface *IF = ta->Link.IF;

        //
        // Restrict the 6-over-4 interface to only receiving
        // packets that are sent to proper link-layer addresses.
        // This is our v4 address and multicast addresses
        // from TunnelConvertMulticastAddress.
        // BUGBUG - Multicast destinations might not be delivered
        // to the proper interface if there are multiple 6over4 interfaces.
        //
        if ((IF != NULL) &&
            ((IPv4H->iph_dest == ta->Link.SrcAddr) ||
             ((((uchar *)&IPv4H->iph_dest)[0] == 239) &&
              (((uchar *)&IPv4H->iph_dest)[1] == 192)))) {

            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);
            PktRefs = IPv6Receive(IF, &IPPacket);
            ASSERT(PktRefs == 0);
            ReleaseIF(IF);
            return;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);
}

//* TunnelReceiveComplete
//
//  Completion function for TunnelReceive,
//  called when the IPv4 stack completes our IRP.
//
NTSTATUS
TunnelReceiveComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    NDIS_STATUS status = Irp->IoStatus.Status;
    void *Data;
    ULONG BytesRead;

    ASSERT(Context == NULL);

    if (NT_SUCCESS(status)) {
        //
        // The incoming data includes the IPv4 header.
        // We should only get properly-formed IPv4 packets.
        //
        BytesRead = Irp->IoStatus.Information;
        Data = Irp->MdlAddress->MappedSystemVa;

        TunnelReceivePacket(Data, BytesRead);
    }

    //
    // Put the IRP back so that TunnelReceive can use it again.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);
    ASSERT(Tunnel.ReceiveIrp == NULL);
    Tunnel.ReceiveIrp = Irp;
    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);

    //
    // Tell IoCompleteRequest to stop working on the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelReceive
//
//  Called from the IPv4 protocol stack, when it receives
//  an encapsulated v6 packet.
//
NTSTATUS
TunnelReceive(
    IN PVOID TdiEventContext,       // The event context
    IN LONG SourceAddressLength,    // Length of SourceAddress field.
    IN PVOID SourceAddress,         // Describes the datagram's originator.
    IN LONG OptionsLength,          // Length of Options field.
    IN PVOID Options,               // Options for the receive.
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // Number of bytes this indication.
    IN ULONG BytesAvailable,        // Number of bytes in complete Tsdu.
    OUT ULONG *BytesTaken,          // Number of bytes used.
    IN PVOID Tsdu,                  // Pointer describing this TSDU,
                                    // typically a lump of bytes
    OUT PIRP *IoRequestPacket)      // TdiReceive IRP if
                                    // MORE_PROCESSING_REQUIRED.
{
    PIRP irp;

    ASSERT(TdiEventContext == NULL);
    ASSERT(BytesIndicated <= BytesAvailable);

    //
    // If the packet is too large, refuse to receive it.
    //
    if (BytesAvailable > TUNNEL_RECEIVE_BUFFER) {
        KdPrint(("TunnelReceive - too big %x\n", BytesAvailable));
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // Check if we already have the entire packet to work with.
    // If so, we can directly call TunnelReceivePacket.
    //
    if (BytesIndicated == BytesAvailable) {

        TunnelReceivePacket(Tsdu, BytesIndicated);

        //
        // Tell our caller that we took the data
        // and that we are done.
        //
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // We need an IRP to receive the entire packet.
    // The IRP has a pre-allocated MDL.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);
    irp = Tunnel.ReceiveIrp;
    Tunnel.ReceiveIrp = NULL;
    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);

    //
    // If we don't have an IRP available to us,
    // just drop the packet. This doesn't happen in practice.
    //
    if (irp == NULL) {
        KdPrint(("TunnelReceive - no irp\n"));
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // Build the receive datagram request.
    //
    TdiBuildReceiveDatagram(irp,
                            Tunnel.V4Device,
                            Tunnel.Pseudo.AOFile,
                            TunnelReceiveComplete,
                            NULL,       // Context
                            irp->MdlAddress,
                            BytesAvailable,
                            &Tunnel.ReceiveInputInfo,
                            &Tunnel.ReceiveOutputInfo,
                            0);         // ReceiveFlags

    //
    // Make the next stack location current.  Normally IoCallDriver would
    // do this, but since we're bypassing that, we do it directly.
    //
    IoSetNextIrpStackLocation(irp);

    //
    // Return the irp to our caller.
    //
    *IoRequestPacket = irp;
    *BytesTaken = 0;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelSetReceiveHandler
//
//  Request notification of received IPv4 datagrams.
//
NTSTATUS
TunnelSetReceiveHandler(PVOID EventHandler)
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;

    //
    // Initialize the event that we use to wait.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Create and initialize the IRP for this operation.
    //
    irp = IoBuildDeviceIoControlRequest(0,      // dummy ioctl
                                        Tunnel.V4Device,
                                        NULL,   // input buffer
                                        0,      // input buffer length
                                        NULL,   // output buffer
                                        0,      // output buffer length
                                        TRUE,   // internal device control?
                                        &event,
                                        &iosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = (ULONG)-1;

    TdiBuildSetEventHandler(irp,
                            Tunnel.V4Device, Tunnel.Pseudo.AOFile,
                            NULL, NULL, // comp routine/context
                            TDI_EVENT_RECEIVE_DATAGRAM,
                            EventHandler, NULL);

    //
    // Make the IOCTL, waiting for it to finish if necessary.
    //
    status = IoCallDriver(Tunnel.V4Device, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode,
                              FALSE, NULL);
        status = iosb.Status;
    }

    return status;
}

//* TunnelAddAddress
//
//  Called when a new TDI address is registered.
//  In particular, we want to know when an IPv4 address becomes
//  available (eg, because DHCP finishes).
//
void
TunnelAddAddress(PTA_ADDRESS Address)
{
    PTDI_ADDRESS_IP IPAddress = (PTDI_ADDRESS_IP) Address->Address;
    TunnelAddress *ta;
    KIRQL OldIrql;

    if ((Address->AddressType == TDI_ADDRESS_TYPE_IP) &&
        (IPAddress->in_addr != 0)) {

        //
        // REVIEW: On very rare occasions we've seen the IPv4 stack report
        // a v4 address to us multiple times. For now, do
        // a preliminary check that we haven't already seen this address.
        // Yes, this isn't fool-proof because there's a race once we unlock.
        //
        KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
        for (ta = Tunnel.Addresses; ta != NULL; ta = ta->Next)
            if (ta->Link.SrcAddr == IPAddress->in_addr)
                break;
        KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

        if (ta == NULL)
            ta = TunnelCreateAddress(IPAddress->in_addr);
        else
            KdPrint(("TunnelAddAddress: %s duplicate\n",
                     FormatV4Address(IPAddress->in_addr)));
    }
}

//* TunnelDelAddress
//
//  Called when a TDI address is removed.
//
void
TunnelDelAddress(PTA_ADDRESS Address)
{
    PTDI_ADDRESS_IP IPAddress = (PTDI_ADDRESS_IP) Address->Address;
    TunnelAddress *ta, **pta;
    KIRQL OldIrql;

    if ((Address->AddressType == TDI_ADDRESS_TYPE_IP) &&
        (IPAddress->in_addr != 0)) {
        TunnelAddress *DelList = NULL;

        //
        // Search for the v4 address in our list of address structures.
        // If found, remove it from the list.
        //
        KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
        for (pta = &Tunnel.P2PTunnels; (ta = *pta) != NULL; ) {
            if (ta->Link.SrcAddr == IPAddress->in_addr) {
                //
                // Remove this address structure and put it on our list.
                //
                *pta = ta->Next;
                ta->Next = DelList;
                DelList = ta;
            }
            else
                pta = &ta->Next;
        }
        for (pta = &Tunnel.Addresses; (ta = *pta) != NULL; pta = &ta->Next) {
            if (ta->Link.SrcAddr == IPAddress->in_addr) {
                //
                // Remove this address structure. There should be only one.
                //
                *pta = ta->Next;
                break;
            }
        }
        KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

        if (ta != NULL) {
            //
            // Release the resources held on behalf of this address.
            //
            TunnelContextCleanup(&ta->Link);
            ASSERT(ta->AutoNTE->IF == &Tunnel.Interface);
            KeAcquireSpinLock(&Tunnel.Interface.Lock, &OldIrql);
            DestroyNTE(&Tunnel.Interface, ta->AutoNTE);
            KeReleaseSpinLock(&Tunnel.Interface.Lock, OldIrql);
            ReleaseNTE(ta->AutoNTE);
            ExFreePool(ta);
        }
        else {
            KdPrint(("TunnelDelAddress: %s not found\n",
                     FormatV4Address(IPAddress->in_addr)));
        }

        while ((ta = DelList) != NULL) {
            DelList = ta->Next;

            TunnelContextCleanup(&ta->Link);
            ASSERT(ta->AutoNTE == NULL);
            ExFreePool(ta);
        }
    }
}

//* TunnelCreatePseudoInterface
//
//  Creates a pseudo-interface for automatic and configured tunnels.
//
//  There is only one such pseudo-interface, so we can use
//  statically allocated memory.
//
Interface *
TunnelCreatePseudoInterface()
{
    Interface *IF;
    NeighborCacheEntry *NCE;

    //
    // One reference for being alive
    // and one for our caller.
    //
    IF = &Tunnel.Interface;
    IF->IF = IF;
    IF->RefCnt = 2;

    KeInitializeSpinLock(&IF->Lock);

    IF->LinkContext = &Tunnel.Pseudo;
    IF->Transmit = TunnelTransmit;
    IF->TrueLinkMTU = TUNNEL_MAX_MTU;
    IF->LinkMTU = TUNNEL_DEFAULT_MTU;
    IF->CurHopLimit = DefaultCurHopLimit;
//  IF->DupAddrDetectTransmits = 0;
//  IF->LinkHeaderSize = 0;  // IPv4 code allocates space for the v4 header.
    IF->LinkAddressLength = sizeof(IPAddr);
    IF->LinkAddress = (uchar *) &Tunnel.Pseudo.SrcAddr;

    // Initialize default security policy
    IF->FirstSP = DefaultSP;
    IF->FirstSP->RefCount++;

    //
    // Disable Neighbor Discovery for this interface.
    //
    IF->Flags = 0;

    NeighborCacheInit(IF);

    //
    // Add the pseudo-interface to the global list of interfaces.
    //
    AddInterface(IF);

    return IF;
}

//* TunnelGetPseudo.IF
//
//  Return the pseudo-interface used for configured
//  and automatic tunneling, if tunneling is initialized.
//
Interface *
TunnelGetPseudoIF()
{
    return Tunnel.Pseudo.IF;
}

//* TunnelCreateConfiguredTunnel
//
//  Creates a configured tunnel endpoint.
//
//  Returns FALSE if we fail to init.
//  This should "never" happen, so we are not
//  careful about cleanup in that case.
//
int
TunnelCreateConfiguredTunnel(
    IPv6Addr *V6Addr,
    IPAddr V4Addr,
    int IsRouter)
{
    NeighborCacheEntry *NCE;
    RouteCacheEntry *RCE;
    NetTableEntry *NTE;
    IPv6Addr Neighbor;

    //
    // We use a v4-compatible address as the neighbor address
    // in a permanent NCE that represents the endpoint of the tunnel.
    //
    CreateV4CompatibleAddress(&Neighbor, V4Addr);

    NCE = CreatePermanentNeighbor(Tunnel.Pseudo.IF, &Neighbor, &V4Addr);
    if (NCE == NULL) {
        KdPrint(("TunnelCreateConfiguredTunnel: "
                 "CreatePermanentNeighbor failed\n"));
        return FALSE;
    }

    KdPrint(("Create configured tunnel(%s, %s, %d): NCE %x\n",
             FormatV6Address(V6Addr), FormatV4Address(V4Addr), IsRouter, NCE));

    if (IsRouter) {
        //
        // Make the tunnel endpoint be a default router.
        // NB: We don't set NCE->IsRouter - that field
        // is used with Neighbor Discovery, which does
        // not operate over configured tunnels.
        //
        RouteTableUpdate(Tunnel.Pseudo.IF, NCE,
                         &UnspecifiedAddr, 0, 0, INFINITE_LIFETIME,
                         DEFAULT_NEXT_HOP_PREF, FALSE, FALSE);
    }

    if (!IsUnspecified(V6Addr)) {
        //
        // We only create a route to the v6 destination,
        // if a real v6 address was supplied.
        //
        RouteTableUpdate(Tunnel.Pseudo.IF, NCE,
                         V6Addr, 128, 0, INFINITE_LIFETIME,
                         DEFAULT_NEXT_HOP_PREF, FALSE, FALSE);
    }

    ReleaseNCE(NCE);
    return TRUE;
}

//* TunnelCreateP2PTunnel
//
//  Creates a point-to-point tunnel.
//  Unlike a configured tunnel, this is a true virtual link.
//
//  The SrcAddr argument specifies our endpoint of the tunnel.
//
TunnelAddress *
TunnelCreateP2PTunnel(TunnelAddress *SrcAddr, IPAddr DstAddr)
{
    TunnelAddress *ta;
    KIRQL OldIrql;

    ta = ExAllocatePool(NonPagedPool, sizeof *ta);
    if (ta == NULL)
        goto ErrorReturn;

    ta->AutoNTE = NULL; // not used for point-to-point tunnels
    ta->Link.SrcAddr = SrcAddr->Link.SrcAddr;
    ta->Link.DstAddr = DstAddr;
    //
    // BUGBUG - Bump ref counts on AOFile.
    //
    ta->Link.AOFile = SrcAddr->Link.AOFile;

    //
    // Create a P2P virtual interface.
    //
    if (TunnelCreateInterface(ta) != NDIS_STATUS_SUCCESS)
        goto ErrorReturnFreeMem;

    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    ta->Next = Tunnel.P2PTunnels;
    Tunnel.P2PTunnels = ta;
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    return ta;

  ErrorReturnFreeMem:
    ExFreePool(ta);
  ErrorReturn:
    return NULL;
}

//* TunnelCreateP2PTunnels
//
//  This functions checks for configured P2P tunnels,
//  by reading the registry 
//  It gets called every time a new v4 address is made available.
//
//  The registry looks like
//  ...\Parameters\ConfiguredP2PTunnels\ - where we look
//              <index>\ -      tunnels are numbered 0, 1, 2, etc
//                      DstV4Address - REG_SZ: v4 address of tunnel endpoint
//                      SrcV4Address - REG_SZ: v4 address of our end
//
void
TunnelCreateP2PTunnels(TunnelAddress *ta)
{
    HANDLE RegKeyParam;
    HANDLE RegKey;
    HANDLE RegKeyIndex;
    NTSTATUS status;
    uint index;

    //
    // The parameter key for the IPv6 stack should exist.
    //
    status = OpenRegKey(&RegKeyParam, NULL, Tunnel.RegKeyNameParam);
    if (!NT_SUCCESS(status))
        return;

    //
    // The parameter key for configured tunnels might not exist.
    //
    status = OpenRegKey(&RegKey, RegKeyParam, L"ConfiguredP2PTunnels");
    ZwClose(RegKeyParam);
    if (!NT_SUCCESS(status))
        return;

    for (index = 0; ; index++) {
        WCHAR buffer[32];
        UNICODE_STRING Index;
        UNICODE_STRING SrcV4Address;
        UNICODE_STRING DstV4Address;
        WCHAR *Terminator;
        IPAddr SrcV4Addr,DstV4Addr;
        ULONG Type;

        Index.Length = 0;
        Index.MaximumLength = 32;
        Index.Buffer = buffer;

        status = RtlIntegerToUnicodeString(index, 10, &Index);
        if (!NT_SUCCESS(status))
            break;

        status = OpenRegKey(&RegKeyIndex, RegKey, Index.Buffer);
        if (!NT_SUCCESS(status))
            break;

        SrcV4Address.Length = SrcV4Address.MaximumLength = 0;
        SrcV4Address.Buffer = NULL;

        DstV4Address.Length = DstV4Address.MaximumLength = 0;
        DstV4Address.Buffer = NULL;

        status = GetRegSZValue(RegKeyIndex, L"SrcV4Address",
                               &SrcV4Address, &Type);
        if (!NT_SUCCESS(status) ||
            (Type != REG_SZ) ||
            (SrcV4Address.Buffer == NULL) ||
            !ParseV4Address(SrcV4Address.Buffer, &Terminator, &SrcV4Addr) ||
            (*Terminator != L'\0')) {

            KdPrint(("TunnelCreateP2PTunnels: index %u bad SrcV4Address\n",
                     index));
            goto ErrorLoop;
        }

        //
        // Skip this point-to-point tunnel if it does not match
        // the v4 source address that we are currently configuring.
        //
        if (SrcV4Addr != ta->Link.SrcAddr)
            goto ErrorLoop;

        status = GetRegSZValue(RegKeyIndex, L"DstV4Address",
                               &DstV4Address, &Type);
        if (!NT_SUCCESS(status) ||
            (Type != REG_SZ) ||
            (DstV4Address.Buffer == NULL) ||
            !ParseV4Address(DstV4Address.Buffer, &Terminator, &DstV4Addr) ||
            (*Terminator != L'\0')) {

            KdPrint(("TunnelCreateP2PTunnels: index %u bad DstV4Address\n",
                     index));
            goto ErrorLoop;
        }

        (void) TunnelCreateP2PTunnel(ta, DstV4Addr);

      ErrorLoop:
        if (SrcV4Address.Buffer != NULL)
            ExFreePool(SrcV4Address.Buffer);
        if (DstV4Address.Buffer != NULL)
            ExFreePool(DstV4Address.Buffer);
        ZwClose(RegKeyIndex);
    }

    ZwClose(RegKey);
}

#if 0
//* TunnelCreateConfiguredTunnels
//
//  This functions initializes configured tunnels,
//  by reading the registry and assigned addresses
//  to our tunnel pseudo-interface and creating
//  configured tunnels to other nodes.
//
//  Returns FALSE if we fail to init.
//  This should "never" happen, so we are not
//  careful about cleanup in that case.
//
//  The registry looks like
//  ...\Parameters\ConfiguredTunnels\ - where we look
//              Addresses - REG_MULTI_SZ: v6 addresses assigned to this node,
//                                        in addition to v4-compatible addrs
//              <index>\ -      tunnels are numbered 0, 1, 2, etc
//                              the values below describe the other node
//                      V6Address - REG_SZ: v6 address of tunnel endpoint
//                      V4Address - REG_SZ: v4 address of tunnel endpoint
//                      IsRouter  - REG_DWORD: (optional) is it a router?
//
// The V6Address is optional, in which case IsRouter must be true.
// (If V6Address is not specified, then IsRouter defaults to true.)
//
int
TunnelCreateConfiguredTunnels(void)
{
    HANDLE RegKeyParam;
    HANDLE RegKey;
    HANDLE RegKeyIndex;
    UNICODE_STRING MultiSZ;
    NTSTATUS status;
    uint index;

    //
    // The parameter key for the IPv6 stack should exist.
    //
    status = OpenRegKey(&RegKeyParam, NULL, Tunnel.RegKeyNameParam);
    if (!NT_SUCCESS(status))
        return FALSE;

    //
    // The parameter key for configured tunnels might not exist.
    //
    status = OpenRegKey(&RegKey, RegKeyParam, L"ConfiguredTunnels");
    ZwClose(RegKeyParam);
    if (!NT_SUCCESS(status)) {
        KdPrint(("No configured tunnels.\n"));
        return TRUE;
    }

    MultiSZ.Length = MultiSZ.MaximumLength = 0;
    MultiSZ.Buffer = NULL;

    status = GetRegMultiSZValue(RegKey, L"Addresses", &MultiSZ);
    if (NT_SUCCESS(status) && (MultiSZ.Buffer != NULL)) {
        WCHAR *Address, *Terminator;
        IPv6Addr Addr;

        for (index = 0; ; index++) {
            Address = EnumRegMultiSz(MultiSZ.Buffer, MultiSZ.Length+1, index);
            if (Address == NULL)
                break;

            if (!ParseV6Address(Address, &Terminator, &Addr) ||
                (*Terminator != L'\0')) {

                KdPrint(("TunnelCreateConfiguredTunnels - bad address %ws\n",
                         Address));
                continue;
            }

            (void) CreateNTE(Tunnel.Pseudo.IF, &Addr, FALSE,
                             INFINITE_LIFETIME, INFINITE_LIFETIME);
        }

        ExFreePool(MultiSZ.Buffer);
    }

    //
    // Now that we've assigned address to our own tunnel interface,
    // create configured tunnel endpoints that we can send to.
    // (We must assign our addresses first, for FindBestSourceAddress.)
    //

    for (index = 0; ; index++) {
        WCHAR buffer[32];
        UNICODE_STRING Index;
        UNICODE_STRING V4Address, V6Address;
        WCHAR *Terminator;
        IPAddr V4Addr;
        IPv6Addr V6Addr;
        ULONG Type;
        int IsRouter;

        Index.Length = 0;
        Index.MaximumLength = 32;
        Index.Buffer = buffer;

        status = RtlIntegerToUnicodeString(index, 10, &Index);
        if (!NT_SUCCESS(status))
            break;

        status = OpenRegKey(&RegKeyIndex, RegKey, Index.Buffer);
        if (!NT_SUCCESS(status))
            break;

        V4Address.Length = V4Address.MaximumLength = 0;
        V4Address.Buffer = NULL;

        V6Address.Length = V6Address.MaximumLength = 0;
        V6Address.Buffer = NULL;

        status = GetRegSZValue(RegKeyIndex, L"V4Address",
                               &V4Address, &Type);
        if (!NT_SUCCESS(status) ||
            (Type != REG_SZ) ||
            (V4Address.Buffer == NULL) ||
            !ParseV4Address(V4Address.Buffer, &Terminator, &V4Addr) ||
            (*Terminator != L'\0') ||
            (V4Addr == 0)) {

            KdPrint(("TunnelCreateConfiguredTunnels: index %u bad v4 addr\n",
                     index));
            goto ErrorLoop;
        }

        status = GetRegSZValue(RegKeyIndex, L"V6Address",
                               &V6Address, &Type);
        if (!NT_SUCCESS(status)) {
            //
            // If the registry does not specify a v6 address,
            // then we use the unspecified address to remember this.
            //
            V6Addr = UnspecifiedAddr;
        }
        else if ((Type != REG_SZ) ||
                 (V6Address.Buffer == NULL) ||
                 !ParseV6Address(V6Address.Buffer, &Terminator, &V6Addr) ||
                 (*Terminator != L'\0') ||
                 IsUnspecified(&V6Addr)) {

            KdPrint(("TunnelCreateConfiguredTunnels: index %u bad v6 addr\n",
                     index));
            goto ErrorLoop;
        }

        //
        // If the v6 address is not specified,
        // then the endpoint must be a router.
        //

        InitRegDWORDParameter(RegKeyIndex, L"IsRouter",
                              (ULONG *)&IsRouter,
                              IsUnspecified(&V6Addr));

        if (IsUnspecified(&V6Addr) && !IsRouter) {
            KdPrint(("TunnelCreateConfiguredTunnels: index %u not router\n",
                     index));
            goto ErrorLoop;
        }

        if (!TunnelCreateConfiguredTunnel(&V6Addr, V4Addr, IsRouter))
            return FALSE;

      ErrorLoop:
        if (V6Address.Buffer != NULL)
            ExFreePool(V6Address.Buffer);
        if (V4Address.Buffer != NULL)
            ExFreePool(V4Address.Buffer);
        ZwClose(RegKeyIndex);
    }

    ZwClose(RegKey);
    return TRUE;
}
#endif // 0


//* TunnelInit - Initialize the tunnel module.
//
//  This functions initializes the tunnel module.
//
//  RegKeyNameParam must be a static string,
//  because we keep a pointer to it and we do not
//  free it in TunnelUnload.
//
//  Returns FALSE if we fail to init.
//  This should "never" happen, so we are not
//  careful about cleanup in that case.
//
//  Note we return TRUE if IPv4 is not available,
//  but then tunnel functionality will not be available.
//
int
TunnelInit(WCHAR *RegKeyNameParam)
{
    NTSTATUS status;

    Tunnel.KernelProcess = IoGetCurrentProcess();

    KeInitializeSpinLock(&Tunnel.Lock);

    //
    // Save this for future use. We save a string instead of a key handle
    // because it's bad to keep a key open indefinitely.
    //
    Tunnel.RegKeyNameParam = RegKeyNameParam;

    //
    // This can legitimately fail, if the IPv4 stack
    // is not available.
    //
    Tunnel.Pseudo.AOHandle = TunnelOpenAddressObject(INADDR_ANY);
    if (Tunnel.Pseudo.AOHandle == NULL) {
        KdPrint(("TunnelInit - IPv4 not available?\n"));
        return TRUE;
    }

    Tunnel.Pseudo.AOFile = TunnelObjectFromHandle(Tunnel.Pseudo.AOHandle);
    Tunnel.V4Device = Tunnel.Pseudo.AOFile->DeviceObject;

    Tunnel.ReceiveIrp = TunnelCreateReceiveIrp();
    if (Tunnel.ReceiveIrp == NULL)
        return FALSE;
    RtlZeroMemory(&Tunnel.ReceiveInputInfo, sizeof Tunnel.ReceiveInputInfo);
    RtlZeroMemory(&Tunnel.ReceiveOutputInfo, sizeof Tunnel.ReceiveOutputInfo);

    Tunnel.Addresses = NULL;

    //
    // The tunnel pseudo-interface needs a dummy link-level address.
    // Similarly the pseudo-NCE needs a dummy link-level address.
    // TunnelTransmit must be able to recognize this special v4 address.
    //
    Tunnel.Pseudo.SrcAddr = INADDR_ANY;

    //
    // Initialize the pseudo-interface used
    // for automatic and configured tunneling.
    //
    Tunnel.Pseudo.IF = TunnelCreatePseudoInterface();
    if (Tunnel.Pseudo.IF == NULL)
        return FALSE;

    //
    // Create the routing table entry for automatic tunneling.
    //
    RouteTableUpdate(Tunnel.Pseudo.IF, NULL,
                     &UnspecifiedAddr, 96, 0,
                     INFINITE_LIFETIME, DEFAULT_STATIC_PREF,
                     FALSE, FALSE);

#if 0
    //
    // We don't do this anymore.
    // ipv6.exe is the preferred configuration method.
    //
    // Create any configured tunnels.
    //

    if (!TunnelCreateConfiguredTunnels()) {
        KdPrint(("TunnelCreateConfiguredTunnels failed!\n"));      
        //
        // BUGBUG Should perhaps return FALSE here but
        // that could leave us with callback to functions
        // that are not available anymmore
        //
    }
#endif

    //
    // After TunnelSetReceiveHandler, we will start receiving
    // encapsulated v6 packets.
    //
    status = TunnelSetReceiveHandler(TunnelReceive);
    if (!NT_SUCCESS(status))
        return FALSE;

    //
    // Request notification of TDI address changes.
    // When we are notified of an IPv4 address,
    // we will create and bind an interface for IPv6 to use.
    //
    status = TdiRegisterAddressChangeHandler(TunnelAddAddress,
                                             TunnelDelAddress,
                                             &Tunnel.TdiAddressChangeHandle);
    if (!NT_SUCCESS(status))
        return FALSE;

    return TRUE;
}


//* TunnelContextCleanup
//
//  Releases resources held by a TunnelContext structure.
//
void
TunnelContextCleanup(TunnelContext *tc)
{
    if (tc->IF != NULL) {
        DestroyIF(tc->IF);
        ReleaseIF(tc->IF);
        ObDereferenceObject(tc->AOFile);
        TunnelCloseAddressObject(tc->AOHandle);
    }
}


//* TunnelUnload
//
//  Called to cleanup when the driver is unloading.
//
void
TunnelUnload(void)
{
    TunnelAddress *ta;
    KIRQL OldIrql;

    //
    // Stop receiving encapsulated (v6 in v4) packets.
    //
    (void) TunnelSetReceiveHandler(NULL);

    //
    // Stop address change notifications.
    //
    (void) TdiDeregisterAddressChangeHandler(Tunnel.TdiAddressChangeHandle);

    //
    // The above functions should block until any current TunnelReceive,
    // TunnelAddAddress, TunnelDelAddress callbacks return
    // and they will prevent any new callbacks into us.
    // Hence there will be no further concurrent accesses
    // to our Tunnel data structures. 
    //

    while ((ta = Tunnel.Addresses) != NULL) {
        Tunnel.Addresses = ta->Next;

        TunnelContextCleanup(&ta->Link);
        ReleaseNTE(ta->AutoNTE);
        ExFreePool(ta);
    }

    while ((ta = Tunnel.P2PTunnels) != NULL) {
        Tunnel.P2PTunnels = ta->Next;

        TunnelContextCleanup(&ta->Link);
        ASSERT(ta->AutoNTE == NULL);
        ExFreePool(ta);
    }

    TunnelContextCleanup(&Tunnel.Pseudo);

    //
    // REVIEW - Should we unlock/free the buffer,
    // or will this do it?
    //
    IoFreeMdl(Tunnel.ReceiveIrp->MdlAddress);
    IoFreeIrp(Tunnel.ReceiveIrp);
}


//* TunnelCreateToken
//
//  Given a link-layer address, creates a 64-bit "interface token"
//  in the low eight bytes of an IPv6 address.
//  Does not modify the other bytes in the IPv6 address.
//
void
TunnelCreateToken(
    void *Context,
    void *LinkAddress,
    IPv6Addr *Address)
{
    TunnelContext *tc = (TunnelContext *)Context;

    //
    // Embed the link's interface index in the interface identifier.
    // This makes the interface identifier unique.
    // Otherwise point-to-point tunnel and 6-over-4 links
    // could have the same link-layer address,
    // which is awkward.
    //
    Address->u.DWord[2] = net_long(tc->IF->Index);
    Address->u.DWord[3] = tc->SrcAddr;
}

//* TunnelReadLinkLayerAddressOption
//
//  Parses a Neighbor Discovery link-layer address option
//  and if valid, returns a pointer to the link-layer address.
//
void *
TunnelReadLinkLayerAddressOption(
    void *Context,
    uchar *OptionData)
{
    //
    // Check that the option length is correct.
    //
    if (OptionData[1] != 1)
        return NULL;

    //
    // Check the must-be-zero padding bytes.
    //
    if ((OptionData[2] != 0) || (OptionData[3] != 0))
        return NULL;

    //
    // Return a pointer to the embedded IPv4 address.
    //
    return OptionData + 4;
}

//* TunnelWriteLinkLayerAddressOption
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
TunnelWriteLinkLayerAddressOption(
    void *Context,
    uchar *OptionData,
    void *LinkAddress)
{
    uchar *IPAddress = (uchar *)LinkAddress;

    //
    // Place the address after the option type/length bytes
    // and two bytes of zero padding.
    //
    OptionData[2] = 0;
    OptionData[3] = 0;
    OptionData[4] = IPAddress[0];
    OptionData[5] = IPAddress[1];
    OptionData[6] = IPAddress[2];
    OptionData[7] = IPAddress[3];
}

//* TunnelConvertMulticastAddress
//
//  Converts an IPv6 multicast address to a link-layer address.
//  Generally this requires hashing the IPv6 address into a set
//  of link-layer addresses, in a link-layer-specific way.
//
void
TunnelConvertMulticastAddress(
    void *Context,
    IPv6Addr *Address,
    void *LinkAddress)
{
    TunnelContext *tc = (TunnelContext *)Context;   

    if (tc->DstAddr == 0) {
        uchar *IPAddress = (uchar *)LinkAddress;

        //
        // This is a 6-over-4 link, so create an appropriate
        // v4 multicast address.
        //

        IPAddress[0] = 239;
        IPAddress[1] = 192; // REVIEW: or 128 or 64??
        IPAddress[2] = Address->u.Byte[14];
        IPAddress[3] = Address->u.Byte[15];
    } else {
        IPAddr *IPAddress = (IPAddr *)LinkAddress;

        //
        // This is a point-to-point tunnel, so write in
        // the address of the other side of the tunnel.
        //

        *IPAddress = tc->DstAddr;
    }
}

//* TunnelSetMulticastAddressList
//
//  Takes an array of link-layer multicast addresses
//  (from TunnelConvertMulticastAddress) from which we should
//  receive packets. Passes them to NDIS.
//
NDIS_STATUS
TunnelSetMulticastAddressList(
    void *Context,
    void *LinkAddresses,
    uint NumKeep,
    uint NumAdd,
    uint NumDel)
{
    TunnelContext *tc = (TunnelContext *)Context;
    IPAddr *Addresses = (IPAddr *)LinkAddresses;
    NTSTATUS status;
    uint i;

    //
    // We only do something for 6-over-4 links.
    //
    if (tc->DstAddr == 0) {
        //
        // We add the multicast addresses to Tunnel.Pseudo.AOFile,
        // instead of tc->AOFile, because we are only receiving
        // from the first address object.
        //
        for (i = 0; i < NumAdd; i++) {
            status = TunnelAddMulticastAddress(Tunnel.Pseudo.AOFile,
                                               tc->SrcAddr,
                                               Addresses[NumKeep + i]);
            if (!NT_SUCCESS(status))
                return (NDIS_STATUS)status;
        }

        for (i = 0; i < NumDel; i++) {
            status = TunnelDelMulticastAddress(Tunnel.Pseudo.AOFile,
                                               tc->SrcAddr,
                                               Addresses[NumKeep + NumAdd + i]);
            if (!NT_SUCCESS(status))
                return (NDIS_STATUS)status;
        }
    }

    return NDIS_STATUS_SUCCESS;
}

//* TunnelClose
//
//  Shuts down a tunnel.
//
void
TunnelClose(void *Context)
{
    TunnelContext *tc = (TunnelContext *)Context;
    TunnelContext Copy;
    TunnelAddress *ta;
    KIRQL OldIrql;

    //
    // Look for this interface in our data structures.
    //

    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    for (ta = Tunnel.Addresses; ta != NULL; ta = ta->Next) {
        if (tc == &ta->Link) {
            //
            // Disable this 6over4 interface.
            // Remember the resources for cleanup below,
            // but disable the interface here while the lock is held.
            //
            Copy = *tc;
            tc->IF = NULL;
            tc->AOFile = NULL;
            tc->AOHandle = NULL;
            break;
        }
    }
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    if (ta != NULL) {
        //
        // Now that we've released the lock,
        // we release the resources held for this interface.
        //
        TunnelContextCleanup(&Copy);
    }
}

//* TunnelCreateInterface
//
//  Creates a virtual interface for a specific IPv4 address.
//  Used for both 6-over-4 and point-to-point virtual interfaces
//  that use Neighbor Discovery.
//
NDIS_STATUS
TunnelCreateInterface(TunnelAddress *ta)
{
    LLIPBindInfo BindInfo;      // Holds info for IPv6.
    Interface *IF;

    BindInfo.lip_context = &ta->Link;
    BindInfo.lip_maxmtu = TUNNEL_MAX_MTU;
    BindInfo.lip_defmtu = TUNNEL_DEFAULT_MTU;
    BindInfo.lip_flags = 0;
    // We do not want IPv6 to reserve space for our link-level header.
    BindInfo.lip_hdrsize = 0;
    BindInfo.lip_addrlen = sizeof(IPAddr);
    BindInfo.lip_addr = (uchar *) &ta->Link.SrcAddr;

    BindInfo.lip_token = TunnelCreateToken;
    BindInfo.lip_rdllopt = TunnelReadLinkLayerAddressOption;
    BindInfo.lip_wrllopt = TunnelWriteLinkLayerAddressOption;
    BindInfo.lip_mcaddr = TunnelConvertMulticastAddress;
    BindInfo.lip_transmit = TunnelTransmitND;
    BindInfo.lip_mclist = TunnelSetMulticastAddressList;
    BindInfo.lip_close = TunnelClose;

    return CreateInterface(&BindInfo, (void **)&ta->Link.IF);
}

#include <stdio.h>

//* ParseV4Address
//
int
ParseV4Address(WCHAR *Sz, WCHAR **Terminator, IPAddr *Addr)
{
    uint addr = 0;
    uint numDots = 0;
    WCHAR *number;
    WCHAR c;
    int needDigit = TRUE;
    uint num;

    for (;;) {
        c = *Sz;

        if (needDigit) {
            if ((L'0' <= c) && (c <= L'9')) {
                number = Sz;
                needDigit = FALSE;
            } else
                return FALSE;
        }
        else if ((L'0' <= c) && (c <= L'9'))
            ;
        else {
            if (!ParseNumber(number, 10, 1<<8, &num))
                return FALSE;
            addr = (addr << 8) | num;

            if (c == L'.') {
                numDots++;
                needDigit = TRUE;
            }
            else
                break;
        }

        Sz++;
    }

    if (numDots != 3)
        return FALSE;

    *Terminator = Sz;
    *Addr = net_long(addr);
    return TRUE;
}

//* FormatV4AddressWorker
//
//  Formats an IPv4 address into a buffer.
//  The buffer is assumed to be large enough.
//  (At least 16 characters.)
//
void
FormatV4AddressWorker(char *Sz, IPAddr Addr)
{
    uint addr = net_long(Addr);

    sprintf(Sz, "%u.%u.%u.%u",
            (addr >> 24) & 0xff,
            (addr >> 16) & 0xff,
            (addr >> 8) & 0xff,
            addr & 0xff);
}

#if DBG
//* FormatV4Address - Print an IPv6 address to a buffer.
//
//  Returns a static buffer containing the address.
//  Because the static buffer is not locked,
//  this function is only useful for debug prints.
//
//  Returns char * instead of WCHAR * because %ws
//  is not usable at DPC level in DbgPrint.
//
char *
FormatV4Address(IPAddr Addr)
{
    static char Buffer[40];

    FormatV4AddressWorker(Buffer, Addr);
    return Buffer;
}
#endif // DBG
