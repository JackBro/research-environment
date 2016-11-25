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
// General IPv6 initialization code lives here.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "route.h"
#include "icmp.h"
#include "neighbor.h"
#include <tdiinfo.h>
#include <tdi.h>
#include <tdikrnl.h>
#include "alloca.h"
#include "security.h"
#include "mld.h"

//
// Useful IPv6 Address Constants.
//
IPv6Addr UnspecifiedAddr = { 0 };
IPv6Addr LoopbackAddr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllNodesOnNodeAddr = {0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllNodesOnLinkAddr = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllRoutersOnLinkAddr = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
IPv6Addr LinkLocalPrefix = {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint MulticastScopes[] = {
    ADE_NODE_LOCAL,
    ADE_LINK_LOCAL,
    ADE_SITE_LOCAL,
    ADE_ORG_LOCAL,
    ADE_GLOBAL
};

ULONG DefaultCurHopLimit;

//
// Timer variables.
//
KTIMER IPv6Timer;
KDPC IPv6TimeoutDpc;
int IPv6TimerStarted = FALSE;

NDIS_HANDLE IPv6PacketPool, IPv6BufferPool;


//
// The NetTableListLock may be acquired while holding an interface lock.
//
NetTableEntry *NetTableList;  // Global list of NTEs.
KSPIN_LOCK NetTableListLock;  // Lock protecting this list.

//
// The IFListLock may be acquired while holding an interface lock.
//
KSPIN_LOCK IFListLock;     // Lock protecting this list.
Interface *IFList = NULL;  // List of interfaces active.

//
// Used to assign indices to interfaces.
// The IFListLock protects this variable.
//
uint NextIFIndex = 1;


//* AddNTEToInterface
//
//  Adds an NTE to an Interface's list of ADEs.
//
//  Called with the interface already locked.
//
void
AddNTEToInterface(Interface *IF, NetTableEntry *NTE)
{
    //
    // The NTE holds a reference for the interface,
    // so anyone with a reference for the NTE
    // can safely dereference NTE->IF.
    //
    AddRefIF(IF);

    NTE->IF = IF;
    NTE->Next = IF->ADE;
    IF->ADE = (AddressEntry *)NTE;
}


typedef struct SynchronizeMulticastContext {
    WORK_QUEUE_ITEM WQItem;
    Interface *IF;
} SynchronizeMulticastContext;

//* SynchronizeMulticastAddresses
//
//  Synchronize the interface's list of link-layer multicast addresses
//  with the link's knowledge of those addresses.
//
//  Callable from thread context, not from DPC context.
//  Called with no locks held.
//
void
SynchronizeMulticastAddresses(void *Context)
{
    SynchronizeMulticastContext *smc = (SynchronizeMulticastContext *) Context;
    Interface *IF = smc->IF;
    void *LinkAddresses;
    LinkLayerMulticastAddress *MCastAddr;
    uint NumKeep, NumDeleted, NumAdded;
    uint i;
    NDIS_STATUS Status;
    KIRQL OldIrql;

    ExFreePool(smc);

    //
    // First acquire the heavy-weight lock used to serialize
    // SetMCastAddrList operations.
    //
    KeWaitForSingleObject(&IF->MCastLock, Executive, KernelMode,
                          FALSE, NULL);

    //
    // Second acquire the lock that protects the interface,
    // so we can examine IF->MCastAddresses et al.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    //
    // If this interface is going away, do nothing.
    //
    if (IsDisabledIF(IF)) {
        KdPrint(("SynchronizeMulticastContext(IF %x)"
                 " - disabled (%u refs)\n", IF, IF->RefCnt));
        goto ErrorExit;
    }

    //
    // Allocate sufficient space for the link addresses
    // that we will pass to SetMCastAddrList.
    //
    LinkAddresses = ExAllocatePool(NonPagedPool,
                                   IF->MCastAddrNum * IF->LinkAddressLength);
    if (LinkAddresses == NULL) {
        KdPrint(("SynchronizeMulticastContext(IF %x) - no pool\n", IF));
        goto ErrorExit;
    }

    //
    // Make a pass through the address array, constructing
    // LinkAddresses. Deleted addresses go at the end.
    //
    NumDeleted = 0;
    MCastAddr = IF->MCastAddresses;
    for (i = 0; i < IF->MCastAddrNum; i++) {
        uint Position;

        if (MCastAddr->RefCnt == 0) {
            //
            // This address is being deleted. Put it at the end.
            //
            Position = IF->MCastAddrNum - ++NumDeleted;
        }
        else {
            //
            // Copy this link address to the front of the array.
            //
            Position = i - NumDeleted;
        }

        RtlCopyMemory(((uchar *)LinkAddresses +
                       Position * IF->LinkAddressLength),
                      MCastAddr->LinkAddress,
                      IF->LinkAddressLength);

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)(MCastAddr + 1) + IF->LinkAddressLength);
    }

    //
    // Remember if we are adding any new addresses.
    //
    NumAdded = IF->MCastAddrNew;
    IF->MCastAddrNew = 0;
    NumKeep = IF->MCastAddrNum - NumAdded - NumDeleted;

    //
    // Remove any unreferenced addresses.
    //
    if (NumDeleted != 0) {
        LinkLayerMulticastAddress *NewMCastAddresses;
        LinkLayerMulticastAddress *NewMCastAddr;
        LinkLayerMulticastAddress *MCastAddrMark;
        LinkLayerMulticastAddress *NextMCastAddr;
        uint Length;

        if (NumDeleted == IF->MCastAddrNum) {
            //
            // None left.
            //
            NewMCastAddresses = NULL;
        }
        else {
            NewMCastAddresses = ExAllocatePool(NonPagedPool,
                ((IF->MCastAddrNum - NumDeleted) *
                 (sizeof *MCastAddr + IF->LinkAddressLength)));
            if (NewMCastAddresses == NULL) {
                KdPrint(("SynchronizeMulticastContext(IF %x)"
                         " - no pool\n", IF));
                ExFreePool(LinkAddresses);
                goto ErrorExit;
            }

            //
            // Copy the addresses that are still referenced
            // to the new array. Normally there will only be
            // one unreferenced address, so it's faster to search
            // for it and then copy the elements before and after.
            // Of course there might be multiple unreferenced addresses.
            //
            NewMCastAddr = NewMCastAddresses;
            MCastAddrMark = IF->MCastAddresses;
            for (i = 0, MCastAddr = IF->MCastAddresses;
                 i < IF->MCastAddrNum;
                 i++, MCastAddr = NextMCastAddr) {

                NextMCastAddr = (LinkLayerMulticastAddress *)
                     ((uchar *)(MCastAddr + 1) + IF->LinkAddressLength);

                if (MCastAddr->RefCnt == 0) {
                    if (MCastAddrMark < MCastAddr) {
                        Length = (uchar *)MCastAddr - (uchar *)MCastAddrMark;
                        RtlCopyMemory(NewMCastAddr, MCastAddrMark, Length);
                        NewMCastAddr = (LinkLayerMulticastAddress *)
                            ((uchar *)NewMCastAddr + Length);
                    }
                    MCastAddrMark = NextMCastAddr;
                }
            }

            if (MCastAddrMark < MCastAddr) {
                Length = (uchar *)MCastAddr - (uchar *)MCastAddrMark;
                RtlCopyMemory(NewMCastAddr, MCastAddrMark, Length);
            }
        }

        ExFreePool(IF->MCastAddresses);
        IF->MCastAddresses = NewMCastAddresses;
        IF->MCastAddrNum -= NumDeleted;
    }

    //
    // We have constructed the LinkAddresses array from the interface.
    // Before we can call SetMCastAddrList, we must drop the interface lock.
    // We still hold the heavy-weight MCastLock, so multiple SetMCastAddrList
    // calls are properly serialized.
    //
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Pass the multicast link addresses down to the link layer,
    // if there's actually anything changed.
    //
    if ((NumAdded == 0) && (NumDeleted == 0)) {
        //
        // Can happen if there are races between worker threads,
        // but should be rare.
        //
        KdPrint(("SynchronizeMulticastAddresses - noop?\n"));
    }
    else {
        Status = (*IF->SetMCastAddrList)(IF->LinkContext, LinkAddresses,
                                         NumKeep, NumAdded, NumDeleted);
        if (Status != NDIS_STATUS_SUCCESS) {
            KdPrint(("SynchronizeMulticastAddresses(%x) -> %x\n", IF, Status));
        }
    }

    KeReleaseMutex(&IF->MCastLock, FALSE);
    ExFreePool(LinkAddresses);
    ReleaseIF(IF);
    return;

  ErrorExit:
    KeReleaseSpinLock(&IF->Lock, OldIrql);
    KeReleaseMutex(&IF->MCastLock, FALSE);
    ReleaseIF(IF);
}

