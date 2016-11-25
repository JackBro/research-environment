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
// NT specific routines for dispatching and handling IRPs.
//


#include <oscfg.h>
#include <ndis.h>
#include <ip6imp.h>
#include "ip6def.h"
#include "icmp.h"
#include "ipsec.h"
#include "security.h"
#include <ntddip6.h>
#include "route.h"
#include "neighbor.h"

//
// Local structures.
//
typedef struct pending_irp {
    LIST_ENTRY Linkage;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PVOID Context;
} PENDING_IRP, *PPENDING_IRP;


//
// Global variables.
//
LIST_ENTRY PendingEchoList;


//
// Local prototypes.
//
NTSTATUS
IPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDispatchDeviceControl(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPCreate(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPCleanup(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPClose(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
DispatchEchoRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

void
CompleteEchoRequest(void *Context, IP_STATUS Status,
                    IPv6Addr *Address, uint ScopeId,
                    void *Data, uint DataSize);

NTSTATUS
IoctlQueryInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryNeighborCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryRouteCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlCreateSecurityPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySecurityPolicyList(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlDeleteSecurityPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlCreateSecurityAssociation(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySecurityAssociationList(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlDeleteSecurityAssociation(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryRouteTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdateRouteTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdateAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryBindingCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlControlInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlDeleteInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlFlushNeighborCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlFlushRouteCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlSetMobilitySecurity(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlSortDestAddrs(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySitePrefix(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdateSitePrefix(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

//
// All of this code is pageable.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IPDispatch)
#pragma alloc_text(PAGE, IPDispatchDeviceControl)
#pragma alloc_text(PAGE, IPCreate)
#pragma alloc_text(PAGE, IPClose)
#pragma alloc_text(PAGE, DispatchEchoRequest)

#endif // ALLOC_PRAGMA


//
// Dispatch function definitions.
//

//* IPDispatch
//
//  This is the dispatch routine for IP.
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPDispatch(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for target device.
    IN PIRP Irp)                     // I/O request packet.
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MajorFunction) {

    case IRP_MJ_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_CREATE:
        status = IPCreate(Irp, irpSp);
        break;

    case IRP_MJ_CLEANUP:
        status = IPCleanup(Irp, irpSp);
        break;

    case IRP_MJ_CLOSE:
        status = IPClose(Irp, irpSp);
        break;

    default:
        KdPrint(("IPDispatch: Invalid major function %d\n",
                 irpSp->MajorFunction));
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

} // IPDispatch


//* IPDispatchDeviceControl
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPDispatchDeviceControl(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    ULONG code;

    PAGED_CODE();

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (code) {

    case IOCTL_ICMPV6_ECHO_REQUEST:
        return DispatchEchoRequest(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_INTERFACE:
        return IoctlQueryInterface(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ADDRESS:
        return IoctlQueryAddress(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_NEIGHBOR_CACHE:
        return IoctlQueryNeighborCache(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ROUTE_CACHE:
        return IoctlQueryRouteCache(Irp, IrpSp);

    case IOCTL_IPV6_CREATE_SECURITY_POLICY:
        return IoctlCreateSecurityPolicy(Irp, IrpSp);
    
    case IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST:
        return IoctlQuerySecurityPolicyList(Irp, IrpSp);

    case IOCTL_IPV6_DELETE_SECURITY_POLICY:
        return  IoctlDeleteSecurityPolicy(Irp, IrpSp);
        
    case IOCTL_IPV6_CREATE_SECURITY_ASSOCIATION:
        return IoctlCreateSecurityAssociation(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST:
        return IoctlQuerySecurityAssociationList(Irp, IrpSp);   

    case IOCTL_IPV6_DELETE_SECURITY_ASSOCIATION:
        return IoctlDeleteSecurityAssociation(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ROUTE_TABLE:
        return IoctlQueryRouteTable(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_ROUTE_TABLE:
        return IoctlUpdateRouteTable(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_ADDRESS:
        return IoctlUpdateAddress(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_BINDING_CACHE:
        return IoctlQueryBindingCache(Irp, IrpSp);

    case IOCTL_IPV6_CONTROL_INTERFACE:
        return IoctlControlInterface(Irp, IrpSp);

    case IOCTL_IPV6_DELETE_INTERFACE:
        return IoctlDeleteInterface(Irp, IrpSp);

    case IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE:
        return IoctlFlushNeighborCache(Irp, IrpSp);

    case IOCTL_IPV6_FLUSH_ROUTE_CACHE:
        return IoctlFlushRouteCache(Irp, IrpSp);

    case IOCTL_IPV6_SET_MOBILITY_SECURITY:
        return IoctlSetMobilitySecurity(Irp, IrpSp);

    case IOCTL_IPV6_SORT_DEST_ADDRS:
        return IoctlSortDestAddrs(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_SITE_PREFIX:
        return IoctlQuerySitePrefix(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_SITE_PREFIX:
        return IoctlUpdateSitePrefix(Irp, IrpSp);

    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

} // IPDispatchDeviceControl


//* IPCreate
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPCreate(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPCreate


//* IPCleanup
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPCleanup(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PPENDING_IRP pendingIrp;
    PLIST_ENTRY entry, nextEntry;
    KIRQL oldIrql;
    LIST_ENTRY completeList;
    PIRP cancelledIrp;

    InitializeListHead(&completeList);

    //
    // Collect all of the pending IRPs on this file object.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingEchoList.Flink;

    while ( entry != &PendingEchoList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
            nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        }
        else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while ( entry != &completeList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        ExFreePool(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    return(STATUS_SUCCESS);

} // IPCleanup


//* IPClose
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPClose(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPClose


//
// ICMP Echo function definitions
//

//* CancelEchoRequest
//
//  This function is called with cancel spinlock held.  It must be
//  released before the function returns.
//
//  The echo control block associated with this request cannot be
//  freed until the request completes.  The completion routine will
//  free it.
//
VOID
CancelEchoRequest(
    IN PDEVICE_OBJECT Device,  // Device on which the request was issued.
    IN PIRP Irp)               // I/O request packet to cancel.
{
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        ExFreePool(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    return;

} // CancelEchoRequest


//* CompleteEchoRequest
//
//  Handles the completion of an ICMP Echo request.
//
void
CompleteEchoRequest(
    void *Context,     // EchoControl structure for this request.
    IP_STATUS Status,  // Status of the transmission.
    IPv6Addr *Address, // Source of the echo reply.
    uint ScopeId,      // Scope of the echo reply source.
    void *Data,        // Pointer to data returned in the echo reply.
    uint DataSize)     // Lengh of the returned data.
{
    KIRQL oldIrql;
    PIRP irp;
    EchoControl *controlBlock;
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;
    ULONG bytesReturned;

    controlBlock = (EchoControl *) Context;

    //
    // Find the echo request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        ExFreePool(controlBlock);
        return;
    }

    irp->IoStatus.Status = ICMPv6EchoComplete(
        controlBlock,
        Status,
        Address,
        ScopeId,
        Data,
        DataSize,
        &irp->IoStatus.Information
        );

    ExFreePool(pendingIrp);
    ExFreePool(controlBlock);

    //
    // Complete the IRP.
    //
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);

} // CompleteEchoRequest


//* PrepareEchoIrpForCancel
//
//  Prepares an Echo IRP for cancellation.
//
BOOLEAN  // Returns: TRUE if IRP was already cancelled, FALSE otherwise.
PrepareEchoIrpForCancel(
    PIRP Irp,                 // I/O request packet to init for cancellation.
    PPENDING_IRP PendingIrp)  // PENDING_IRP structure for this IRP.
{
    BOOLEAN cancelled = TRUE;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelEchoRequest);
        InsertTailList(&PendingEchoList, &(PendingIrp->Linkage));
        cancelled = FALSE;
    }

    IoReleaseCancelSpinLock(oldIrql);

    return(cancelled);

} // PrepareEchoIrpForCancel


//* DispatchEchoRequest
//
//  Processes an ICMP request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful. The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
DispatchEchoRequest(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IP_STATUS ipStatus;
    PPENDING_IRP pendingIrp;
    EchoControl *controlBlock;
    PICMPV6_ECHO_REPLY replyBuffer;
    BOOLEAN cancelled;

    PAGED_CODE();

    pendingIrp = ExAllocatePool(NonPagedPool, sizeof(PENDING_IRP));

    if (pendingIrp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto echo_error;
    }

    controlBlock = ExAllocatePool(NonPagedPool, sizeof(EchoControl));

    if (controlBlock == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(pendingIrp);
        goto echo_error;
    }

    pendingIrp->Irp = Irp;
    pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;

    controlBlock->WhenIssued = KeQueryPerformanceCounter(NULL);
    controlBlock->ReplyBuf = Irp->AssociatedIrp.SystemBuffer;
    controlBlock->ReplyBufLen =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    IoMarkIrpPending(Irp);

    cancelled = PrepareEchoIrpForCancel(Irp, pendingIrp);

    if (!cancelled) {
        ipStatus = ICMPv6EchoRequest(
            Irp->AssociatedIrp.SystemBuffer,                     // request buf
            IrpSp->Parameters.DeviceIoControl.InputBufferLength, // request len
            controlBlock,                                        // echo ctrl
            CompleteEchoRequest                                  // cmplt rtn
            );

        if (ipStatus == IP_SUCCESS) {
            ntStatus = STATUS_PENDING;
        } else {
            //
            // An internal error of some kind occurred. Complete the
            // request.
            //
            CompleteEchoRequest(
                controlBlock,
                ipStatus,
                &UnspecifiedAddr,
                0,
                NULL,
                0
                );

            //
            // The NT ioctl was successful, even if the request failed. The
            // request status was passed back in the first reply block.
            //
            ntStatus = STATUS_SUCCESS;
        }

        return(ntStatus);
    }

    //
    // Irp has already been cancelled.
    //
    ntStatus = STATUS_CANCELLED;
    ExFreePool(pendingIrp);
    ExFreePool(controlBlock);

  echo_error:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(ntStatus);

} // DispatchEchoRequest


//* IoctlQueryInterface
//
//  Processes an IOCTL_IPV6_QUERY_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful. The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_INTERFACE *Query;
    IPV6_INFO_INTERFACE *Info;
    Interface *IF;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->Index == 0) {
        //
        // Return the index of the first interface.
        //
        Info->Query.Index = FindNextInterfaceIndex(NULL);
        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Return information about the specified interface.
        //
        IF = FindInterfaceFromIndex(Query->Index);
        if (IF == NULL) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }

        Irp->IoStatus.Information = sizeof *Info;

        //
        // Return the interface's link-level address,
        // if there is room in the user's buffer.
        //
        Info->LinkLevelAddressLength = IF->LinkAddressLength;
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof *Info + IF->LinkAddressLength) {

            RtlCopyMemory(Info + 1, IF->LinkAddress, IF->LinkAddressLength);
            Irp->IoStatus.Information += IF->LinkAddressLength;
        }

        //
        // Return miscellaneous information about the interface.
        //
        Info->SiteIndex = IF->Site;
        Info->TrueLinkMTU = IF->TrueLinkMTU;
        Info->LinkMTU = IF->LinkMTU;
        Info->CurHopLimit = IF->CurHopLimit;
        Info->BaseReachableTime = IF->BaseReachableTime;
        Info->ReachableTime = ConvertTicksToMillis(IF->ReachableTime);
        Info->RetransTimer = ConvertTicksToMillis(IF->RetransTimer);
        Info->DupAddrDetectTransmits = IF->DupAddrDetectTransmits;

        Info->Discovers = !!(IF->Flags & IF_FLAG_DISCOVERS);
        Info->Advertises = !!(IF->Flags & IF_FLAG_ADVERTISES);
        Info->Forwards = !!(IF->Flags & IF_FLAG_FORWARDS);
        if (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)
            Info->MediaConnected = 1;
        else if (IF->Flags & IF_FLAG_MEDIA_RECONNECTED)
            Info->MediaConnected = 2;
        else
            Info->MediaConnected = 0;

        //
        // Return index of the next interface (or zero).
        //
        Info->Query.Index = FindNextInterfaceIndex(IF);
        ReleaseIF(IF);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryInterface


//* IoctlQueryAddress
//
//  Processes an IOCTL_IPV6_QUERY_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ADDRESS *Query;
    IPV6_INFO_ADDRESS *Info;
    Interface *IF = NULL;
    AddressEntry *ADE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Return information about the specified interface.
    //
    IF = FindInterfaceFromIndex(Query->IF.Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address)) {
        //
        // Return the address of the first ADE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        if (IF->ADE != NULL)
            Info->Query.Address = IF->ADE->Address;
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;
    } else {
        //
        // Find the specified ADE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        for (ADE = IF->ADE; ; ADE = ADE->Next) {
            if (ADE == NULL) {
                KeReleaseSpinLock(&IF->Lock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &ADE->Address))
                break;
        }

        //
        // Return misc. information about the ADE.
        //
        Info->Type = ADE->Type;
        Info->Scope = ADE->Scope;

        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *)ADE;

            Info->DADState = NTE->DADState;
            Info->AutoConfigured = NTE->AutoConfigured;
            Info->ValidLifetime = ConvertTicksToSeconds(NTE->ValidLifetime);
            Info->PreferredLifetime = ConvertTicksToSeconds(NTE->PreferredLifetime);
            break;
        }
        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *)ADE;

            Info->MCastRefCount = MAE->MCastRefCount;
            Info->MCastFlags = MAE->MCastFlags;
            Info->MCastTimer = ConvertTicksToSeconds(MAE->MCastTimer);
            break;
        }
        }

        //
        // Return address of the next ADE (or zero).
        //
        if (ADE->Next == NULL)
            Info->Query.Address = UnspecifiedAddr;
        else
            Info->Query.Address = ADE->Next->Address;

        KeReleaseSpinLock(&IF->Lock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryAddress


//* IoctlQueryNeighborCache
//
//  Processes an IOCTL_IPV6_QUERY_NEIGHBOR_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryNeighborCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_NEIGHBOR_CACHE *Query;
    IPV6_INFO_NEIGHBOR_CACHE *Info;
    Interface *IF = NULL;
    NeighborCacheEntry *NCE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Return information about the specified interface.
    //
    IF = FindInterfaceFromIndex(Query->IF.Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address)) {
        //
        // Return the address of the first NCE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        if (IF->FirstNCE != SentinelNCE(IF))
            Info->Query.Address = IF->FirstNCE->NeighborAddress;
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        uint Now = IPv6TickCount;

        //
        // Find the specified NCE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        for (NCE = IF->FirstNCE; ; NCE = NCE->Next) {
            if (NCE == SentinelNCE(IF)) {
                KeReleaseSpinLock(&IF->Lock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &NCE->NeighborAddress))
                break;
        }

        Irp->IoStatus.Information = sizeof *Info;

        //
        // Return the neighbor's link-level address,
        // if there is room in the user's buffer.
        //
        Info->LinkLevelAddressLength = IF->LinkAddressLength;
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof *Info + IF->LinkAddressLength) {

            RtlCopyMemory(Info + 1, NCE->LinkAddress, IF->LinkAddressLength);
            Irp->IoStatus.Information += IF->LinkAddressLength;
        }

        //
        // Update the state of the NCE.
        //
        if (NCE->NDState == ND_STATE_REACHABLE) {

            if ((uint)(Now - NCE->LastReachable) > IF->ReachableTime)
                NCE->NDState = ND_STATE_STALE;
        }

        //
        // Return miscellaneous information about the NCE.
        //
        Info->IsRouter = NCE->IsRouter;
        Info->IsUnreachable = NCE->IsUnreachable;
        Info->NDState = NCE->NDState;
        Info->ReachableTimer = ConvertTicksToMillis(IF->ReachableTime -
                                   (Now - NCE->LastReachable));

        //
        // Return address of the next NCE (or zero).
        //
        if (NCE->Next == SentinelNCE(IF))
            Info->Query.Address = UnspecifiedAddr;
        else
            Info->Query.Address = NCE->Next->NeighborAddress;

        KeReleaseSpinLock(&IF->Lock, OldIrql);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryNeighborCache


//* IoctlQueryRouteCache
//
//  Processes an IOCTL_IPV6_QUERY_ROUTE_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryRouteCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_CACHE *Query;
    IPV6_INFO_ROUTE_CACHE *Info;
    RouteCacheEntry *RCE;
    BindingCacheEntry *BCE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == 0) {
        //
        // Return the index and address of the first RCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RouteCache.First != SentinelRCE) {
            Info->Query.IF.Index = RouteCache.First->NTE->IF->Index;
            Info->Query.Address = RouteCache.First->Destination;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        uint Now = IPv6TickCount;

        //
        // Find the specified RCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (RCE = RouteCache.First; ; RCE = RCE->Next) {
            if (RCE == SentinelRCE) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &RCE->Destination) &&
                (Query->IF.Index == RCE->NTE->IF->Index))
                break;
        }

        //
        // Return misc. information about the RCE.
        //
        Info->Type = RCE->Type;
        Info->Flags = RCE->Flags;
        Info->Valid = (RCE->Valid == RouteCacheValidationCounter);
        Info->SourceAddress = RCE->NTE->Address;
        Info->NextHopAddress = RCE->NCE->NeighborAddress;
        Info->NextHopInterface = RCE->NCE->IF->Index;
        Info->PathMTU = RCE->PathMTU;
        if (RCE->PMTULastSet != 0) {
            uint SinceLastSet = Now - RCE->PMTULastSet;
            ASSERT((int)SinceLastSet >= 0);
            if (PATH_MTU_RETRY_TIME < SinceLastSet)
                Info->PMTUProbeTimer =
                    ConvertTicksToMillis(PATH_MTU_RETRY_TIME - SinceLastSet);
            else
                Info->PMTUProbeTimer = 0; // Fires on next packet.
        } else
            Info->PMTUProbeTimer = INFINITE_LIFETIME; // Not set.
        if (RCE->LastError != 0)
            Info->ICMPLastError = ConvertTicksToMillis(Now - RCE->LastError);
        else
            Info->ICMPLastError = 0;
        if (RCE->CareOfRCE != NULL) {
            Info->CareOfAddress = RCE->CareOfRCE->Destination;
            BCE = FindBindingCache(&RCE->Destination);
            if (BCE != NULL) {
                Info->BindingSeqNumber = BCE->BindingSeqNumber;
                Info->BindingLifetime = ConvertTicksToSeconds(BCE->BindingLifetime);
            }
        } else {
            Info->CareOfAddress = UnspecifiedAddr;
            Info->BindingSeqNumber = 0;
            Info->BindingLifetime = 0;
        }

        //
        // Return index and address of the next RCE (or zero).
        //
        if (RCE->Next == SentinelRCE) {
            Info->Query.IF.Index = 0;
        } else {
            Info->Query.IF.Index = RCE->Next->NTE->IF->Index;
            Info->Query.Address = RCE->Next->Destination;
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryRouteCache


//* IoctlCreateSecurityPolicy
//
NTSTATUS
IoctlCreateSecurityPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_CREATE_SECURITY_POLICY *CreateSP;
    IPV6_CREATE_SECURITY_POLICY_RESULT *CreateSPResult;
    SecurityPolicy *SP;        

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *CreateSP) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *CreateSPResult)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }
    
    Irp->IoStatus.Information = sizeof *CreateSPResult;

    CreateSPResult = (IPV6_CREATE_SECURITY_POLICY_RESULT *)Irp->AssociatedIrp.SystemBuffer;
    CreateSP = (IPV6_CREATE_SECURITY_POLICY *)Irp->AssociatedIrp.SystemBuffer;
    
    // Allocate memory for Security Policy.
    SP = ExAllocatePool(NonPagedPool, sizeof *SP);
    if (SP == NULL) 
    {
        CreateSPResult->Status = CREATE_MEMORY_ALLOC_ERROR;        
    }
    else
    {
        //
        // Copy CreateSP to SP.
        //      
        SP->Index = CreateSP->SPIndex;
        SP->RemoteAddr = CreateSP->RemoteAddr;
        SP->RemoteAddrData = CreateSP->RemoteAddrData;
        SP->RemoteAddrSelector = CreateSP->RemoteAddrSelector;
        SP->RemoteAddrField = CreateSP->RemoteAddrField;

        SP->LocalAddr = CreateSP->LocalAddr;
        SP->LocalAddrData = CreateSP->LocalAddrData;
        SP->LocalAddrSelector = CreateSP->LocalAddrSelector;
        SP->LocalAddrField = CreateSP->LocalAddrField;

        SP->TransportProto = CreateSP->TransportProto;
        SP->TransportProtoSelector = CreateSP->TransportProtoSelector;

        SP->RemotePort = CreateSP->RemotePort;
        SP->RemotePortData = CreateSP->RemotePortData;
        SP->RemotePortSelector = CreateSP->RemotePortSelector;
        SP->RemotePortField = CreateSP->RemotePortField;

        SP->LocalPort = CreateSP->LocalPort;
        SP->LocalPortData = CreateSP->LocalPortData;
        SP->LocalPortSelector = CreateSP->LocalPortSelector;
        SP->LocalPortField = CreateSP->LocalPortField;

        SP->SecPolicyFlag = CreateSP->IPSecAction;
        SP->IPSecSpec.Protocol = CreateSP->IPSecProtocol;
        SP->IPSecSpec.Mode = CreateSP->IPSecMode;
        SP->IPSecSpec.RemoteSecGWIPAddr = CreateSP->RemoteSecurityGWAddr;
        SP->DirectionFlag = CreateSP->Direction;
        SP->OutboundSA = NULL;
        SP->InboundSA = NULL;
        SP->PrevSABundle = NULL;
        SP->RefCount = 0; 
        SP->NestCount = 1;
        SP->IFIndex = CreateSP->SPInterface;
        
        // 
        // Check if this is an add or insert.
        //
        if(CreateSP->InsertIndex == 0)
        {
            // 
            // Add.
            //

            // Check SP Index and SA Bundle Index values.
            CreateSPResult->Status = SetSPIndexValues(SP, CreateSP->SABundleIndex);
            
            if(CreateSPResult->Status == CREATE_SUCCESS)
            {
                //
                // Check interface value.
                //
                CreateSPResult->Status = SetSPInitialInterface(SP);            
            }
            
            if(CreateSPResult->Status == CREATE_SUCCESS)
            {
                // Insert the SP entry in the Kernel.
                AddSecurityPolicy(SP);
            }
            else
            {
                // Free failed SP memory.
                ExFreePool(SP);
            }
        }
        else 
        {
            // 
            // Insert.
            //
            CreateSPResult->Status = InsertSecurityPolicy(SP, 
                CreateSP->InsertIndex, CreateSP->SPIndex, 
                CreateSP->SABundleIndex, CreateSP->SPInterface);
        }
        
        Irp->IoStatus.Status = STATUS_SUCCESS;    
    }

  Return:        
    IoCompleteRequest(Irp, IO_NO_INCREMENT);   

    return Irp->IoStatus.Status;
} // IoctlCreateSecurityPolicy


//* IoctlCreateSecurityAssociation
//
NTSTATUS
IoctlCreateSecurityAssociation(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_CREATE_SECURITY_ASSOCIATION_RESULT *CreateSAResult;
    IPV6_CREATE_SECURITY_ASSOCIATION *CreateSA; 
    SecurityAssociation *SA;    

    PAGED_CODE();

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Information = sizeof *CreateSAResult;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *CreateSA) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *CreateSAResult)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    CreateSAResult = (IPV6_CREATE_SECURITY_ASSOCIATION_RESULT *)Irp->AssociatedIrp.SystemBuffer;
    CreateSA = (IPV6_CREATE_SECURITY_ASSOCIATION *)Irp->AssociatedIrp.SystemBuffer;    
    
    // Allocate memory for Security Association.
    SA = ExAllocatePool(NonPagedPool, sizeof *SA);
    if (SA == NULL) {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        CreateSAResult->Status = CREATE_MEMORY_ALLOC_ERROR;
        goto Return;
    }

    //
    // Copy CreateSA to SA.
    //    
    SA->SPI = CreateSA->SPI;
    SA->SequenceNum = 0;
    SA->SADestAddr = CreateSA->SADestAddr;
    SA->DestAddr = CreateSA->DestAddr;    
    SA->SrcAddr = CreateSA->SrcAddr;
    SA->TransportProto = CreateSA->TransportProto;
    SA->DestPort = CreateSA->DestPort;
    SA->SrcPort = CreateSA->SrcPort;
    SA->DirectionFlag = CreateSA->Direction;   
    SA->RefCount = 0;

    SA->AlgorithmId = CreateSA->AlgorithmId;
    SA->KeyLength = AlgorithmTable[SA->AlgorithmId].KeySize;
    SA->RawKeyLength = CreateSA->RawKeySize;
    
    // Allocate memory for raw key.
    SA->RawKey = ExAllocatePool(NonPagedPool, SA->RawKeyLength);
    if (SA->RawKey == NULL) {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        CreateSAResult->Status = CREATE_MEMORY_ALLOC_ERROR;
        // Free SA
        ExFreePool(SA);
        goto Return;
    }   
    
    // Copy raw key to SA.
    memcpy((uchar *)SA->RawKey, (uchar *)CreateSA->RawKey, SA->RawKeyLength);

#ifdef IPSEC_DEBUG
    DbgPrint("SA %d RawKey: ", CreateSA->SAIndex);
    DumpKey(SA->RawKey, SA->RawKeyLength);
#endif

    // Allocate memory for preped key.
    SA->Key = ExAllocatePool(NonPagedPool, SA->KeyLength);
    if (SA->Key == NULL) {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        CreateSAResult->Status = CREATE_MEMORY_ALLOC_ERROR;
        // Free SA
        ExFreePool(SA);
        goto Return;
    }

    // Prepare the manual key.
    AlgorithmTable[SA->AlgorithmId].PrepareKey(CreateSA->RawKey, 
        CreateSA->RawKeySize, SA->Key);

    CreateSAResult->Status = SetSAIndexValues(SA, CreateSA->SAIndex, CreateSA->SecPolicyIndex);

    if(CreateSAResult->Status == CREATE_SUCCESS)
    {
        // Create SA entry in Kernel.
        InsertSecurityAssociation(SA);
    }
    else
    {
        // Free failed SA memory.
        ExFreePool(SA);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);    

    return Irp->IoStatus.Status;
} // IoctlCreateSecurityAssociation


//* IoctlQuerySecurityPolicyList
//
NTSTATUS
IoctlQuerySecurityPolicyList(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_POLICY_LIST *Query;
    IPV6_INFO_SECURITY_POLICY_LIST *Info;
    SecurityPolicy *SP;   
    Interface *IF = NULL;
    KIRQL OldIrql; 
    
    PAGED_CODE();

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Information = sizeof *Info;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;

    if (Query->SPInterface != 0) {
        // Find the Interface.
        IF = FindInterfaceFromIndex(Query->SPInterface);
        if (IF == NULL) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            goto Return;
        }
    }   

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // 
    // Get the correct SP to start the query.
    // Have choice between: All SPs, Common SPs, and Interface Specific SPs.
    //
    if (Query->Index == 0) {      
        // 
        // All SPs or per-interface SPs?
        //
        if (IF == NULL)
            SP = GetFirstSecPolicy();
        else
            SP = IF->FirstSP;

        if (SP == NULL) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
            goto Return;
        }                       

        Info->SPIndex = SP->Index; 
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto Return;
    }
    
    // Get the specific SP.
    SP = FindSecurityPolicyMatch(Query->Index);
    if (SP == NULL) {
        // The SA does not exist.
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
        goto Return;
    }  

    //
    // Get the next index to query.
    //
    if (IF == NULL) {
        // 
        // Global list.
        //       
        if (SP->Next == NULL) {
            // Next SP for this interface is NULL so no more SPs.
            Info->NextSPIndex = 0;
        }
        else {
            // Next SP for the interface.
            Info->NextSPIndex = SP->Next->Index;
        }
    }
    else {
        //
        // Interface list.
        //           
        if (SP->NextSecPolicy == NULL) {            
            // Next SP for this interface is NULL so no more SPs.
            Info->NextSPIndex = 0;
        }
        else {
            // Next SP for the interface.
            Info->NextSPIndex = SP->NextSecPolicy->Index;
        }
    }

    //
    // Copy SP to Info.
    //    
    Info->SPIndex = SP->Index;

    Info->RemoteAddr = SP->RemoteAddr;
    Info->RemoteAddrData = SP->RemoteAddrData;
    Info->RemoteAddrSelector = SP->RemoteAddrSelector;
    Info->RemoteAddrField = SP->RemoteAddrField;
    
    Info->LocalAddr = SP->LocalAddr;
    Info->LocalAddrData = SP->LocalAddrData;
    Info->LocalAddrSelector = SP->LocalAddrSelector;
    Info->LocalAddrField = SP->LocalAddrField;
    
    Info->TransportProto = SP->TransportProto;
    Info->TransportProtoSelector = SP->TransportProtoSelector;
    
    Info->RemotePort = SP->RemotePort;
    Info->RemotePortData = SP->RemotePortData;
    Info->RemotePortSelector = SP->RemotePortSelector;
    Info->RemotePortField = SP->RemotePortField;
    
    Info->LocalPort = SP->LocalPort;
    Info->LocalPortData = SP->LocalPortData;
    Info->LocalPortSelector = SP->LocalPortSelector;
    Info->LocalPortField = SP->LocalPortField;

    Info->IPSecProtocol = SP->IPSecSpec.Protocol;
    Info->IPSecMode = SP->IPSecSpec.Mode;
    Info->RemoteSecurityGWAddr = SP->IPSecSpec.RemoteSecGWIPAddr;
    Info->Direction = SP->DirectionFlag;
    Info->IPSecAction = SP->SecPolicyFlag;
    Info->SABundleIndex = GetSecPolicyIndex(SP->SABundle);  
    Info->SPInterface = SP->IFIndex;

    Irp->IoStatus.Status = STATUS_SUCCESS;

  Return:
    if (IF != NULL)
        ReleaseIF(IF);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Irp->IoStatus.Status;
} // IoctlQuerySecurityPolicyList

//* IoctlDeleteSecurityPolicy
//
NTSTATUS
IoctlDeleteSecurityPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_POLICY_LIST *Query;
    IPV6_INFO_SECURITY_POLICY_LIST *Info;
    SecurityPolicy *SP;      
    KIRQL OldIrql; 
    
    PAGED_CODE();
    
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Information = sizeof *Info;

    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {        
        goto Return;
    }
    
    Query = (IPV6_QUERY_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;
    
    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    if(Query->Index == 0)
    {        
        SecurityPolicy *NextSP = NULL;
        
        // Use this for the SP count.
        Info->SABundleIndex = 0;
        
        SP = GetFirstSecPolicy();        
        
        while (SP != NULL)        
        {
            NextSP = SP->Next;
            Info->SPIndex = SP->Index;             
            
            if(DeleteSP(SP) == FALSE)
            {     
                Info->NextSPIndex = 0;
                break;
            }
            
            Info->NextSPIndex = 1;
            Info->SABundleIndex++;
            SP = NextSP;
        }          
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    else
    {
        Info->SABundleIndex = 1;

        // Get the specific SP.
        SP = FindSecurityPolicyMatch(Query->Index);
        if(SP == NULL) 
        {
            // The SP does not exist.            
            goto Return;
        } 
        
        // Remove the SP.  
        if(DeleteSP(SP) == FALSE)
        {
            // The SP does not exist.            
            goto Return;
        }

        Info->SPIndex = SP->Index;  
        Info->NextSPIndex = 1;
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    
Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Irp->IoStatus.Status;
}


//* IoctlQuerySecurityAssociationList
//
NTSTATUS
IoctlQuerySecurityAssociationList(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST *Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST *Info;
    SecurityAssociation *SA;   
    KIRQL OldIrql;    
    
    PAGED_CODE();

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Information = sizeof *Info;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);    

    if (Query->Index == 0) 
    {      
        // 
        // Just getting first SA so user program knows what SP index to query.
        //
        // Get first SP.
        SA = GetFirstSecAssociation();
        if(SA == NULL) 
        {
            // No SPs exist which is expected for the first time.
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
            goto Return;
        } 

        // Current SP to output.
        Info->SAIndex = SA->Index; 
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto Return;
    }  
    else
    {
        // 
        // Actually querying the SA information now.
        //
        // Get the specific SA.
        SA = FindSecurityAssociationMatch(Query->Index);
        if(SA == NULL) 
        {
            // The SA does not exist.
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
            goto Return;
        }
    }   

    if(SA->Next == NULL)
    {
        // Next SP for this interface is NULL so no more SPs.
        Info->NextSAIndex = 0;
    }
    else
    {
        // Next SP for the interface.
        Info->NextSAIndex = SA->Next->Index;
    }

    //
    // Copy SA to Info.
    //
    Info->SAIndex = SA->Index;
    Info->SPI = SA->SPI;
    Info->SADestAddr = SA->SADestAddr;
    Info->DestAddr = SA->DestAddr;
    Info->SrcAddr = SA->SrcAddr;
    Info->TransportProto = SA->TransportProto;
    Info->DestPort = SA->DestPort;
    Info->SrcPort = SA->SrcPort;
    Info->Direction = SA->DirectionFlag;
    Info->SecPolicyIndex = GetSecPolicyIndex(SA->SecPolicy);
    Info->AlgorithmId = SA->AlgorithmId;

    Irp->IoStatus.Status = STATUS_SUCCESS;

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Irp->IoStatus.Status;
} // IoctlQuerySecurityAssociationList

//* IoctlDeleteSecurityAssociation
//
NTSTATUS
IoctlDeleteSecurityAssociation(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST *Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST *Info;
    SecurityAssociation *SA;   
    KIRQL OldIrql;    
    
    PAGED_CODE();

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Information = sizeof *Info;

    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {             
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;    

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);    

    if(Query->Index == 0)
    {
        SecurityAssociation *NextSA = NULL;

        // Use this for the SA count.
        Info->SecPolicyIndex = 0;

        SA = GetFirstSecAssociation();        

        while (SA != NULL)        
        {
            NextSA = SA->Next;

            Info->SAIndex = SA->Index;            

            if(DeleteSA(SA) == FALSE)
            {                              
                Info->NextSAIndex = 0;
                break;
            }            
            
            Info->NextSAIndex = 1;
            Info->SecPolicyIndex++;            
            SA = NextSA;
        }        
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    else
    {
        Info->SecPolicyIndex = 1;

        // Get the specific SA.
        SA = FindSecurityAssociationMatch(Query->Index);
        if(SA == NULL) 
        {
            // The SA does not exist.                       
            goto Return;
        }  
        
        // Remove the SA.  
        if (DeleteSA(SA) == FALSE)
        {            
            goto Return;
        }

        Info->NextSAIndex = 1;
        Info->SAIndex = SA->Index;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }    

Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Irp->IoStatus.Status;
}

//* IoctlQueryRouteTable
//
//  Processes an IOCTL_IPV6_QUERY_ROUTE_TABLE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryRouteTable(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_TABLE *Query;
    IPV6_INFO_ROUTE_TABLE *Info;
    RouteTableEntry *RTE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->Neighbor.IF.Index == 0) {
        //
        // Return the prefix and neighbor of the first RTE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if ((RTE = RouteTable.First) != NULL) {
            Info->Query.Prefix = RTE->Prefix;
            Info->Query.PrefixLength = RTE->PrefixLength;
            if (!IsOnLinkRTE(RTE)) {
                Info->Query.Neighbor.IF.Index = RTE->NCE->IF->Index;
                Info->Query.Neighbor.Address = RTE->NCE->NeighborAddress;
            } else {
                Info->Query.Neighbor.IF.Index = RTE->IF->Index;
                Info->Query.Neighbor.Address = UnspecifiedAddr;
            }
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Find the specified RTE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (RTE = RouteTable.First; ; RTE = RTE->Next) {
            if (RTE == NULL) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Prefix, &RTE->Prefix) &&
                (Query->PrefixLength == RTE->PrefixLength) &&
                (IsOnLinkRTE(RTE) ?
                 ((Query->Neighbor.IF.Index == RTE->IF->Index) &&
                  IP6_ADDR_EQUAL(&Query->Neighbor.Address,
                                 &UnspecifiedAddr)) :
                 ((Query->Neighbor.IF.Index == RTE->NCE->IF->Index) &&
                  IP6_ADDR_EQUAL(&Query->Neighbor.Address,
                                 &RTE->NCE->NeighborAddress))))
                break;
        }

        //
        // Return misc. information about the RTE.
        //
        Info->SitePrefixLength = RTE->SitePrefixLength;;
        Info->ValidLifetime = ConvertTicksToSeconds(RTE->ValidLifetime);
        Info->Preference = RTE->Preference;
        Info->Publish = !!(RTE->Flags & RTE_FLAG_PUBLISH);
        Info->Immortal = !!(RTE->Flags & RTE_FLAG_IMMORTAL);

        //
        // Return prefix and neighbor of the next RTE (or zero).
        //
        if ((RTE = RTE->Next) == NULL) {
            Info->Query.Neighbor.IF.Index = 0;
        } else {
            Info->Query.Prefix = RTE->Prefix;
            Info->Query.PrefixLength = RTE->PrefixLength;
            if (!IsOnLinkRTE(RTE)) {
                Info->Query.Neighbor.IF.Index = RTE->NCE->IF->Index;
                Info->Query.Neighbor.Address = RTE->NCE->NeighborAddress;
            }
            else {
                Info->Query.Neighbor.IF.Index = RTE->IF->Index;
                Info->Query.Neighbor.Address = UnspecifiedAddr;
            }
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryRouteTable


//* IoctlUpdateRouteTable
//
//  Processes an IOCTL_IPV6_UPDATE_ROUTE_TABLE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateRouteTable(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_INFO_ROUTE_TABLE *Info;
    Interface *IF = NULL;
    NeighborCacheEntry *NCE;
    RouteTableEntry *RTE;
    uint ValidLifetime;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Sanity check the arguments.
    //
    if ((Info->Query.PrefixLength > 128) ||
        (Info->SitePrefixLength > Info->Query.PrefixLength)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    //
    // Find the specified interface.
    //
    // We don't allow pseudo interfaces (which includes loopback),
    // except for v4-compatible addresses on the tunneling pseudo-interface.
    //
    IF = FindInterfaceFromIndex(Info->Query.Neighbor.IF.Index);
    if ((IF == NULL) ||
        (!(IF->Flags & IF_FLAG_DISCOVERS) && (IF != TunnelGetPseudoIF()))) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IF == TunnelGetPseudoIF()) {
        if (IsUnspecified(&Info->Query.Neighbor.Address)) {
            //
            // We use prefixes on-link to the tunnel pseudo-interface
            // for automatic tunneling and 6to4 tunneling.
            // There must be room for a v4 address to appear
            // after the prefix in destination addresses.
            //
            if ((Info->Query.PrefixLength > 96) ||
                (Info->Query.PrefixLength % 8 != 0)) {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_3;
                goto Return;
            }
            NCE = NULL;
        }
        else {
            //
            // We only allow v4-compatible neighbors on the tunnel
            // pseudo-interface. The link address is derived
            // from the v4-compatible address.
            //
            if (!IsV4Compatible(&Info->Query.Neighbor.Address)) {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            NCE = CreatePermanentNeighbor(IF, &Info->Query.Neighbor.Address,
                                    &Info->Query.Neighbor.Address.u.DWord[3]);
            if (NCE == NULL) {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Return;
            }
        }
    }
    else if (IsUnspecified(&Info->Query.Neighbor.Address)) {
        //
        // The prefix is on-link.
        //
        NCE = NULL;
    }
    else {
        //
        // REVIEW - Sanity check that the specified neighbor address
        // is reasonably on-link to the specified interface?
        //
        if (!IsUniqueAddress(&Info->Query.Neighbor.Address) ||
            IsLoopback(&Info->Query.Neighbor.Address)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
            goto Return;
        }

        //
        // Find or create the specified neighbor.
        //
        NCE = FindOrCreateNeighbor(IF, &Info->Query.Neighbor.Address);
        if (NCE == NULL) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Return;
        }
    }

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);

    //
    // Create/update the specified route.
    //
    RouteTableUpdate(IF, NCE,
                     &Info->Query.Prefix,
                     Info->Query.PrefixLength,
                     Info->SitePrefixLength,
                     ValidLifetime, Info->Preference,
                     Info->Publish, Info->Immortal);
    if (NCE != NULL)
        ReleaseNCE(NCE);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlUpdateRouteTable

//* IoctlUpdateAddress
//
//  Processes an IOCTL_IPV6_UPDATE_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_UPDATE_ADDRESS *Info;
    Interface *IF = NULL;
    uint ValidLifetime, PreferredLifetime;
    KIRQL OldIrql;
    int rc;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_UPDATE_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    //
    // We only support unicast and anycast addresses here.
    // Use socket apis for multicast addresses.
    //
    if ((Info->Type != ADE_UNICAST) &&
        (Info->Type != ADE_ANYCAST)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_4;
        goto Return;
    }

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromIndex(Info->Query.IF.Index);
    if ((IF == NULL) || (IF == &LoopInterface)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);
    PreferredLifetime = ConvertSecondsToTicks(Info->PreferredLifetime);

    if (PreferredLifetime > ValidLifetime) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
        goto Return;
    }

    //
    // For anycast addresses, only zero and infinite lifetimes allowed.
    //
    if (Info->Type == ADE_ANYCAST) {
        if ((ValidLifetime != PreferredLifetime) ||
            ((ValidLifetime != 0) &&
             (ValidLifetime != INFINITE_LIFETIME)) ||
            (Info->AutoConfigured != FALSE)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
            goto Return;
        }
    }

    //
    // Sanity check the address.
    //
    if (IsMulticast(&Info->Query.Address) ||
        IsUnspecified(&Info->Query.Address) ||
        IsLoopback(&Info->Query.Address)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    //
    // Create/update/delete the address.
    // We must go to DPC level for AddrConfUpdate.
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    if (Info->Type == ADE_ANYCAST) {
        if (Info->ValidLifetime == 0)
            rc = FindAndDeleteAAE(IF, &Info->Query.Address);
        else
            rc = FindOrCreateAAE(IF, &Info->Query.Address, NULL);
    }
    else {
        rc = AddrConfUpdate(IF, &Info->Query.Address,
                            (uchar) (Info->AutoConfigured != FALSE),
                            ValidLifetime, PreferredLifetime,
                            TRUE, NULL);
    }
    KeLowerIrql(OldIrql);

    Irp->IoStatus.Status = rc ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlUpdateAddress

//* IoctlQueryBindingCache
//
//  Processes an IOCTL_IPV6_QUERY_BINDING_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryBindingCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_BINDING_CACHE *Query;
    IPV6_INFO_BINDING_CACHE *Info;
    BindingCacheEntry *BCE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_BINDING_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_BINDING_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (IsUnspecified(&Query->HomeAddress)) {
        //
        // Return the home address of the first BCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (BindingCache != NULL) {
            Info->Query.HomeAddress = BindingCache->HomeAddr;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Find the specified BCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (BCE = BindingCache; ; BCE = BCE->Next) {
            if (BCE == NULL) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->HomeAddress, &BCE->HomeAddr)) 
                break;
        }

        //
        // Return misc. information about the BCE.
        //
        Info->HomeAddress = BCE->HomeAddr;
        Info->CareOfAddress = BCE->CareOfRCE->Destination;
        Info->BindingSeqNumber = BCE->BindingSeqNumber;
        Info->BindingLifetime = ConvertTicksToSeconds(BCE->BindingLifetime);

        //
        // Return home address of the next BCE (or Unspecified).
        //
        if (BCE->Next == NULL) {
            Info->Query.HomeAddress = UnspecifiedAddr;
        } else {
            Info->Query.HomeAddress = BCE->Next->HomeAddr;
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQueryBindingCache


//* IoctlControlInterface
//
//  Processes an IOCTL_IPV6_CONTROL_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlControlInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_CONTROL_INTERFACE *Info;
    Interface *IF;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_CONTROL_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromIndex(Info->Query.Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (Info->LinkMTU != 0)
        UpdateLinkMTU(IF, Info->LinkMTU);

    if (Info->SiteIndex != 0) {
        //
        // For now, only allow interfaces that participate
        // in neighbor discovery to belong to a site.
        //
        if (!(IF->Flags & IF_FLAG_DISCOVERS)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }

        //
        // REVIEW - Are there any locking issues here?
        //
        IF->Site = Info->SiteIndex;
        InvalidateRouteCache();
    }

    Irp->IoStatus.Status =
        ControlInterface(IF, Info->Advertises, Info->Forwards);

    ReleaseIF(IF);

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlControlInterface


//* IoctlDeleteInterface
//
//  Processes an IOCTL_IPV6_DELETE_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlDeleteInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_INTERFACE *Info;
    Interface *IF;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromIndex(Info->Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if ((IF == TunnelGetPseudoIF()) || (IF == &LoopInterface)) {
        //
        // Can not delete the loopback or tunnel interfaces.
        //
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
    }
    else {
        //
        // This will disable the interface, so it will effectively
        // disappear. When the last ref is gone it will be freed.
        //
        DestroyIF(IF);
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    ReleaseIF(IF);

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlDeleteInterface


//* IoctlFlushNeighborCache
//
//  Processes an IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlFlushNeighborCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_NEIGHBOR_CACHE *Query;
    Interface *IF;
    IPv6Addr *Address;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromIndex(Query->IF.Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address))
        Address = NULL;
    else
        Address = &Query->Address;

    FlushNeighborCache(IF, Address);
    ReleaseIF(IF);
    Irp->IoStatus.Status = STATUS_SUCCESS;

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlFlushNeighborCache


//* IoctlFlushRouteCache
//
//  Processes an IOCTL_IPV6_FLUSH_ROUTE_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlFlushRouteCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_CACHE *Query;
    Interface *IF;
    IPv6Addr *Address;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == 0) {
        IF = NULL;
    }
    else {
        //
        // Find the specified interface.
        //
        IF = FindInterfaceFromIndex(Query->IF.Index);
        if (IF == NULL) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }
    }

    if (IsUnspecified(&Query->Address))
        Address = NULL;
    else
        Address = &Query->Address;

    FlushRouteCache(IF, Address);
    if (IF != NULL)
        ReleaseIF(IF);
    Irp->IoStatus.Status = STATUS_SUCCESS;

  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlFlushRouteCache

//* IoctlSetMobilitySecurity
//
//  Processes an IOCTL_IPV6_SET_MOBILITY_SECURITY request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlSetMobilitySecurity(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_SET_MOBILITY_SECURITY *Set;
    uint OldMobilitySecurity;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Set) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Set = (IPV6_SET_MOBILITY_SECURITY *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Save the current value for use below.
    //
    OldMobilitySecurity = MobilitySecurity;

    //
    // If requested, set mobility security.
    // But don't forget parameter validation.
    //
    if (Set->MobilitySecurity != MOBILITY_SECURITY_QUERY) {
        if ((Set->MobilitySecurity == MOBILITY_SECURITY_ON) ||
            (Set->MobilitySecurity == MOBILITY_SECURITY_OFF)) {
            //
            // Set mobility security.
            //
            MobilitySecurity = Set->MobilitySecurity;
        }
        else {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }
    }

    //
    // Return the previous value.
    //
    Set->MobilitySecurity = OldMobilitySecurity;

    Irp->IoStatus.Information = sizeof *Set;
    Irp->IoStatus.Status = STATUS_SUCCESS;

Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
} // IoctlSetMobilitySecurity.

//* SortDestAddrs
//
//  Sort the destination addresses as follows:
//      link-local
//      site-local
//      global/native
//      v4-compatible
//      v4-mapped
//
//  Also, remove any site-local addresses IF
//  a) there are global addresses AND
//  b) none of the global addresses match any site-prefixes
//
//  We rewrite the addresses in place, possibly
//  decreasing the number of addresses.
//
void
SortDestAddrs(TDI_ADDRESS_IP6 *Addrs, uint *pNumAddrs)
{
    TDI_ADDRESS_IP6 TempAddr;
    uint NextAddr = 0;
    uint i;
    uint FirstSiteLocal, AfterSiteLocal;
    uint FirstGlobal, AfterGlobal;
    uint NumAddrs = *pNumAddrs;

    //
    // This is an inefficient sorting algorithm but
    // in practice the number of addresses will be small.
    //

    //
    // Move link-local addresses to the front.
    //
    for (i = NextAddr; i < NumAddrs; i++) {
        if (IsLinkLocal((IPv6Addr *)&Addrs[i].sin6_addr)) {
            //
            // Move to the front, if it's not already there.
            //
            if (NextAddr != i) {
                TempAddr = Addrs[i];
                RtlMoveMemory(&Addrs[NextAddr + 1],
                              &Addrs[NextAddr],
                              sizeof TempAddr * (i - NextAddr));
                Addrs[NextAddr] = TempAddr;
            }
            NextAddr++;
        }
    }

    //
    // Move site-local addresses to the front.
    //
    FirstSiteLocal = NextAddr;
    for (i = NextAddr; i < NumAddrs; i++) {
        if (IsSiteLocal((IPv6Addr *)&Addrs[i].sin6_addr)) {
            //
            // Move to the front, if it's not already there.
            //
            if (NextAddr != i) {
                TempAddr = Addrs[i];
                RtlMoveMemory(&Addrs[NextAddr + 1],
                              &Addrs[NextAddr],
                              sizeof TempAddr * (i - NextAddr));
                Addrs[NextAddr] = TempAddr;
            }
            NextAddr++;
        }
    }
    AfterSiteLocal = NextAddr;

    //
    // Move global native addresses to the front.
    //
    FirstGlobal = NextAddr;
    for (i = NextAddr; i < NumAddrs; i++) {
        if (!IsV4Compatible((IPv6Addr *)&Addrs[i].sin6_addr) &&
            !IsV4Mapped((IPv6Addr *)&Addrs[i].sin6_addr)) {
            //
            // Move to the front, if it's not already there.
            //
            if (NextAddr != i) {
                TempAddr = Addrs[i];
                RtlMoveMemory(&Addrs[NextAddr + 1],
                              &Addrs[NextAddr],
                              sizeof TempAddr * (i - NextAddr));
                Addrs[NextAddr] = TempAddr;
            }
            NextAddr++;
        }
    }
    AfterGlobal = NextAddr;

    //
    // Move v4-compatible addresses to the front.
    //
    for (i = NextAddr; i < NumAddrs; i++) {
        if (IsV4Compatible((IPv6Addr *)&Addrs[i].sin6_addr)) {
            //
            // Move to the front, if it's not already there.
            //
            if (NextAddr != i) {
                TempAddr = Addrs[i];
                RtlMoveMemory(&Addrs[NextAddr + 1],
                              &Addrs[NextAddr],
                              sizeof TempAddr * (i - NextAddr));
                Addrs[NextAddr] = TempAddr;
            }
            NextAddr++;
        }
    }

#if DBG
    //
    // Count the v4-mapped addresses so we can then ASSERT that
    // we've accounted for all of the addresses.
    //
    for (i = NextAddr; i < NumAddrs; i++) {
        if (IsV4Mapped((IPv6Addr *)&Addrs[i].sin6_addr)) {
            NextAddr++;
        }
    }

    ASSERT(NextAddr == NumAddrs);
#endif

    //
    // If there are site-local addresses,
    // determine if they should be dropped.
    //
    if (FirstSiteLocal != AfterSiteLocal) {
        uint Site;

        if (FirstGlobal != AfterGlobal) {
            for (i = FirstGlobal; i < AfterGlobal; i++) {
                Site = SitePrefixMatch((IPv6Addr *)
                                       &Addrs[i].sin6_addr);
                if (Site != 0) {
                    //
                    // We have found a global address that matches
                    // one of our site prefixes. No need to look further.
                    //
                    goto KeepSiteLocalAddresses;
                }
            }

            //
            // If none of the global addresses matched our site prefixes,
            // it is not safe to use the site-local addresses. Remove them.
            //
            RtlMoveMemory(&Addrs[FirstSiteLocal],
                          &Addrs[AfterSiteLocal],
                          ((NumAddrs - AfterSiteLocal) *
                           sizeof(TDI_ADDRESS_IP6)));
            NumAddrs -= AfterSiteLocal - FirstSiteLocal;
            *pNumAddrs = NumAddrs;
        }
        else {
            //
            // If there are no global addresses,
            // then we don't know what site identifier
            // to use for the site-local addresses.
            //
            Site = 0;

        KeepSiteLocalAddresses:
            for (i = FirstSiteLocal; i < AfterSiteLocal; i++)
                Addrs[i].sin6_scope_id = Site;
        }
    }
}

//* IoctlSortDestAddrs
//
//  Processes an IOCTL_IPV6_SORT_DEST_ADDRS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlSortDestAddrs(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_ADDRESS_IP6 *Addrs;
    uint NumAddrs;

    PAGED_CODE();

    //
    // There's no fast path here for zero/one address.
    // User-level code should avoid this ioctl in that case.
    //

    Addrs = (TDI_ADDRESS_IP6 *) Irp->AssociatedIrp.SystemBuffer;
    NumAddrs = (IrpSp->Parameters.DeviceIoControl.InputBufferLength /
                sizeof *Addrs);

    SortDestAddrs(Addrs, &NumAddrs);

    Irp->IoStatus.Information = NumAddrs * sizeof *Addrs;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
} // IoctlSortDestAddrs.

//* IoctlQuerySitePrefix
//
//  Processes an IOCTL_IPV6_QUERY_SITE_PREFIX request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQuerySitePrefix(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SITE_PREFIX *Query;
    IPV6_INFO_SITE_PREFIX *Info;
    SitePrefixEntry *SPE;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == 0) {
        //
        // Return query parameters of the first SPE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if ((SPE = SitePrefixTable) != NULL) {
            Info->Query.Prefix = SPE->Prefix;
            Info->Query.PrefixLength = SPE->SitePrefixLength;
            Info->Query.IF.Index = SPE->IF->Index;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Find the specified SPE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (SPE = SitePrefixTable; ; SPE = SPE->Next) {
            if (SPE == NULL) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Prefix, &SPE->Prefix) &&
                (Query->PrefixLength == SPE->SitePrefixLength) &&
                (Query->IF.Index == SPE->IF->Index))
                break;
        }

        //
        // Return misc. information about the SPE.
        //
        Info->ValidLifetime = ConvertTicksToSeconds(SPE->ValidLifetime);

        //
        // Return query parameters of the next SPE (or zero).
        //
        if ((SPE = SPE->Next) == NULL) {
            Info->Query.IF.Index = 0;
        } else {
            Info->Query.Prefix = SPE->Prefix;
            Info->Query.PrefixLength = SPE->SitePrefixLength;
            Info->Query.IF.Index = SPE->IF->Index;
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlQuerySitePrefix


//* IoctlUpdateSitePrefix
//
//  Processes an IOCTL_IPV6_UPDATE_SITE_PREFIX request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateSitePrefix(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_INFO_SITE_PREFIX *Info;
    Interface *IF = NULL;
    SitePrefixEntry *SPE;
    uint ValidLifetime;
    KIRQL OldIrql;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Sanity check the arguments.
    //
    if (Info->Query.PrefixLength > 128) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromIndex(Info->Query.IF.Index);
    if (IF == NULL) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);

    //
    // Create/update the specified site prefix.
    //
    SitePrefixUpdate(IF,
                     &Info->Query.Prefix,
                     Info->Query.PrefixLength,
                     ValidLifetime);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;

} // IoctlUpdateSitePrefix