//* DeferSynchronizeMulticastAddresses
//
//  Because SynchronizeMulticastAddresses can only be called
//  from a thread context with no locks held, this function
//  provides a way to defer a call to SynchronizeMulticastAddresses
//  when running at DPC level.
//
//  Called with the interface lock held.
//
void
DeferSynchronizeMulticastAddresses(Interface *IF)
{
    SynchronizeMulticastContext *smc;

    smc = ExAllocatePool(NonPagedPool, sizeof *smc);
    if (smc == NULL) {
        KdPrint(("DeferSynchronizeMulticastAddresses - no pool\n"));
        return;
    }

    ExInitializeWorkItem(&smc->WQItem, SynchronizeMulticastAddresses, smc);
    smc->IF = IF;
    AddRefIF(IF);
    IF->Flags &= ~IF_FLAG_MCAST_SYNC;

    ExQueueWorkItem(&smc->WQItem, CriticalWorkQueue);
}

//* AddLinkLayerMulticastAddress
//
//  Called to indicate interest in the link-layer multicast address
//  corresponding to the supplied IPv6 multicast address.
//
//  Called with the interface locked.
//
void
AddLinkLayerMulticastAddress(Interface *IF, IPv6Addr *Address)
{
    void *LinkAddress = alloca(IF->LinkAddressLength);
    LinkLayerMulticastAddress *MCastAddr;
    uint i;

    //
    // Create the link-layer multicast address
    // that corresponds to the IPv6 multicast address.
    //
    (*IF->ConvertMCastAddr)(IF->LinkContext, Address, LinkAddress);

    //
    // Check if the link-layer multicast address is already present.
    //

    MCastAddr = IF->MCastAddresses;
    for (i = 0; i < IF->MCastAddrNum; i++) {
        //
        // Have we found the link-layer address?
        //
        if (RtlCompareMemory(MCastAddr->LinkAddress, LinkAddress,
                             IF->LinkAddressLength) ==
                                        IF->LinkAddressLength)
            goto FoundMCastAddr;

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)(MCastAddr + 1) + IF->LinkAddressLength);
    }

    //
    // We must add this link-layer multicast address.
    //

    MCastAddr = ExAllocatePool(NonPagedPool,
        (IF->MCastAddrNum + 1) * (sizeof *MCastAddr + IF->LinkAddressLength));
    if (MCastAddr == NULL) {
        KdPrint(("AddLinkLayerMulticastAddress - no pool\n"));
        return;
    }

    if (IF->MCastAddresses != NULL) {
        RtlCopyMemory(MCastAddr, IF->MCastAddresses,
                      (IF->MCastAddrNum *
                       (sizeof *MCastAddr + IF->LinkAddressLength)));
        ExFreePool(IF->MCastAddresses);
    }

    IF->MCastAddresses = MCastAddr;

    MCastAddr = (LinkLayerMulticastAddress *)
        ((uchar *)MCastAddr +
         IF->MCastAddrNum * (sizeof *MCastAddr + IF->LinkAddressLength));
    MCastAddr->RefCnt = 0;
    RtlCopyMemory(MCastAddr->LinkAddress, LinkAddress, IF->LinkAddressLength);

    IF->MCastAddrNum++;
    IF->MCastAddrNew++;
    IF->Flags |= IF_FLAG_MCAST_SYNC;

  FoundMCastAddr:
    MCastAddr->RefCnt++;
}

//* DelLinkLayerMulticastAddress
//
//  Called to retract interest in the link-layer multicast address
//  corresponding to the supplied IPv6 multicast address.
//
//  Called with the interface locked.
//
void
DelLinkLayerMulticastAddress(Interface *IF, IPv6Addr *Address)
{
    void *LinkAddress = alloca(IF->LinkAddressLength);
    LinkLayerMulticastAddress *MCastAddr;
    uint i;

    //
    // Create the link-layer multicast address
    // that corresponds to the IPv6 multicast address.
    //
    (*IF->ConvertMCastAddr)(IF->LinkContext, Address, LinkAddress);

    //
    // Find the link-layer multicast address.
    // It must be present.
    //

    MCastAddr = IF->MCastAddresses;
    for (i = 0; ; i++) {
        ASSERT(i < IF->MCastAddrNum);

        //
        // Have we found the link-layer address?
        //
        if (RtlCompareMemory(MCastAddr->LinkAddress, LinkAddress,
                             IF->LinkAddressLength) ==
                                        IF->LinkAddressLength)
            break;

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)(MCastAddr + 1) + IF->LinkAddressLength);
    }

    //
    // Decrement the address's refcount.
    // If it hits zero, indicate a need to synchronize.
    //
    ASSERT(MCastAddr->RefCnt != 0);
    if (--MCastAddr->RefCnt == 0)
        IF->Flags |= IF_FLAG_MCAST_SYNC;
}


//* AddressScope
//
//  Examine an address and determines its scope.
//
ushort
AddressScope(IPv6Addr *Addr)
{
    if (IsMulticast(Addr)) {
        return Addr->u.Byte[1] & 0xf;
    } else {
        if (IsLinkLocal(Addr))
            return ADE_LINK_LOCAL;
        else if (IsSiteLocal(Addr))
            return ADE_SITE_LOCAL;
        else
            return ADE_GLOBAL;
    }
}


//* DeleteMAE
//
//  Cleanup and delete an MAE because the multicast address
//  is no longer assigned to the interface.
//  It is already removed from the interface's list.
//  
//  Called with the interface already locked.
//
void
DeleteMAE(Interface *IF, MulticastAddressEntry *MAE)
{
    int SendDoneMsg;

    KeAcquireSpinLockAtDpcLevel(&QueryListLock);
    if (!IsDisabledIF(IF) && (MAE->MCastFlags & MAE_LAST_REPORTER)) {
        //
        // We need to send a Done message.
        // Put the MAE on the QueryList with a zero timer.
        //
        if (MAE->MCastTimer == 0)
            AddToQueryList(MAE);
        else
            MAE->MCastTimer = 0;
        MAE->IF = IF; // BUGBUG - Interface reference counting.

        SendDoneMsg = TRUE;
    }
    else {
        //
        // If the MLD timer is running, remove from the query list.
        //
        if (MAE->MCastTimer != 0)
            RemoveFromQueryList(MAE);

        SendDoneMsg = FALSE;
    }
    KeReleaseSpinLockFromDpcLevel(&QueryListLock);

    //
    // Retract our interest in the corresponding
    // link-layer multicast address.
    //
    DelLinkLayerMulticastAddress(IF, &MAE->Address);

    //
    // Delete the MAE, unless we left it on the QueryList
    // pending a Done message.
    //
    if (!SendDoneMsg)
        ExFreePool(MAE);
}


//* FindAndReleaseMAE
//
//  Finds the MAE for a multicast address and releases one reference
//  for the MAE. May result in the MAE disappearing.
//
//  If successful, returns the MAE.
//  Note that it may be an invalid pointer!
//  Returns NULL on failure.
//  
//  Called with the interface already locked.
//
MulticastAddressEntry *
FindAndReleaseMAE(Interface *IF, IPv6Addr *Addr)
{
    AddressEntry **pADE;
    MulticastAddressEntry *MAE;

    pADE = FindADE(IF, Addr);
    MAE = (MulticastAddressEntry *) *pADE;
    if (MAE != NULL) {
        if (MAE->Type == ADE_MULTICAST) {
            ASSERT(MAE->MCastRefCount != 0);

            if (--MAE->MCastRefCount == 0) {
                //
                // The MAE has no more references.
                // Remove it from the Interface and delete it.
                //
                *pADE = MAE->Next;
                DeleteMAE(IF, MAE);
            }
        }
        else {
            //
            // Return NULL for error.
            //
            MAE = NULL;
        }
    }

    return MAE;
}


//* FindAndReleaseSolicitedNodeMAE
//
//  Finds the MAE for the corresponding solicited-node multicast address
//  and releases one reference for the MAE.
//  May result in the MAE disappearing.
//
//  Called with the interface already locked.
//
void
FindAndReleaseSolicitedNodeMAE(Interface *IF, IPv6Addr *Addr)
{
    if (IF->Flags & IF_FLAG_DISCOVERS) {
        IPv6Addr MCastAddr;
        MulticastAddressEntry *MAE;

        //
        // Create the corresponding solicited-node multicast address.
        //
        CreateSolicitedNodeMulticastAddress(Addr, &MCastAddr);

        //
        // Release the MAE for the solicited-node address.
        // NB: This may fail during interface shutdown
        // if we remove the solicited-node MAE before the NTE or AAE.
        //
        MAE = FindAndReleaseMAE(IF, &MCastAddr);
        ASSERT((MAE != NULL) || IsDisabledIF(IF));
    }
}


//* FindOrCreateMAE
//
//  If an MAE for the multicast address already exists,
//  just bump the reference count. Otherwise create a new MAE.
//  Returns NULL for failure.
//
//  If an NTE is supplied and an MAE is created,
//  then the MAE is associated with the NTE.
//
//  Called with the interface already locked.
//
MulticastAddressEntry *
FindOrCreateMAE(
    Interface *IF,
    IPv6Addr *Addr,
    NetTableEntry *NTE)
{
    AddressEntry **pADE;
    MulticastAddressEntry *MAE;

    //
    // Can not create a new MAE if the interface is shutting down.
    //
    if (IsDisabledIF(IF))
        return NULL;

    pADE = FindADE(IF, Addr);
    MAE = (MulticastAddressEntry *) *pADE;

    if (MAE == NULL) {
        //
        // Create a new MAE.
        //
        MAE = ExAllocatePool(NonPagedPool, sizeof(MulticastAddressEntry));
        if (MAE == NULL)
            return NULL;

        //
        // Initialize the new MAE.
        //
        if (NTE != NULL)
            MAE->NTE = NTE;
        else
            MAE->IF = IF;
        MAE->Address = *Addr;
        MAE->Type = ADE_MULTICAST;
        MAE->Scope = AddressScope(Addr);
        MAE->MCastRefCount = 0; // Incremented below.
        MAE->MCastTimer = 0;
        MAE->NextQL = NULL;

        //
        // With any luck the compiler should optimize these
        // field assignments...
        //
        if (IsMLDReportable(MAE)) {
            //
            // We should send MLD reports for this address.
            // Start by sending initial reports immediately.
            //
            MAE->MCastFlags = MAE_REPORTABLE;
            MAE->MCastCount = MLD_NUM_INITIAL_REPORTS;
            MAE->MCastSuppressLB = 0;
            MAE->MCastTimer = 1; // Immediately.
            KeAcquireSpinLockAtDpcLevel(&QueryListLock);
            AddToQueryList(MAE);
            KeReleaseSpinLockFromDpcLevel(&QueryListLock);
        }
        else {
            MAE->MCastFlags = 0;
            MAE->MCastCount = 0;
            MAE->MCastSuppressLB = 0;
            MAE->MCastTimer = 0;
        }

        //
        // Add the MAE to the interface's ADE list.
        //
        MAE->Next = NULL;
        *pADE = (AddressEntry *)MAE;

        //
        // Indicate our interest in the corresponding
        // link-layer multicast address.
        //
        AddLinkLayerMulticastAddress(IF, Addr);
    }
    else {
        ASSERT(MAE->Type == ADE_MULTICAST);
    }

    MAE->MCastRefCount++;
    return MAE;
}


//* FindOrCreateSolicitedNodeMAE
//
//  Called with a unicast or anycast address.
//
//  If an MAE for the solicited-node multicast address already exists,
//  just bump the reference count. Otherwise create a new MAE.
//  Returns TRUE for success.
//
//  Called with the interface already locked.
//
int
FindOrCreateSolicitedNodeMAE(Interface *IF, IPv6Addr *Addr)
{
    if (IF->Flags & IF_FLAG_DISCOVERS) {
        IPv6Addr MCastAddr;

        //
        // Create the corresponding solicited-node multicast address.
        //
        CreateSolicitedNodeMulticastAddress(Addr, &MCastAddr);

        //
        // Find or create an MAE for the solicited-node multicast address.
        //
        return FindOrCreateMAE(IF, &MCastAddr, NULL) != NULL;
    }
    else {
        //
        // Only interfaces that support Neighbor Discovery
        // use solicited-node multicast addresses.
        //
        return TRUE;
    }
}


//* FindOrCreateAAE
//
//  Adds an anycast address to the interface,
//  associated with the NTE.
//
//  If the interface already has the anycast address assigned,
//  then this does nothing.
//
//  Returns TRUE for success.
//
//  Called with NO locks held.
//  (Except when it is called on the loopback interface -
//  then another interface's lock may be held.)
//  Callable from DPC context, not from thread context.
//
int
FindOrCreateAAE(Interface *IF, IPv6Addr *Addr,
                NetTableEntryOrInterface *NTEorIF)
{
    AddressEntry **pADE;
    AnycastAddressEntry *AAE;
    int rc;

    if (NTEorIF == NULL)
        NTEorIF = CastFromIF(IF);

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    if (IsDisabledIF(IF)) {
        //
        // Can't create a new AAE if the interface is shutting down.
        //
        rc = FALSE;
    }
    else {
        pADE = FindADE(IF, Addr);
        AAE = (AnycastAddressEntry *) *pADE;
        if (AAE == NULL) {
            //
            // Create an AAE for the anycast address.
            //
            AAE = ExAllocatePool(NonPagedPool, sizeof(AnycastAddressEntry));
            if (AAE != NULL) {

                //
                // Initialize the new AAE.
                //
                AAE->NTEorIF = NTEorIF;
                AAE->Address = *Addr;
                AAE->Type = ADE_ANYCAST;
                AAE->Scope = AddressScope(Addr);

                //
                // Add the AAE to the interface's ADE list.
                //
                AAE->Next = NULL;
                *pADE = (AddressEntry *)AAE;

                if (IF != &LoopInterface) {
                    //
                    // Create the corresponding solicited-node
                    // multicast address MAE.
                    //
                    rc = FindOrCreateSolicitedNodeMAE(IF, Addr);

                    //
                    // Create a loopback route for this address.
                    //
                    RouteTableUpdate(IF, LoopNCE, Addr, 128, 0,
                                     INFINITE_LIFETIME, DEFAULT_LOOPBACK_PREF,
                                     FALSE, FALSE);

                    //
                    // The loop-back interface needs an ADE for this address,
                    // so that it will accept the arriving packets.
                    // We create anycast ADEs for this purpose.
                    //
                    rc = FindOrCreateAAE(&LoopInterface, Addr, NTEorIF);
                }
                else
                    rc = TRUE;
            }
            else
                rc = FALSE;
        }
        else {
            //
            // The ADE already exists -
            // just verify that it is anycast.
            //
            rc = (AAE->Type == ADE_ANYCAST);
        }

        if (IsMCastSyncNeeded(IF))
            DeferSynchronizeMulticastAddresses(IF);
    }
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    return rc;
}


//* DeleteAAE
//
//  Cleanup and delete an AAE.
//  It is already removed from the interface's list.
//
//  Called with the interface lock held.
//
void
DeleteAAE(Interface *IF, AnycastAddressEntry *AAE)
{
    int rc;

    if (IF != &LoopInterface) {
        //
        // The corresponding solicited-node address is not needed.
        //
        FindAndReleaseSolicitedNodeMAE(IF, &AAE->Address);

        //
        // The loopback route is not needed.
        //
        RouteTableUpdate(IF, LoopNCE, &AAE->Address, 128, 0,
                         0, DEFAULT_LOOPBACK_PREF,
                         FALSE, FALSE);

        //
        // The loopback interface's ADE is not needed.
        //
        rc = FindAndDeleteAAE(&LoopInterface, &AAE->Address);
        ASSERT(rc);
    }

    ExFreePool(AAE);
}


//* FindAndDeleteAAE
//
//  Deletes an anycast address from the interface.
//  Returns TRUE for success.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
int
FindAndDeleteAAE(Interface *IF, IPv6Addr *Addr)
{
    AddressEntry **pADE;
    AnycastAddressEntry *AAE;
    int rc;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    pADE = FindADE(IF, Addr);
    AAE = (AnycastAddressEntry *) *pADE;
    if (AAE != NULL) {
        if (AAE->Type == ADE_ANYCAST) {
            //
            // Delete the AAE.
            //
            *pADE = AAE->Next;
            DeleteAAE(IF, AAE);
            rc = TRUE;
        }
        else {
            //
            // This is an error - it should be anycast.
            //
            rc = FALSE;
        }
    }
    else {
        //
        // If the address already doesn't exist, then OK.
        //
        rc = TRUE;
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    return rc;
}


//* JoinGroupAtAllScopes
//
//  Join a multicast group at all scopes up to the specified scope.
//  Returns TRUE for success. (BUGBUG)
//  Called with the interface already locked.
//
int
JoinGroupAtAllScopes(Interface *IF, IPv6Addr *GroupAddr, uint MaxScope)
{
    IPv6Addr Address = *GroupAddr;
    MulticastAddressEntry *MAE;
    uint i;

    //
    // Note that we skip node-local for now.
    // Node-local is handled specially in loopback.
    //
    for (i = 1;
         ((i < sizeof MulticastScopes / sizeof MulticastScopes[0]) &&
          (MulticastScopes[i] <= MaxScope));
         i++) {

        Address.u.Byte[1] = (Address.u.Byte[1] & 0xf0) | MulticastScopes[i];
        MAE = FindOrCreateMAE(IF, &Address, NULL);
    }

    return TRUE;
}


//* LeaveGroupAtAllScopes
//
//  Leave a multicast group at all scopes.
//  Called with the interface already locked.
//
void
LeaveGroupAtAllScopes(Interface *IF, IPv6Addr *GroupAddr, uint MaxScope)
{
    IPv6Addr Address = *GroupAddr;
    MulticastAddressEntry *MAE;
    uint i;

    //
    // Note that we skip node-local for now.
    // Node-local is handled specially in loopback.
    //
    for (i = 1;
         ((i < sizeof MulticastScopes / sizeof MulticastScopes[0]) &&
          (MulticastScopes[i] <= MaxScope));
         i++) {

        Address.u.Byte[1] = (Address.u.Byte[1] & 0xf0) | MulticastScopes[i];
        MAE = FindAndReleaseMAE(IF, &Address);
        ASSERT(MAE != NULL);
    }
}


//* DestroyADEs
//
//  Destroy all AddressEntries that reference an NTE.
//
//  Called with the interface already locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
DestroyADEs(Interface *IF, NetTableEntry *NTE)
{
    AddressEntry *AnycastList = NULL;
    AddressEntry *ADE, **PrevADE;

    PrevADE = &IF->ADE;
    while ((ADE = *PrevADE) != NULL) {
        if (ADE == (AddressEntry *)NTE) {
            //
            // Remove the NTE from the list but do not
            // free the memory - that happens later.
            //
            *PrevADE = ADE->Next;
        }
        else if (ADE->NTE == NTE) {
            //
            // Remove this ADE because it references the NTE.
            //
            *PrevADE = ADE->Next;

            switch (ADE->Type) {
            case ADE_UNICAST:
                ASSERTMSG("DestroyADEs: unicast ADE?\n", FALSE);
                break;

            case ADE_ANYCAST: {
                //
                // We can't call FindAndReleaseSolicitedNodeMAE here
                // because it could screw up our list traversal.
                // So put the ADE on our temporary list and do it later.
                //
                ADE->Next = AnycastList;
                AnycastList = ADE;
                break;
            }

            case ADE_MULTICAST: {
                MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;

                DeleteMAE(IF, MAE);
                break;
            }
            }
        }
        else {
            PrevADE = &ADE->Next;
        }
    }

    //
    // Now we can safely process the anycast ADEs.
    //
    while ((ADE = AnycastList) != NULL) {
        AnycastList = ADE->Next;
        DeleteAAE(IF, (AnycastAddressEntry *)ADE);
    }
}


//* FindADE - find an ADE entry for the given interface.
//
//  If the address is assigned to the interface,
//  returns the address of the link pointing to the ADE.
//  Otherwise returns a pointer to the link (currently NULL)
//  where a new ADE should be added to extend the list.
//
//  The caller must lock the IF before calling this function.
//
AddressEntry **
FindADE(
    Interface *IF,
    IPv6Addr *Addr)
{
    AddressEntry **pADE, *ADE;

    //
    // Check if address is assigned to the interface using the
    // interface's ADE list.
    //
    // REVIEW: Change the ADE list to a more efficient data structure?
    //
    for (pADE = &IF->ADE; (ADE = *pADE) != NULL; pADE = &ADE->Next) {
        if (IP6_ADDR_EQUAL(Addr, &ADE->Address))
            break;
    }

    return pADE;
}


//* FindAddressOnInterface
//
//  If the address is assigned to the interface,
//  returns either an NTE (with ref) or an Interface
//  that should receive the address.
//  Otherwise returns NULL.
//
//  Note that the returned NTEorIF->IF MAY not equal
//  the interface argument. This happens with loopback.
//
//  Callable from DPC context, not from thread context.
//
NetTableEntryOrInterface *
FindAddressOnInterface(
    Interface *IF,
    IPv6Addr *Addr,
    ushort *AddrType)
{
    AddressEntry *ADE;
    NetTableEntryOrInterface *NTEorIF;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    ADE = *FindADE(IF, Addr);
    if (ADE != NULL) {
        if ((*AddrType = ADE->Type) == ADE_UNICAST) {
            NTEorIF = CastFromNTE((NetTableEntry *)ADE);
            goto ReturnNTE;
        }
        else {
            NTEorIF = ADE->NTEorIF;
            if (IsNTE(NTEorIF))
            ReturnNTE:
                AddRefNTE(CastToNTE(NTEorIF));
        }
    }
    else {
        NTEorIF = NULL;
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    return NTEorIF;
}

//* CreateNTE - Creates an NTE on an interface
//
//  Returns one reference for the caller.
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
NetTableEntry *
CreateNTE(Interface *IF, IPv6Addr *Address, uchar AutoConfigured,
          uint ValidLifetime, uint PreferredLifetime)
{
    NetTableEntry *NTE;
    int rc;

    //
    // Can't create a new NTE if the interface is shutting down.
    //
    if (IsDisabledIF(IF))
        return NULL;

    NTE = ExAllocatePool(NonPagedPool, sizeof(NetTableEntry));
    if (NTE == NULL)
        return NULL;

    KdPrint(("CreateNTE(IF %x, Addr %s) -> NTE %x\n",
             IF, FormatV6Address(Address), NTE));

    //
    // Initialize the NTE.
    // It has one reference held by the interface,
    // and one reference for our caller.
    //
    NTE->Address = *Address;
    NTE->Type = ADE_UNICAST;
    NTE->Scope = AddressScope(Address);
    AddNTEToInterface(IF, NTE);
    NTE->RefCnt = 2;
    NTE->AutoConfigured = AutoConfigured;

    if (IF->DupAddrDetectTransmits == 0) {
        //
        // Duplicate Address Detection on this interface is disabled.
        //
        if (ValidLifetime == 0)
            NTE->DADState = DAD_STATE_DEPRECATED;
        else
            NTE->DADState = DAD_STATE_PREFERRED;
        NTE->DADCount = NTE->DADCountLB = 0;
        NTE->DADTimer = 0;
    } else {
        //
        // Initialize for DAD.
        //
        NTE->DADState = DAD_STATE_TENTATIVE;
        NTE->DADCount = IF->DupAddrDetectTransmits;
        NTE->DADCountLB = 0;
        //
        // Send first solicit at next IPv6Timeout.
        //
        NTE->DADTimer = 1;
    }
    NTE->ValidLifetime = ValidLifetime;
    NTE->PreferredLifetime = PreferredLifetime;

    //
    // Create the corresponding solicited-node multicast address MAE.
    //
    if (!FindOrCreateSolicitedNodeMAE(IF, Address))
        goto failure;

    //
    // Create a loopback route for this address.
    //
    RouteTableUpdate(IF, LoopNCE, Address, 128, 0,
                     INFINITE_LIFETIME, DEFAULT_LOOPBACK_PREF,
                     FALSE, FALSE);

    //
    // The loop-back interface needs an ADE for this address,
    // so that it will accept the arriving packets.
    // We create anycast ADEs for this purpose.
    //
    rc = FindOrCreateAAE(&LoopInterface, Address, CastFromNTE(NTE));
    ASSERT(rc);

    //
    // Add this NTE to the front of the NetTableList.
    // 
    KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
    NTE->NextOnNTL = NetTableList;
    NetTableList = NTE;
    KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

    //
    // Note that we have created a new NTE.
    //
    InvalidateRouteCache();
    return NTE;

  failure:
    // BUGBUG - cleanup
    KdPrint(("CreateNTE failed!\n"));
    return NULL;
}

//* AddInterface
//
//  Add a new interface to the global list.
//  Also assigns the interface index.
//
void
AddInterface(Interface *IF)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);
    IF->Index = NextIFIndex++;
    IF->Next = IFList;
    IFList = IF;
    KeReleaseSpinLock(&IFListLock, OldIrql);
}


//* CreateInterface
//
//  Creates an IPv6 interface given some link-layer information.
//
//  If successful, returns a reference for the interface.
//
NDIS_STATUS
CreateInterface(LLIPBindInfo *BindInfo, void **Context)
{
    NetTableEntry *NTE;             // Current NTE being initialized.
    Interface *IF;                  // Interface being added.
    IPv6Addr Address;
    KIRQL OldIrql;

    //
    // Prevent new interfaces from being created
    // while the stack is unloading.
    //
    if (Unloading) {
        KdPrint(("CreateInterface - unloading\n"));
        return NDIS_STATUS_FAILURE;
    }

    //
    // Before doing the real work, take advantage of the link-level
    // address passed up here to re-seed our random number generator.
    //
    SeedRandom(*(uint *)BindInfo->lip_addr);

    //
    // Has the IPv6 timer been started yet?
    //
    if (InterlockedExchange((LONG *)&IPv6TimerStarted, TRUE) == FALSE) {
        LARGE_INTEGER InitialWakeUp;
        uint InitialWakeUpMillis;

        //
        // Start the timer with an initial relative expiration time and
        // also a recurring period.  The initial expiration time is
        // negative (to indicate a relative time), and in 100ns units, so
        // we first have to do some conversions.  The initial expiration
        // time is randomized to help prevent synchronization between
        // different machines.
        //
        InitialWakeUpMillis = RandomNumber(IPv6_TIMEOUT, 2*IPv6_TIMEOUT);
        KdPrint(("IPv6: InitialWakeUpMillis = %u\n", InitialWakeUpMillis));
        InitialWakeUp.QuadPart = -(LONGLONG)InitialWakeUpMillis * 10000;
        KeSetTimerEx(&IPv6Timer, InitialWakeUp, IPv6_TIMEOUT, &IPv6TimeoutDpc);
    }

    //
    // Allocate memory to hold an interface.
    //
    IF = ExAllocatePool(NonPagedPool, sizeof *IF);
    if (IF == NULL) {
        // Couldn't allocate memory.
        goto failure;
    }

    RtlZeroMemory(IF, sizeof *IF);
    IF->IF = IF;

    //
    // Start with one reference because this is an active interface.
    // And one reference for our caller.
    //
    IF->RefCnt = 2;

    //
    // Initialize interfaces first SP with the default policies.
    // BUGBUG - Locking of the SP ref count?
    //
    IF->FirstSP = DefaultSP;
    IF->FirstSP->RefCount++;  

    //
    // Return the new Interface to our caller now.
    // This makes it available to the link-layer when
    // we call CreateToken etc, before CreateInterface returns.
    //
    *Context = IF;

    KeInitializeSpinLock(&IF->Lock);
    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    IF->LinkContext = BindInfo->lip_context;
    IF->Transmit = BindInfo->lip_transmit;
    IF->CreateToken = BindInfo->lip_token;
    IF->ReadLLOpt = BindInfo->lip_rdllopt;
    IF->WriteLLOpt = BindInfo->lip_wrllopt;
    IF->ConvertMCastAddr = BindInfo->lip_mcaddr;
    IF->SetMCastAddrList = BindInfo->lip_mclist;
    IF->Close = BindInfo->lip_close;
    IF->LinkAddressLength = BindInfo->lip_addrlen;
    IF->LinkAddress = BindInfo->lip_addr;
    IF->LinkHeaderSize = BindInfo->lip_hdrsize;
    IF->TrueLinkMTU = BindInfo->lip_maxmtu;
    IF->LinkMTU = BindInfo->lip_defmtu;
    IF->LoopbackCapable = BindInfo->lip_flags & LIP_MCLOOP_FLAG;
    IF->BaseReachableTime = REACHABLE_TIME;
    IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
    IF->RetransTimer = RETRANS_TIMER;
    IF->DupAddrDetectTransmits = 1; // BUGBUG Should be configurable.
    IF->CurHopLimit = DefaultCurHopLimit;

    //
    // For now, all interfaces are considered to be in the same site.
    // BUGBUG: This should be administrator configurable.
    //
    IF->Site = 1;  // Arbitrary.

    //
    // Allow Neighbor Discovery for this interface.
    //
    IF->Flags = IF_FLAG_DISCOVERS;

    NeighborCacheInit(IF);

    //
    // Initialize so IPv6Timeout will send the first Router Solicitation.
    // BUGBUG: Should have random delay here.
    //
    IF->RSTimer = 1;

    //
    // The interface keeps track of the link-layer multicast addresses
    // that the link layer is current listening on.
    //
    KeInitializeMutex(&IF->MCastLock, 0);
    //
    // We need to get APCs while holding MCastLock,
    // so that we can get IO completions
    // for our TDI calls on 6over4 interfaces.
    // This is not a security problem because
    // only kernel worker threads use MCastLock
    // so they can't be suspended by the user.
    //
    IF->MCastLock.ApcDisable = 0;
#if 0 // not needed
    IF->MCastAddresses = NULL;
    IF->MCastAddrRefCnts = NULL;
    IF->MCastAddrNum = 0;
    IF->MCastAddrNew = 0;
#endif

    //
    // Create the all-nodes multicast addresses.
    //
    if (! JoinGroupAtAllScopes(IF, &AllNodesOnLinkAddr, ADE_LINK_LOCAL))
        goto failure;

    //
    // Create a link-local address for this interface.
    // Other addresses will be created later via stateless
    // auto-configuration.
    //
    Address = LinkLocalPrefix;
    (*IF->CreateToken)(IF->LinkContext, IF->LinkAddress, &Address);

    NTE = CreateNTE(IF, &Address, FALSE,
                    INFINITE_LIFETIME, INFINITE_LIFETIME);
    if (NTE == NULL)
        goto failure;

    //
    // REVIEW: The LinkLocalNTE field is a bad idea.
    // It does not hold a ref and it is accessed without the IF lock.
    //
    IF->LinkLocalNTE = NTE;
    ReleaseNTE(NTE);

    //
    // Create a route to link-local destinations on this interface.
    //
    RouteTableUpdate(IF, NULL, &LinkLocalPrefix, 10, 0, INFINITE_LIFETIME,
                     DEFAULT_ON_LINK_PREF, FALSE, FALSE);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Add ourselves to the front of the global interface list.
    // This also assigns the interface index.
    // This is done last so the interface is fully initialized
    // when it shows up on the list.
    //
    AddInterface(IF);

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    return NDIS_STATUS_SUCCESS;

failure:
    // BUGBUG - cleanup
    return NDIS_STATUS_FAILURE;
}


//* DestroyIF
//
//  Shuts down an interface, making the interface effectively disappear.
//  The interface will actually be freed when its last ref is gone.
//
void
DestroyIF(Interface *IF)
{
    AddressEntry *ADE;
    int WasDisabled;
    KIRQL OldIrql;

    //
    // First things first: disable the interface.
    // If it's already disabled, we're done.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    WasDisabled = IF->Flags & IF_FLAG_DISABLED;
    IF->Flags |= IF_FLAG_DISABLED;
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    if (WasDisabled) {
        KdPrint(("DestroyInterface(IF %u/%x) - already disabled?\n",
                 IF->Index, IF));
        return;
    }

    KdPrint(("DestroyInterface(IF %u/%x) -> disabled\n",
             IF->Index, IF));

    //
    // Destroy all the ADEs. Because the interface is disabled,
    // new ADEs will not subsequently be created.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    while ((ADE = IF->ADE) != NULL) {
        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *) ADE;
            DestroyNTE(IF, NTE);
            break;
        }

        case ADE_ANYCAST: {
            AnycastAddressEntry *AAE = (AnycastAddressEntry *) ADE;
            IF->ADE = AAE->Next;
            DeleteAAE(IF, AAE);
            break;
        }

        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;
            IF->ADE = MAE->Next;
            DeleteMAE(IF, MAE);
            break;
        }
        }
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Shutdown the link-layer.
    //
    if (IF->Close != NULL)
        (*IF->Close)(IF->LinkContext);

    //
    // Clean up routing associated with the interface.
    //
    RouteTableRemove(IF);

    //
    // Release the reference that the interface
    // held for itself by virtue of being active.
    //
    ReleaseIF(IF);
}


//* DestroyInterface
//
//  Called from a link layer to destroy an interface.
//
void
DestroyInterface(void *Context)
{
    Interface *IF = (Interface *) Context;

    DestroyIF(IF);
}


//* ReleaseInterface
//
//  Called from the link-layer to release its reference
//  for the interface.
//
void
ReleaseInterface(void *Context)
{
    Interface *IF = (Interface *) Context;

    ReleaseIF(IF);
}


//* UpdateLinkMTU
//
//  Update the link's MTU.
//  For example, in response to a Router Advertisement with an MTU option.
//
//  Callable from thread or DPC context.
//  Called with NO locks held.
//
void
UpdateLinkMTU(Interface *IF, uint MTU)
{
    KIRQL OldIrql;

    //
    // No need to take the interface lock to read
    // the TrueLinkMTU and LinkMTU fields.
    //
    if ((IPv6_MINIMUM_MTU <= MTU) && (MTU <= IF->TrueLinkMTU) &&
        (MTU != IF->LinkMTU)) {

        //
        // Update the LinkMTU field.
        // If the interface is advertising, then it should
        // send a new RA promptly because the RAs contain the MTU option.
        // We need the interface lock for this,
        // not for updating LinkMTU.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        IF->LinkMTU = MTU;
        if (IF->Flags & IF_FLAG_ADVERTISES) {
            //
            // Send a Router Advertisement very soon.
            //
            IF->RATimer = 1;
        }
        KeReleaseSpinLock(&IF->Lock, OldIrql);
    }
}


//* FindInterfaceFromIndex
//
//  Given the index of an interface, finds the interface.
//  Returns a reference for the interface, or
//  returns NULL if no valid interface is found.
//
//  Callable from thread or DPC context.
//
Interface *
FindInterfaceFromIndex(uint Index)
{
    Interface *IF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if (IF->Index == Index) {
            //
            // Fail to find disabled interfaces.
            //
            if (IsDisabledIF(IF))
                IF = NULL;
            else
                AddRefIF(IF);
            break;
        }
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    return IF;
}


//* FindNextInterfaceIndex
//
//  Returns the index of the next valid (not disabled) interface.
//  If the argument is NULL, returns the index of the first valid interface.
//  Returns zero if there is no next interface.
//
//  Callable from thread or DPC context.
//
uint
FindNextInterfaceIndex(Interface *IF)
{
    KIRQL OldIrql;
    uint NextIndex;

    KeAcquireSpinLock(&IFListLock, &OldIrql);

    if (IF == NULL)
        IF = IFList;
    else
        IF = IF->Next;

    while ((IF != NULL) && IsDisabledIF(IF))
        IF = IF->Next;

    if (IF == NULL)
        NextIndex = 0;
    else
        NextIndex = IF->Index;

    KeReleaseSpinLock(&IFListLock, OldIrql);

    return NextIndex;
}


#pragma BEGIN_INIT

//* IPInit - Initialize ourselves.
//
//  This routine is called during initialization from the OS-specific
//  init code. We need to check for the presence of the common xport
//  environment first.
//
int  // Returns: 0 if initialization failed, non-zero if it succeeds.
IPInit(
    WCHAR *RegKeyNameParam)
{
    NDIS_STATUS Status;
    LARGE_INTEGER Now;

    KeInitializeSpinLock(&NetTableListLock);
    KeInitializeSpinLock(&IFListLock);

    //
    // Prepare our periodic timer and its associated DPC object.
    //
    // When the timer expires, the IPv6Timeout deferred procedure
    // call (DPC) is queued.  Everything we need to do at some
    // specific frequency is driven off of this routine.
    //
    // We don't actually start the timer until an interface
    // is created. We need random seed info from the interface.
    // Plus there's no point in the overhead unless there are interfaces.
    //
    KeInitializeDpc(&IPv6TimeoutDpc, IPv6Timeout, NULL);  // No parameter.
    KeInitializeTimer(&IPv6Timer);

    //
    // Perform initial seed of our random number generator using
    // low bits of a high-precision time-since-boot.  Since this isn't
    // the greatest seed (having just booted it won't vary much), we later
    // XOR in bits from our link-level addresses as we discover them.
    //
    Now = KeQueryPerformanceCounter(NULL);
    SeedRandom(Now.LowPart ^ Now.HighPart);

    // Initialize the ProtocolSwitchTable.
    ProtoTabInit();

    //
    // Create Packet and Buffer pools for IPv6.
    // BUGBUG: Arbitrary amounts. But currently they need
    // to be very large, so we can fragment large packets.
    //
    NdisAllocatePacketPool(&Status, &IPv6PacketPool, 100,
                           sizeof(Packet6Context));
    NdisAllocateBufferPool(&Status, &IPv6BufferPool, 100);

    ReassemblyInit();

    ICMPv6Init();

    if (!IPSecInit())
    {
        return FALSE;
    }

    //
    // Start the routing module
    //
    InitRouting();

    //
    // The IPv6 timer is initialized in CreateInterface,
    // when the first real interface is created.
    // The calls below will start creating interfaces;
    // our data structures should all be initialized now.
    //

    NetTableList = LoopbackInit();
    if (NetTableList == NULL)
        return FALSE;     // Couldn't initialize loopback.

    if (!LanInit(RegKeyNameParam)) {
        ExFreePool(NetTableList);
        return FALSE;     // Couldn't initialize lan.c.
    }

    if (!TunnelInit(RegKeyNameParam)) {
        ExFreePool(NetTableList);
        return FALSE;     // Couldn't initialize 6over4.c.
    }    

    return TRUE;
}

#pragma END_INIT


//* IPUnload
//
//  Called to shutdown the IP module in preparation
//  for unloading the protocol stack.
//
void
IPUnload(void)
{
    Interface *IF;
    KIRQL OldIrql;

    //
    // Stop the periodic timer.
    //
    KeCancelTimer(&IPv6Timer);

    //
    // Call each interface's close function.
    // Note that interfaces might disappear while
    // the interface list is unlocked,
    // but new interfaces will not be created
    // and the list does not get reordered.
    //
    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        AddRefIF(IF);
        KeReleaseSpinLock(&IFListLock, OldIrql);

        DestroyIF(IF);

        KeAcquireSpinLock(&IFListLock, &OldIrql);
        ReleaseIF(IF);
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    ReleaseNCE(LoopNCE);

    NetTableCleanup();
    InterfaceCleanup();
    UnloadRouting();
}


//* GetLinkLocalAddress
//
//  Returns the interface's link-local address,
//  if it is valid.
//
//  Callable from thread or DPC context.
//
//  Returns FALSE if the link-local address is not valid.
//
int
GetLinkLocalAddress(
    Interface *IF,   // Interface for which to find an address.
    IPv6Addr *Addr)  // Where to return address found.
{
    KIRQL OldIrql;
    NetTableEntry *NTE;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    NTE = IF->LinkLocalNTE;
    if (NTE != NULL) {
        if (IsValidNTE(NTE))
            *Addr = NTE->Address;
        else
            NTE = NULL;
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);
    return (int)NTE;
}


//* AddrConfUpdate - Perform address auto-configuration.
//
//  Called when we receive a valid Router Advertisement
//  with a Prefix Information option that has the A (autonomous) bit set.
//  Or in other situations when we want to update/create an address.
//
//  Our caller is responsible for any sanity-checking of the address.
//
//  Our caller is responsible for checking that the preferred lifetime
//  is not greater than the valid lifetime.
//
//  Returns TRUE for success.
//  Will also optionally return the NTE, with a reference for the caller.
//  It is possible to succeed and return a null NTE.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
int
AddrConfUpdate(
    Interface *IF,
    IPv6Addr *Addr,
    uchar AutoConfigured,
    uint ValidLifetime,
    uint PreferredLifetime,
    int Authenticated,
    NetTableEntry **pNTE)
{
    NetTableEntry *NTE;
    int rc;

    ASSERT(!IsMulticast(Addr) && !IsLoopback(Addr) && !IsLoopback(Addr));

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    //
    // Search for an existing Net Table Entry.
    // Note that some of the list elements
    // are actually ADEs of other flavors.
    //
    for (NTE = (NetTableEntry *)IF->ADE;
         ;
         NTE = (NetTableEntry *)NTE->Next) {

        if (NTE == NULL) {
            //
            // No existing entry for this prefix.
            // Create an entry if the lifetime is non-zero.
            //
            if (ValidLifetime != 0) {
                //
                // Auto-configure the new address.
                //
                NTE = CreateNTE(IF, Addr, AutoConfigured,
                                ValidLifetime, PreferredLifetime);
                rc = (int) NTE;
            }
            else {
                //
                // The address does not exist and we are not creating it.
                // So no NTE but we are successfull.
                //
                rc = TRUE;
            }
            break;
        }

        if (IP6_ADDR_EQUAL(&NTE->Address, Addr)) {
            if (NTE->Type != ADE_UNICAST) {
                //
                // Failure - the address must be unicast.
                //
                rc = (int) (NTE = NULL);
                break;
            }

            //
            // Found an existing entry.
            // Reset the lifetime values.
            // AddrConfTimeout (called from IPv6Timeout) handles
            // the invalid & deprecated state transitions.
            //
            // Note that to prevent denial of service,
            // we don't accept updates that lower the lifetime
            // to small values.
            //
            if ((ValidLifetime > PREFIX_LIFETIME_SAFETY) ||
                (ValidLifetime > NTE->ValidLifetime) ||
                Authenticated)
                NTE->ValidLifetime = ValidLifetime;
            else if (NTE->ValidLifetime <= PREFIX_LIFETIME_SAFETY)
                ; // ignore
            else
                NTE->ValidLifetime = PREFIX_LIFETIME_SAFETY;

            NTE->PreferredLifetime = PreferredLifetime;

            ASSERT(NTE->PreferredLifetime <= NTE->ValidLifetime);

            //
            // Need another reference to return.
            //
            AddRefNTE(NTE);
            rc = (int) NTE;
            break;
        }
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (pNTE != NULL)
        *pNTE = NTE;
    else if (NTE != NULL)
        ReleaseNTE(NTE);

    return rc;
}

//* AddrConfReset
//
//  Resets the interface's auto-configured address lifetimes.
//
//  Called with the interface locked.
//  Callable from thread or DPC context.
//
void
AddrConfReset(Interface *IF, uint MaxLifetime)
{
    NetTableEntry *NTE;

    for (NTE = (NetTableEntry *) IF->ADE;
         NTE != NULL;
         NTE = (NetTableEntry *) NTE->Next) {

        //
        // Is this an auto-configured unicast address?
        //
        if ((NTE->Type == ADE_UNICAST) && NTE->AutoConfigured) {

            //
            // Set the valid lifetime to a small value.
            // If we don't get an RA soon, the address will expire.
            //
            if (NTE->ValidLifetime > MaxLifetime)
                NTE->ValidLifetime = MaxLifetime;
            if (NTE->PreferredLifetime > NTE->ValidLifetime)
                NTE->PreferredLifetime = NTE->ValidLifetime;
        }
    }
}

//* ReconnectADEs
//
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
ReconnectADEs(Interface *IF)
{
    AddressEntry *ADE;

    for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *) ADE;

            //
            // Restart Duplicate Address Detection,
            // if it is enabled for this interface.
            //
            if ((IF->DupAddrDetectTransmits != 0) &&
                (NTE->DADState != DAD_STATE_INVALID)) {

                NTE->DADState = DAD_STATE_TENTATIVE;
                NTE->DADCount = IF->DupAddrDetectTransmits;
                NTE->DADTimer = 1;
            }
            break;
        }

        case ADE_ANYCAST:
            //
            // Nothing to do for anycast addresses.
            //
            break;

        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;

            //
            // Rejoin this multicast group,
            // if it is reportable.
            //
            KeAcquireSpinLockAtDpcLevel(&QueryListLock);
            if (MAE->MCastFlags & MAE_REPORTABLE) {
                MAE->MCastCount = MLD_NUM_INITIAL_REPORTS;
                if (MAE->MCastTimer == 0)
                    AddToQueryList(MAE);
                MAE->MCastTimer = 1;
            }
            KeReleaseSpinLockFromDpcLevel(&QueryListLock);
            break;
        }
        }
    }
}

//* DestroyNTE
//
//  Make an NTE be invalid, resulting in its eventual destruction.
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
DestroyNTE(Interface *IF, NetTableEntry *NTE)
{
    ASSERT(NTE->IF == IF);

    if (NTE->DADState != DAD_STATE_INVALID) {

        KdPrint(("DestroyNTE(NTE %x, Addr %s) -> invalid\n",
                 NTE, FormatV6Address(&NTE->Address)));

        //
        // Invalidate this address.
        //
        NTE->DADState = DAD_STATE_INVALID;
        InvalidateRouteCache();

        //
        // Remove ADEs that reference this address.
        // Note that this also removes from the interface's list,
        // but not does free, the NTE itself.
        //
        DestroyADEs(IF, NTE);

        //
        // The corresponding solicited-node address is not needed.
        //
        FindAndReleaseSolicitedNodeMAE(IF, &NTE->Address);

        if (IF != &LoopInterface) {
            //
            // The loop-back interface also has an ADE for this NTE.
            //
            KeAcquireSpinLockAtDpcLevel(&LoopInterface.Lock);
            DestroyADEs(&LoopInterface, NTE);
            KeReleaseSpinLockFromDpcLevel(&LoopInterface.Lock);
        }

        //
        // Remove the loopback route for this address.
        //
        RouteTableUpdate(IF, LoopNCE,
                         &NTE->Address, 128, 0,
                         0, DEFAULT_LOOPBACK_PREF,
                         FALSE, FALSE);

        //
        // Release the interface's reference for the NTE.
        //
        ReleaseNTE(NTE);
    }
}


//* AddrConfTimeout - Perform valid/preferred lifetime expiration.
//
//  Called periodically from NetTableTimeout on every NTE.
//  As usual, caller must hold a reference for the NTE.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
AddrConfTimeout(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    NetTableEntry **PrevNTE;

    ASSERT(NTE->Type == ADE_UNICAST);

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // If the valid lifetime is zero, then the NTE should be invalid.
    //
    if (NTE->ValidLifetime == 0) {
        DestroyNTE(IF, NTE);
    } else if (NTE->ValidLifetime != INFINITE_LIFETIME)
        NTE->ValidLifetime--;

    //
    // If the preferred lifetime is zero, then the NTE should be deprecated.
    //
    if (NTE->PreferredLifetime == 0) {
        if (NTE->DADState == DAD_STATE_PREFERRED) {

            KdPrint(("AddrConfTimeout(NTE %x, Addr %s) -> deprecated\n",
                     NTE, FormatV6Address(&NTE->Address)));

            //
            // Make this address be deprecated.
            //
            NTE->DADState = DAD_STATE_DEPRECATED;
            InvalidateRouteCache();
        }
    } else {
        //
        // If the address was deprecated, then it should be preferred.
        // (AddrConfUpdate must have just increased the preferred lifetime.)
        //
        if (NTE->DADState == DAD_STATE_DEPRECATED)
            NTE->DADState = DAD_STATE_PREFERRED;

        if (NTE->PreferredLifetime != INFINITE_LIFETIME)
            NTE->PreferredLifetime--;
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
}


//* NetTableCleanup
//
//  Cleans up any NetTableEntries with zero references.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
NetTableCleanup(void)
{
    NetTableEntry *DestroyList = NULL;
    NetTableEntry *NTE, **PrevNTE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&NetTableListLock, &OldIrql);

    PrevNTE = &NetTableList;
    while ((NTE = *PrevNTE) != NULL) {

        if (NTE->RefCnt == 0) {

            ASSERT(NTE->DADState == DAD_STATE_INVALID);
            *PrevNTE = NTE->NextOnNTL;
            NTE->NextOnNTL = DestroyList;
            DestroyList = NTE;

        } else {
            PrevNTE = &NTE->NextOnNTL;
        }
    }

    KeReleaseSpinLock(&NetTableListLock, OldIrql);

    while (DestroyList != NULL) {
        NTE = DestroyList;
        DestroyList = NTE->NextOnNTL;

        KdPrint(("NetTableCleanup(NTE %x, Addr %s) -> destroyed\n",
                 NTE, FormatV6Address(&NTE->Address)));

        ReleaseIF(NTE->IF);
        ExFreePool(NTE);
    }
}


//* NetTableTimeout
//
//  Called periodically from IPv6Timeout.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
NetTableTimeout(void)
{
    NetTableEntry *NTE;
    int SawZeroReferences = FALSE;

    //
    // Because new NTEs are only added at the head of the list,
    // we can unlock the list during our traversal
    // and know that the traversal will terminate properly.
    //

    KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->NextOnNTL) {
        AddRefNTE(NTE);
        KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

        //
        // Check for Duplicate Address Detection timeout.
        // The timer check here is only an optimization,
        // because it is made without holding the appropriate lock.
        //
        if (NTE->DADTimer != 0)
            DADTimeout(NTE);

        //
        // Perform lifetime expiration.
        //
        AddrConfTimeout(NTE);

        KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
        ReleaseNTE(NTE);

        //
        // We assume that loads of RefCnt are atomic.
        //
        if (NTE->RefCnt == 0)
            SawZeroReferences = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

    if (SawZeroReferences)
        NetTableCleanup();
}


//* InterfaceCleanup
//
//  Cleans up any Interfaces with zero references.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
InterfaceCleanup(void)
{
    Interface *DestroyList = NULL;
    Interface *IF, **PrevIF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);

    PrevIF = &IFList;
    while ((IF = *PrevIF) != NULL) {

        if (IF->RefCnt == 0) {

            ASSERT(IsDisabledIF(IF));
            *PrevIF = IF->Next;
            IF->Next = DestroyList;
            DestroyList = IF;

        } else {
            PrevIF = &IF->Next;
        }
    }

    KeReleaseSpinLock(&IFListLock, OldIrql);

    while (DestroyList != NULL) {
        IF = DestroyList;
        DestroyList = IF->Next;

        KdPrint(("InterfaceCleanup(IF %u/%x) -> destroyed\n", IF->Index, IF));

        //
        // ADEs should already be destroyed.
        // We just need to cleanup NCEs and free the interface.
        //
        ASSERT(IF->ADE == NULL);
        NeighborCacheDestroy(IF);
#if 0
        //
        // BUGBUG - There are two problems here.
        // First, the tunnel and loopback interfaces are statically allocated.
        // Second, there appears to be a problem with IF->MCastLock.
        //
        ExFreePool(IF);
#endif
    }
}


//* InterfaceTimeout
//
//  Called periodically from IPv6Timeout.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
InterfaceTimeout(void)
{
    static uint RecalcReachableTimer = 0;
    int RecalcReachable;
    int ForceRAs;
    Interface *IF;
    int SawZeroReferences = FALSE;

    //
    // Recalculate ReachableTime every few hours,
    // even if no Router Advertisements are received.
    //
    if (RecalcReachableTimer == 0) {
        RecalcReachable = TRUE;
        RecalcReachableTimer = RECALC_REACHABLE_INTERVAL;
    } else {
        RecalcReachable = FALSE;
        RecalcReachableTimer--;
    }

    //
    // Grab the value of ForceRouterAdvertisements.
    //
    ForceRAs = InterlockedExchange(&ForceRouterAdvertisements, FALSE);

    //
    // Because new interfaces are only added at the head of the list,
    // we can unlock the list during our traversal
    // and know that the traversal will terminate properly.
    //

    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        AddRefIF(IF);
        KeReleaseSpinLockFromDpcLevel(&IFListLock);

        //
        // Handle per-neighbor timeouts.
        //
        NeighborCacheTimeout(IF);

        //
        // Handle router solicitations.
        // The timer check here is only an optimization,
        // because it is made without holding the appropriate lock.
        //
        if (IF->RSTimer != 0)
            RouterSolicitTimeout(IF);

        //
        // Handle router advertisements.
        // The timer check here is only an optimization,
        // because it is made without holding the appropriate lock.
        //
        if (IF->RATimer != 0)
            RouterAdvertTimeout(IF, ForceRAs);

        //
        // Recalculate the reachable time.
        //
        if (RecalcReachable) {

            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);
        }

        KeAcquireSpinLockAtDpcLevel(&IFListLock);
        ReleaseIF(IF);

        //
        // We assume that loads of RefCnt are atomic.
        //
        if (IF->RefCnt == 0)
            SawZeroReferences = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    if (SawZeroReferences)
        InterfaceCleanup();
}


//* ControlInterface
//
//  Allows the forwarding & advertising attributes
//  of an interface to be changed.
//
//  Called with no locks held.
//
NTSTATUS
ControlInterface(
    Interface *IF,
    int Advertises,
    int Forwards)
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    //
    // The Advertises attribute can only be controlled
    // on interfaces that support Neighbor Discovery.
    //
    if ((Advertises != -1) && !(IF->Flags & IF_FLAG_DISCOVERS)) {
        Status = STATUS_INVALID_PARAMETER_2;
    }
    else {
        switch (Advertises) {
        case -2:
            //
            // Cause the interface to send a Router Solicitation
            // or Router Advertisement.
            //
            if (IF->Flags & IF_FLAG_ADVERTISES) {
                //
                // Send a Router Advertisement very soon.
                //
                IF->RATimer = 1;
            }
            else {
                //
                // Send a Router Solicitation.
                //
                IF->RSCount = 0;
                IF->RSTimer = 1;
            }
            break;

        case -1:
            //
            // Leave the interface alone.
            //
            break;

        case FALSE:
            //
            // Stop being an advertising interface,
            // if it is currently advertising.
            //
            if (IF->Flags & IF_FLAG_ADVERTISES) {
                //
                // Leave the all-routers multicast group.
                //
                LeaveGroupAtAllScopes(IF, &AllRoutersOnLinkAddr,
                                      ADE_SITE_LOCAL);

                //
                // Stop sending Router Advertisements.
                //
                IF->Flags &= ~IF_FLAG_ADVERTISES;
                IF->RATimer = 0;

                //
                // Send Router Solicitations again.
                //
                IF->RSCount = 0;
                IF->RSTimer = 1;
            }
            break;

        case TRUE:
            //
            // Become an advertising interfacing,
            // if it is not already.
            //
            if (!(IF->Flags & IF_FLAG_ADVERTISES)) {
                MulticastAddressEntry *MAE;

                //
                // Join the all-routers multicast groups.
                //
                if (JoinGroupAtAllScopes(IF, &AllRoutersOnLinkAddr,
                                         ADE_SITE_LOCAL)) {
                    //
                    // A non-advertising interface is now advertising.
                    //
                    IF->Flags |= IF_FLAG_ADVERTISES;

                    //
                    // Start sending Router Advertisements.
                    //
                    IF->RATimer = 1; // Send first RA very quickly.
                    IF->RACount = MAX_INITIAL_RTR_ADVERTISEMENTS;
                }
                else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            break;
        }
    }

    //
    // Control the forwarding behavior, if we haven't had an error.
    // Any change in forwarding behavior requires InvalidRouteCache because
    // FindNextHop uses IF_FLAG_FORWARDS. Also force the next RA
    // for all advertising interfaces to be sent quickly,
    // because their content might depend on forwarding behavior.
    //
    if ((Status == STATUS_SUCCESS) && (Forwards != -1)) {
        if (Forwards) {
            //
            // If the interface is not currently forwarding,
            // enable forwarding.
            //
            if (!(IF->Flags & IF_FLAG_FORWARDS)) {
                IF->Flags |= IF_FLAG_FORWARDS;
                InvalidateRouteCache();
                ForceRouterAdvertisements = TRUE;
            }
        }
        else {
            //
            // If the interface is currently forwarding,
            // disable forwarding.
            //
            if (IF->Flags & IF_FLAG_FORWARDS) {
                IF->Flags &= ~IF_FLAG_FORWARDS;
                InvalidateRouteCache();
                ForceRouterAdvertisements = TRUE;
            }
        }
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return Status;
}

//* SetInterfaceLinkStatus
//
//  Change the interface's link status. In particular,
//  set whether the media is connected or disconnected.
//
void
SetInterfaceLinkStatus(
    void *Context,
    int MediaConnected)         // TRUE or FALSE.
{
    Interface *IF = (Interface *) Context;
    KIRQL OldIrql;

    //
    // Note that media-connect/disconnect events
    // can be "lost". We are not informed if the
    // cable is unplugged/replugged while we are
    // shutdown, hibernating, or on standby.
    //

    KdPrint(("SetInterfaceLinkStatus(IF %x) -> %s\n",
             IF, MediaConnected ? "connected" : "disconnected"));

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if (MediaConnected) {
        //
        // The cable was plugged back in.
        //
        IF->Flags &= ~IF_FLAG_MEDIA_DISCONNECTED;

        //
        // Changes in IF_FLAG_MEDIA_DISCONNECTED must
        // invalidate the route cache.
        //
        InvalidateRouteCache();

        //
        // Purge potentially obsolete link-layer information.
        // Things might have changed while we were unplugged.
        //
        NeighborCacheStale(IF);

        //
        // Rejoin multicast groups and restart Duplicate Address Detection.
        //
        ReconnectADEs(IF);

        if (IF->Flags & IF_FLAG_ADVERTISES) {
            //
            // Send a Router Advertisement very soon.
            //
            IF->RATimer = 1;
        }
        else {
            //
            // Start sending Router Solicitations.
            //
            IF->RSCount = 0;
            IF->RSTimer = 1;

            //
            // Remember that this interface was just reconnected,
            // so when we receive a Router Advertisement
            // we can take special action.
            //
            IF->Flags |= IF_FLAG_MEDIA_RECONNECTED;
        }
    }
    else {
        //
        // The cable was unplugged.
        //
        IF->Flags = (IF->Flags | IF_FLAG_MEDIA_DISCONNECTED) &~
                                                IF_FLAG_MEDIA_RECONNECTED;

        //
        // Changes in IF_FLAG_MEDIA_DISCONNECTED must
        // invalidate the route cache.
        //
        InvalidateRouteCache();
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);
}
