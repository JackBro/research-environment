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
// Neighbor Discovery (ND) Protocol for Internet Protocol Version 6.
// Logically a part of ICMPv6, but put in a seperate file for clarity.
// See RFC 2461 for details.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "alloca.h"

//
//  REVIEW: What is a reasonable value for NeighborCacheLimit?
//  Should probably be sized relative to physical memory
//  and link characteristics.
//
//  The route cache holds RCEs, which in turn hold references
//  to NCEs. If the limit for RCE cache is large relative to the
//  limit for NCE caching, we will waste much time trying to find
//  unused NCEs. We may need a mechanism to flush unused RCEs
//  when we run short of NCEs.
//
//  Also note that we never reclaim NCEs (no matter how stale)
//  until we hit the limit. Not clear if this is good or bad.
//  And we never free NCEs at all.
//
//  We cache & reclaim NCEs on a per-interface basis.
//  Theoretically it would be better to use a global LRU list.
//  However this would introduce added overhead (making NCEs bigger)
//  and locking. At this point I believe that it isn't worth it.
//
//  Another thought - it's much more important to support many RCEs
//  than it is to support many NCEs.
//
uint NeighborCacheLimit = 8;

//* NeighborCacheInit
//
//  Initialize the neighbor cache for an interface.
//
//  This is done when the interface is being created
//  so it does not need to be locked.
//
void
NeighborCacheInit(Interface *IF)
{
    IF->FirstNCE = IF->LastNCE = SentinelNCE(IF);
    IF->NCECount = 0;
}

//* NeighborCacheDestroy
//
//  Cleanup and deallocate the NCEs in the neighbor cache.
//
//  This is done when the interface is being destroyed
//  and no one else has access to it, so it does not need to be locked.
//
void
NeighborCacheDestroy(Interface *IF)
{
    NeighborCacheEntry *NCE;
    PNDIS_PACKET Packet;

    ASSERT(IsDisabledIF(IF));
    ASSERT(IF->RefCnt == 0);

    while ((NCE = IF->FirstNCE) != SentinelNCE(IF)) {
        ASSERT(NCE->IF == IF);
        ASSERT(NCE->RefCnt == 0);

        //
        // Unlink the NCE.
        //
        NCE->Next->Prev = NCE->Prev;
        NCE->Prev->Next = NCE->Next;

        //
        // If there's a packet waiting, destroy it too.
        //
        Packet = NCE->WaitQueue;
        if (Packet != NULL)
            IPv6SendComplete(NULL, Packet, NDIS_STATUS_REQUEST_ABORTED);

        ExFreePool(NCE);
    }
}

//* NeighborCacheStale
//
//  Called to indicate that the contents of the neighbor cache are stale.
//  Deletes unreferenced NCEs, transitions remaining NCEs to INCOMPLETE.
//
//  Callable from thread or DPC context.
//  Called with the interface lock held.
//
void
NeighborCacheStale(Interface *IF)
{
    NeighborCacheEntry *NCE, *NextNCE;

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NextNCE) {

        NextNCE = NCE->Next;

        if ((NCE->RefCnt == 0) && (NCE->WaitQueue == NULL)) {
            //
            // Unlink the NCE.
            //
            NCE->Next->Prev = NCE->Prev;
            NCE->Prev->Next = NextNCE;

            ExFreePool(NCE);
        }
        else {
            if (NCE->NDState != ND_STATE_PERMANENT) {
                //
                // Forget everything we know about this neighbor.
                // The net result is like FindOrCreateNeighbor.
                //
                NCE->IsRouter = FALSE;
                NCE->IsUnreachable = FALSE;
                NCE->NDState = ND_STATE_INCOMPLETE;

                if (NCE->WaitQueue != NULL) {
                    //
                    // There's a packet waiting, so restart discovery.
                    // NeighborCacheTimeout will send a solicit for us.
                    //
                    NCE->EventCount = 1;
                    NCE->EventTimer = IF->RetransTimer;
                }
                else {
                    //
                    // Cancel any pending timeout.
                    //
                    NCE->EventTimer = NCE->EventCount = 0;
                }

                InvalidateRouteCache();
            }
        }
    }
}

//
//* AddRefNCEInCache
//
//  Increments the reference count on an NCE
//  in the interface's neighbor cache.
//  The NCE's current reference count may be zero.
//
//  Callable from thread or DPC context.
//  Called with the interface lock held.
//
void
AddRefNCEInCache(NeighborCacheEntry *NCE)
{
    //
    // If the NCE previously had no references,
    // increment the interface's reference count.
    //
    if (InterlockedIncrement(&NCE->RefCnt) == 1) {
        Interface *IF = NCE->IF;

        ASSERT(!IsDisabledIF(IF));
        AddRefIF(IF);
    }
}

//* ReleaseNCE
//
//  Releases a reference for an NCE.
//  May result in deallocation of the NCE.
//
//  Callable from thread or DPC context.
//
void
ReleaseNCE(NeighborCacheEntry *NCE)
{
    //
    // If the NCE has no more references,
    // release its reference for the interface.
    // This may cause the interface (and hence the NCE)
    // to be deallocated.
    //
    if (InterlockedDecrement(&NCE->RefCnt) == 0) {
        Interface *IF = NCE->IF;

        ReleaseIF(IF);
    }
}

//* CreateOrReuseNeighbor
//
//  Creates a new NCE for an interface.
//  Attempts to reuse an existing NCE if the interface
//  already has too many NCEs.
//
//  Called with the interface lock held.
//
//  Returns NULL if a new NCE could not be created.
//  Returns the NCE with RefCnt, IF, LinkAddress, WaitQueue
//  fields initialized.
//
NeighborCacheEntry *
CreateOrReuseNeighbor(Interface *IF)
{
    NeighborCacheEntry *NCE;

    if (IF->NCECount >= NeighborCacheLimit) {
        //
        // The cache is full, so try to reclaim an unused NCE.
        // Search backwards (starting with the LRU NCEs)
        // for an NCE with zero references.
        //
        for (NCE = IF->LastNCE; NCE != SentinelNCE(IF); NCE = NCE->Prev) {

            if ((NCE->RefCnt == 0) &&
                (NCE->WaitQueue == NULL)) {
            ReuseNCE:
                //
                // Reuse this NCE.
                // Just need to unlink it.
                //
                NCE->Next->Prev = NCE->Prev;
                NCE->Prev->Next = NCE->Next;

                return NCE;
            }
        }
    }
    else if (((NCE = IF->LastNCE) != SentinelNCE(IF)) &&
             (NCE->RefCnt == 0) &&
             (NCE->WaitQueue == NULL) &&
             (((NCE->NDState == ND_STATE_INCOMPLETE) && !NCE->IsUnreachable) ||
              (NCE->NDState == ND_STATE_PERMANENT))) {
        //
        // FindRoute tends to create NCEs that end up not getting used.
        // We reuse these unused NCEs even when the cache is not full.
        //
        goto ReuseNCE;
    }

    NCE = (NeighborCacheEntry *) ExAllocatePool(NonPagedPool, sizeof *NCE +
                                                IF->LinkAddressLength);
    if (NCE == NULL)
        return NULL;

    IF->NCECount++;

    NCE->RefCnt = 0;
    NCE->LinkAddress = (void *)(NCE + 1);
    NCE->IF = IF;
    NCE->WaitQueue = NULL;

    return NCE;
}

//* CreatePermanentNeighbor
//
//  Creates a permanent neighbor - one that does not participate
//  in neighbor discovery or neighbor unreachability detection.
//
//  Callable from thread or DPC context.
//
//  Returns NULL if the NCE could not be created.
//
NeighborCacheEntry *
CreatePermanentNeighbor(Interface *IF, IPv6Addr *Addr, void *LinkAddress)
{
    KIRQL OldIrql;
    NeighborCacheEntry *NCE;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (IP6_ADDR_EQUAL(Addr, &NCE->NeighborAddress)) {
            //
            // Found matching entry.
            //
            ASSERT(NCE->NDState == ND_STATE_PERMANENT);
            ASSERT(RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                                    IF->LinkAddressLength) ==
                                                IF->LinkAddressLength);
            goto ReturnNCE;
        }
    }

    //
    // Get a new entry for this neighbor.
    //
    NCE = CreateOrReuseNeighbor(IF);
    if (NCE == NULL) {
        KeReleaseSpinLock(&IF->Lock, OldIrql);
        return NULL;
    }

    //
    // Initialize the neighbor cache entry.
    // The RefCnt, IF, LinkAddress, WaitQueue fields are already initialized.
    //
    NCE->NeighborAddress = *Addr;
    RtlCopyMemory(NCE->LinkAddress, LinkAddress, IF->LinkAddressLength);
    NCE->IsRouter = FALSE;
    NCE->IsUnreachable = FALSE;
    NCE->NDState = ND_STATE_PERMANENT;
    NCE->EventTimer = 0;

    //
    // Link new entry into chain of NCEs on this interface.
    //
    NCE->Next = IF->FirstNCE;
    NCE->Next->Prev = NCE;
    NCE->Prev = SentinelNCE(IF);
    NCE->Prev->Next = NCE;

  ReturnNCE:
    AddRefNCEInCache(NCE);
    KeReleaseSpinLock(&IF->Lock, OldIrql);
    return NCE;
}

//* FindOrCreateNeighbor
//
//  Searches an interface's neighbor cache for an entry.
//  Creates the entry (but does NOT send initial solicit)
//  if an existing entry is not found.
//
//  Takes interface locks; may be called with route cache lock held.
//  Callable from thread or DPC context.
//
//  We avoid sending a solicit here, because this function
//  is called while holding locks that make that a bad idea.
//
//  Returns NULL only if an NCE could not be created.
//
NeighborCacheEntry *
FindOrCreateNeighbor(Interface *IF, IPv6Addr *Addr)
{
    KIRQL OldIrql;
    NeighborCacheEntry *NCE;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (IP6_ADDR_EQUAL(Addr, &NCE->NeighborAddress)) {
            //
            // Found matching entry.
            //
            goto ReturnNCE;
        }
    }

    //
    // Get a new entry for this neighbor.
    //
    NCE = CreateOrReuseNeighbor(IF);
    if (NCE == NULL) {
        KeReleaseSpinLock(&IF->Lock, OldIrql);
        return NULL;
    }

    //
    // Initialize the neighbor cache entry.
    // The RefCnt, IF, LinkAddress, WaitQueue fields are already initialized.
    //
    NCE->NeighborAddress = *Addr;
#if DBG
    RtlZeroMemory(NCE->LinkAddress, IF->LinkAddressLength);
#endif
    NCE->IsRouter = FALSE;
    NCE->IsUnreachable = FALSE;
    NCE->NDState = ND_STATE_INCOMPLETE;
    NCE->EventTimer = NCE->EventCount = 0;

    // 
    // We don't want to perform neighbor discovery on multicast addresses,
    // so we set the ND state accordingly.  Also, we create the link-layer
    // multicast address for IP multicast addresses.
    //
    if (IsMulticast(Addr)) {
        (*IF->ConvertMCastAddr)(IF->LinkContext, Addr, NCE->LinkAddress);
        NCE->NDState = ND_STATE_PERMANENT;  // This means no ND.
    }

    //
    // Link new entry into chain of NCEs on this interface.
    // Put the new entry at the end, because until it is
    // used to send a packet it has not proven itself valuable.
    //
    NCE->Prev = IF->LastNCE;
    NCE->Prev->Next = NCE;
    NCE->Next = SentinelNCE(IF);
    NCE->Next->Prev = NCE;

  ReturnNCE:
    AddRefNCEInCache(NCE);
    KeReleaseSpinLock(&IF->Lock, OldIrql);
    return NCE;
}

//* GetReachability
//
//  Returns reachability information for a neighbor.
//      -1      Time to round-robin.
//      0       Interface is disconnected - definitely not reachable.
//      1       ND failed - probably not reachable.
//      2       ND succeeded, or has not concluded.
//
//  Because FindRoute uses GetReachability, any state change
//  that changes GetReachability's return value
//  must invalidate the route cache.
//
//  The -1 return value is special - it indicates that FindRoute
//  should round-robin and use a different route. It is not
//  persistent - a subsequent call to GetReachability will return 1.
//
//  Callable from DPC context (or with the route lock held),
//  not from thread context.
//
int
GetReachability(NeighborCacheEntry *NCE)
{
    Interface *IF = NCE->IF;
    int Reachable;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    if (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)
        Reachable = 0;
    else if (NCE->IsUnreachable) {
        if (NCE->DoRoundRobin) {
            NCE->DoRoundRobin = FALSE;
            Reachable = -1;
        } else
            Reachable = 1;
    } else
        Reachable = 2;
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    return Reachable;
}

//* NeighborCacheUpdate - Update link address information about a neighbor.
//
//  Called when we've received possibly new information about one of our
//  neighbors, the source of a Neighbor Solicitation, Router Advertisement,
//  or Router Solicitation.
//
//  Note that our receipt of this packet DOES NOT imply forward reachability
//  to this neighbor, so we do not update our LastReachable timer.
//
//  Callable from DPC context, not from thread context.
//
//  The LinkAddress might be NULL, which means we can only process IsRouter.
//
//  If IsRouter is FALSE, then we don't know whether the neighbor
//  is a router or not.  If it's TRUE, then we know it is a router.
//
void
NeighborCacheUpdate(NeighborCacheEntry *NCE, // The neighbor.
                    void *LinkAddress,       // Corresponding media address.
                    int IsRouter)            // Do we know it's a router.
{
    Interface *IF = NCE->IF;
    PNDIS_PACKET Packet = NULL;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // Check to see if the link address changed.
    //
    if ((LinkAddress != NULL) &&
        ((NCE->NDState == ND_STATE_INCOMPLETE) ||
         RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                          IF->LinkAddressLength) != IF->LinkAddressLength)) {
        //
        // Link-level address changed.  Update cache entry with the
        // new one and change state to STALE as we haven't verified
        // forward reachability with the new address yet.
        //
        RtlCopyMemory(NCE->LinkAddress, LinkAddress, IF->LinkAddressLength);
        NCE->EventTimer = 0; // Cancel any outstanding timeout.
        NCE->NDState = ND_STATE_STALE;

        //
        // Flush the queue of waiting packets.
        // (Only relevant if we were in the INCOMPLETE state.)
        //
        if (NCE->WaitQueue != NULL) {

            Packet = NCE->WaitQueue;
            NCE->WaitQueue = NULL;
        }
    }

    //
    // If we know that the neighbor is a router,
    // remember that fact.
    //
    if (IsRouter)
        NCE->IsRouter = TRUE;

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    //
    // If we can now send a packet, do so.
    // (Without holding a lock.)
    //
    if (Packet != NULL) {
        uint Offset;

        Offset = PC(Packet)->pc_offset;
        IPv6Send0(Packet, Offset, NCE, &(PC(Packet)->DiscoveryAddress));
    }
}

//* NeighborCacheSearch
//
//  Searches the neighbor cache for an entry that matches
//  the neighbor IPv6 address. If found, returns the link-level address.
//  Returns FALSE to indicate failure.
//
//  Callable from DPC context, not from thread context.
//
int
NeighborCacheSearch(
    Interface *IF,
    IPv6Addr *Neighbor,
    void *LinkAddress)
{
    NeighborCacheEntry *NCE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (IP6_ADDR_EQUAL(Neighbor, &NCE->NeighborAddress)) {
            //
            // Entry found. Return it's cached link-address,
            // if it's valid.
            //
            if (NCE->NDState == ND_STATE_INCOMPLETE) {
                //
                // No valid link address.
                //
                break;
            }

            //
            // The entry has a link-level address.
            // Must copy it with the lock held.
            //
            RtlCopyMemory(LinkAddress, NCE->LinkAddress,
                          IF->LinkAddressLength);
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);
            return TRUE;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    return FALSE;
}

//* NeighborCacheAdvert
//
//  Updates the neighbor cache in response to an advertisement.
//  If no matching entry is found, ignores the advertisement.
//  (See RFC 1970 section 7.2.5.)
//
//  Callable from DPC context, not from thread context.
//
void
NeighborCacheAdvert(
    Interface *IF,
    IPv6Addr *TargetAddress,
    void *LinkAddress,
    ulong Flags)
{
    NeighborCacheEntry *NCE;
    PNDIS_PACKET Packet = NULL;
    int PurgeRouting = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {
        if (IP6_ADDR_EQUAL(TargetAddress, &NCE->NeighborAddress)) {

            //
            // Pick up link-level address from the advertisement,
            // if we don't have one yet or if override is set.
            //
            if ((LinkAddress != NULL) &&
                ((NCE->NDState == ND_STATE_INCOMPLETE) ||
                 ((Flags & ND_NA_FLAG_OVERRIDE) &&
                  RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                   IF->LinkAddressLength) != IF->LinkAddressLength))) {

                RtlCopyMemory(NCE->LinkAddress, LinkAddress,
                              IF->LinkAddressLength);

                NCE->EventTimer = 0; // Cancel any outstanding timeout.
                NCE->NDState = ND_STATE_STALE;

                //
                // Flush the queue of waiting packets.
                //
                if (NCE->WaitQueue != NULL) {

                    Packet = NCE->WaitQueue;
                    NCE->WaitQueue = NULL;

                    //
                    // Need to keep ref on NCE after we unlock.
                    //
                    AddRefNCEInCache(NCE);
                }

                goto AdvertisementMatchesCachedAddress;
            }

            if ((NCE->NDState != ND_STATE_INCOMPLETE) &&
                ((LinkAddress == NULL) ||
                 RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                  IF->LinkAddressLength) == IF->LinkAddressLength)) {
                ushort WasRouter;

            AdvertisementMatchesCachedAddress:

                //
                // If this is a solicited advertisement
                // for our cached link-layer address,
                // then we have confirmed reachability.
                //
                if (Flags & ND_NA_FLAG_SOLICITED) {
                    NCE->EventTimer = 0;  // Cancel any outstanding timeout.
                    NCE->LastReachable = IPv6TickCount;  // Timestamp it.
                    NCE->NDState = ND_STATE_REACHABLE;

                    if (NCE->IsUnreachable) {
                        //
                        // We had previously concluded that this neighbor
                        // is unreachable. Now we know otherwise.
                        //
                        NCE->IsUnreachable = FALSE;
                        InvalidateRouteCache();
                    }
                }

                //
                // If this is an advertisement
                // for our cached link-layer address,
                // then update IsRouter.
                //
                WasRouter = NCE->IsRouter;
                NCE->IsRouter = ((Flags & ND_NA_FLAG_ROUTER) != 0);
                if (WasRouter && !NCE->IsRouter) {
                    //
                    // This neighbor used to be a router, but is no longer.
                    //
                    PurgeRouting = TRUE;

                    //
                    // Need to keep ref on NCE after we unlock.
                    //
                    AddRefNCEInCache(NCE);
                }
            }
            else {
                //
                // This is not an advertisement
                // for our cached link-layer address.
                // If the advertisement was unsolicited,
                // give NUD a little nudge.
                //
                if (Flags & ND_NA_FLAG_SOLICITED) {
                    //
                    // This is probably a second NA in response
                    // to our multicast NS for an anycast address.
                    //
                }
                else {
                    if (NCE->NDState == ND_STATE_REACHABLE)
                        NCE->NDState = ND_STATE_STALE;
                }
            }

            //
            // Only one NCE should match.
            //
            break;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    //
    // If we can now send a packet, do so.
    // (Without holding a lock.)
    //
    // It is possible that this neighbor is no longer a router,
    // and the waiting packet wants to use the neighbor as a router.
    // In this situation the ND spec requires that we still send
    // the waiting packet to the neighbor. Narten/Nordmark confirmed
    // this interpretation in private email.
    //
    if (Packet != NULL) {
        uint Offset = PC(Packet)->pc_offset;
        IPv6Send0(Packet, Offset, NCE, &(PC(Packet)->DiscoveryAddress));
        ReleaseNCE(NCE);
    }

    //
    // If need be, purge the routing data structures.
    //
    if (PurgeRouting) {
        InvalidateRouter(NCE);
        ReleaseNCE(NCE);
    }
}

typedef struct NeighborCacheEntrySolicitInfo {
    WORK_QUEUE_ITEM WQItem;
    NeighborCacheEntry *NCE;            // Holds a reference.
    IPv6Addr *DiscoveryAddress;         // NULL or points to AddrBuf.
    IPv6Addr AddrBuf;
} NeighborCacheEntrySolicitInfo;

//* NeighborCacheEntrySendSolicitWorker
//
//  Worker function for sending a solicit for an NCE.
//  Because NeighborCacheEntryTimeout can not call NeighborSolicitSend
//  directly, it uses this function via a worker thread.
//
void
NeighborCacheEntrySendSolicitWorker(PVOID Context)
{
    NeighborCacheEntrySolicitInfo *Info =
        (NeighborCacheEntrySolicitInfo *) Context;

    NeighborSolicitSend(Info->NCE, Info->DiscoveryAddress);
    ReleaseNCE(Info->NCE);
    ExFreePool(Info);
}

typedef struct SendAbortInfo {
    WORK_QUEUE_ITEM WQItem;
    Interface *IF;                      // BUGBUG - Refcounting.
    NDIS_PACKET *Packet;                // Holds ownership.
} SendAbortInfo;

//* SendAbortWorker
//
//  Worker function for aborting a packet send.
//  Because NeighborCacheEntryTimeout can not call IPv6SendAbort
//  directly, it uses this function via a worker thread.
//
void
SendAbortWorker(PVOID Context)
{
    SendAbortInfo *Info = (SendAbortInfo *) Context;
    NDIS_PACKET *Packet = Info->Packet;

    IPv6SendAbort(CastFromIF(Info->IF),
                  Packet, PC(Packet)->pc_offset,
                  ICMPv6_DESTINATION_UNREACHABLE,
                  ICMPv6_ADDRESS_UNREACHABLE,
                  0, FALSE);
    ExFreePool(Info);
}

//* NeighborCacheEntryTimeout - handle an event timeout on an NCE.
//
//  NeighborCacheTimeout calls this routine when
//  an NCE's EventTimer expires.
//
//  Called with the interface lock held.
//
//  Because we can not call NeighborSolicitSend, IPv6SendAbort,
//  or IPv6SendComplete while holding the interface lock,
//  we punt these operations to a worker thread.
//  This means they may be delayed.
//  However we make all the NCE state transitions directly here,
//  so they will happen in a timely fashion.
//
void
NeighborCacheEntryTimeout(
    NeighborCacheEntry *NCE)
{
    //
    // Neighbor Discovery has timeouts for initial neighbor
    // solicitation retransmissions, the delay state, and probe
    // neighbor solicitation retransmissions.  All of these share
    // the same EventTimer, and are distinguished from each other
    // by the NDState.
    //
    switch (NCE->NDState) {
    case ND_STATE_INCOMPLETE:
        //
        // Retransmission timer expired.  Check to see if
        // sending another solicit would exceed the maximum.
        //
        if (++NCE->EventCount > MAX_MULTICAST_SOLICIT) {
            //
            // Failed to initiate connectivity to neighbor.
            // Reset to dormant INCOMPLETE state.
            //
            NCE->EventCount = 0;
            if (NCE->WaitQueue != NULL) {
                SendAbortInfo *Info;
                PNDIS_PACKET Packet;

                //
                // Remove the waiting packet from the NCE
                // and use SendAbortWorker to send
                // an ICMP Address Unreachable error.
                // We can't call IPv6SendAbort directly
                // because we hold the interface lock.
                //
                Packet = NCE->WaitQueue;
                NCE->WaitQueue = NULL;

                //
                // If this allocation fails we are in a bit of a pickle.
                // Because we hold the interface lock, we can't call
                // IPv6SendComplete to dispose of the packet.
                // So we just leak the packet.
                //
                Info = ExAllocatePool(NonPagedPool, sizeof *Info);
                if (Info != NULL) {
                    ExInitializeWorkItem(&Info->WQItem, SendAbortWorker, Info);
                    Info->IF = NCE->IF; // BUGBUG - Refcounting.
                    Info->Packet = Packet;
                    ExQueueWorkItem(&Info->WQItem, CriticalWorkQueue);
                }
            }

            //
            // This neighbor is not reachable.
            // IsUnreachable may already be TRUE.
            // But we need to give FindRoute an opportunity to round-robin.
            //
            NCE->IsUnreachable = TRUE;
            NCE->DoRoundRobin = TRUE;
            InvalidateRouteCache();

        } else {
            //
            // Retransmit initial solicit, taking source address
            // from the waiting packet.
            //
            goto SendSolicit;
        }
        break;

    case ND_STATE_DELAY:
        //
        // Delay timer expired.  Enter PROBE state, send
        // first probe, and start retransmission timer.
        //
        ASSERT(NCE->WaitQueue == NULL);
        NCE->NDState = ND_STATE_PROBE;
        NCE->EventCount = 0;
        NCE->EventTimer = NCE->IF->RetransTimer;

        // Drop into PROBE state to send first probe...

    case ND_STATE_PROBE:
        //
        // Retransmission timer expired.  Check to see if
        // sending another solicit would exceed the maximum.
        //
        ASSERT(NCE->WaitQueue == NULL);
        if (++NCE->EventCount > MAX_UNICAST_SOLICIT) {
            //
            // Failed to initiate connectivity to neighbor.
            // Reset to dormant INCOMPLETE state.
            //
            NCE->NDState = ND_STATE_INCOMPLETE;
            NCE->EventCount = 0;

            //
            // This neighbor is not reachable.
            // IsUnreachable may already be TRUE.
            // But we need to give FindRoute an opportunity to round-robin.
            //
            NCE->IsUnreachable = TRUE;
            NCE->DoRoundRobin = TRUE;
            InvalidateRouteCache();

        } else {
            NeighborCacheEntrySolicitInfo *Info;
            NDIS_PACKET *Packet;

            //
            // Retransmit probe solicitation. We can not call
            // NeighborSolicitSend directly because we have
            // the interface locked, so punt to a worker thread.
            //
          SendSolicit:
            NCE->EventTimer = NCE->IF->RetransTimer;

            //
            // If this allocation fails we just skip this solicitation.
            //
            Info = ExAllocatePool(NonPagedPool, sizeof *Info);
            if (Info != NULL) {
                ExInitializeWorkItem(&Info->WQItem,
                                     NeighborCacheEntrySendSolicitWorker,
                                     Info);

                AddRefNCEInCache(NCE);
                Info->NCE = NCE;

                //
                // If we have a packet waiting for address resolution,
                // then take the source address for the solicit
                // from the waiting packet.
                //
                Packet = NCE->WaitQueue;
                if (Packet != NULL) {
                    Info->DiscoveryAddress = &Info->AddrBuf;
                    Info->AddrBuf = PC(Packet)->DiscoveryAddress;
                } else {
                    Info->DiscoveryAddress = NULL;
                }

                ExQueueWorkItem(&Info->WQItem, CriticalWorkQueue);
            }
        }
        break;

    default:
        //
        // Should never happen.  Other states don't utilize
        // the EventTimer.
        //
        ASSERT(!"NeighborCacheEntryTimeout: ND Event Timer in bad state\n");
        break;
    }
}

//* NeighborCacheTimeout
//
//  Called periodically from IPv6Timeout to handle timeouts
//  in the interface's neighbor cache.
//
//  Callable from DPC context, not from thread context.
//
//  Note that NeighborCacheTimeout performs an unbounded
//  amount of work while holding the interface lock.
//  NeighborCacheEntryTimeout can send packets.
//  This is difficult to avoid, because if we unlock
//  then the list can be scrambled.
//
//  One possible strategy that would help, if this is a problem,
//  would be to have a second singly-linked list of NCEs
//  that require action.  With one traversal we reference NCEs
//  and create the action list.  Then we could traverse the action
//  list at our leisure, taking/dropping the interface lock.
//
//  On the other hand, this is all moot on a uniprocessor
//  because our locks are spinlocks and we are already at DPC level.
//  That is, on a uniprocessor KeAcquireSpinLockAtDpcLevel is a no-op.
//
void
NeighborCacheTimeout(Interface *IF)
{
    NeighborCacheEntry *NCE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (NCE->EventTimer != 0) {
            //
            // Timer is running.  Decrement and check for expiration.
            //
            if (--NCE->EventTimer == 0) {
                //
                // Timer went off.
                //
                NeighborCacheEntryTimeout(NCE);
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
}

//* FlushNeighborCache
//
//  Flushes unused neighbor cache entries.
//  If an address is supplied, flushes the NCE (at most one) for that address.
//  Otherwise, flushes all unused NCEs on the interface.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
FlushNeighborCache(Interface *IF, IPv6Addr *Addr)
{
    NeighborCacheEntry *Delete = NULL;
    NeighborCacheEntry *NCE, *NextNCE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NextNCE) {

        NextNCE = NCE->Next;
        if (Addr == NULL)
            ; // Examine this NCE then keep looking.
        else if (IP6_ADDR_EQUAL(Addr, &NCE->NeighborAddress))
            NextNCE = SentinelNCE(IF); // Can terminate loop after this NCE.
        else
            continue; // Skip this NCE.

        //
        // Can we flush this NCE?
        //
        if ((NCE->RefCnt == 0) &&
            (NCE->WaitQueue == NULL)) {
            //
            // Just need to unlink it.
            //
            NCE->Next->Prev = NCE->Prev;
            NCE->Prev->Next = NCE->Next;

            //
            // And put it on our Delete list.
            //
            NCE->Next = Delete;
            Delete = NCE;
        }
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Finish by actually deleting the flushed NCEs.
    //
    while (Delete != NULL) {
        NCE = Delete;
        Delete = NCE->Next;
        ExFreePool(NCE);
    }
}


//* RouterSolicitReceive - Handle Router Solicitation messages.
//
//  See section 6.2.6 of the ND spec.
//
char *
RouterSolicitReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6Receive.
    ICMPv6Header *ICMP)         // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    void *SourceLinkAddress;
    int SendRouterAdvert = FALSE;

    //
    // Ignore the advertisement unless this is an advertising interface.
    //
    if (!(IF->Flags & IF_FLAG_ADVERTISES))
        return NULL;

    //
    // Validate the solicitation.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the length, source address, hop limit, and ICMP code.
    //
    if (Packet->IP->HopLimit != 255) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrint(("RouterSolicitReceive: "
                 "Received a routed router solicitation\n"));
        return NULL;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted router solicitation.  Drop the packet.
        //
        KdPrint(("RouterSolicitReceive: "
                 "Received a corrupted router solicitation\n"));
        return NULL;
    }

    //
    // We should have a 4-byte reserved field.
    //
    if (Packet->TotalSize < 4) {
        //
        // Packet too short to contain minimal solicitation.
        //
        KdPrint(("RouterSolicitReceive: Received a too short solicitation\n"));
        return NULL;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (Packet->ContigSize != Packet->TotalSize) {
        if (PacketPullup(Packet, Packet->TotalSize) == 0) {
            // Can only fail if we run out of memory.
            return NULL;
        }
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Skip over 4 byte "Reserved" field, ignoring whatever may be in it.
    //
    AdjustPacketParams(Packet, 4);

    //
    // We may have a source link-layer address option present.
    // Check for it and silently ignore all others.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrint(("RouterSolicitReceive: "
                     "Received option with bogus length\n"));
            return NULL;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_SOURCE_LINK_LAYER_ADDRESS) {
            //
            // Parse the link-layer address option.
            //
            SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                 (uchar *)Packet->Data);
            if (SourceLinkAddress == NULL) {
                //
                // Invalid option format. Drop the packet.
                //
                KdPrint(("RouterSolicitReceive: Received bogus ll option\n"));
                return NULL;
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one. We must sanity-check all option lengths.
            //
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We've received and parsed a valid router solicitation.
    //

    if (IsUnspecified(&Packet->IP->Source)) {
#if 0
        //
        // This is a new check, introduced post-RFC 1970.
        // REVIEW - For backwards-compatibility, disable it.
        //
        if (SourceLinkAddress != NULL) {
            KdPrint(("RouterSolicitReceive: "
                     "Received SLA with unspecified Source?\n"));
            return NULL;
        }
#endif
    }
    else {
        //
        // Only bother with this if SourceLinkAddress is present;
        // if it's not, NeighborCacheUpdate won't do anything.
        //
        if (SourceLinkAddress != NULL) {
            NeighborCacheEntry *NCE;
            //
            // Get the Neighbor Cache Entry for the source of this RA.
            //
            NCE = FindOrCreateNeighbor(IF, &Packet->IP->Source);
            if (NCE != NULL) {
                //
                // Update the Neighbor Cache Entry for the source of this RA.
                //
                // REVIEW: We deviate from the spec here. The spec says
                // that if you receive an RS from a Source, then you MUST
                // set the IsRouter flag for that Source to FALSE.
                // However consider a node which is not advertising
                // but it is forwarding. Such a node might send an RS
                // but IsRouter should be TRUE for that node.
                //
                NeighborCacheUpdate(NCE, SourceLinkAddress, FALSE);
                ReleaseNCE(NCE);
            }
        }
    }

    //
    // Calculate next unsolicited RA time.
    // NB: Although we checked IF_FLAG_ADVERTISES above,
    // the situation could be different now.
    //
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    if (IF->RATimer != 0) {
        SendRouterAdvert = TRUE;

        //
        // Re-arm the timer.
        //
        IF->RATimer = RandomNumber(MinRtrAdvInterval, MaxRtrAdvInterval);

        //
        // If we are in "fast" mode, then ensure
        // that the next RA is sent quickly.
        //
        if (IF->RACount != 0) {
            IF->RACount--;
            if (IF->RATimer > MAX_INITIAL_RTR_ADVERT_INTERVAL)
                IF->RATimer = MAX_INITIAL_RTR_ADVERT_INTERVAL;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendRouterAdvert) {
        //
        // Send a Router Advertisement immediately.
        // BUGBUG: Should rate-limit. Should have a small random delay.
        //
        RouterAdvertSend(IF);
    }
    return NULL;
}


//* RouterAdvertReceive - Handle Router Advertisement messages.
//
//  Validate message, update Default Router list, On-Link Prefix list,
//  perform address auto-configuration.
//  See sections 6.1.2, 6.3.4 of RFC 1970.
//
char *
RouterAdvertReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6Receive.
    ICMPv6Header *ICMP)         // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    uint Flags, CurHopLimit, RouterLifetime;
    uint AdvReachableTime, RetransTimer;
    void *SourceLinkAddress;
    NeighborCacheEntry *NCE;
    IPv6Addr Addr;

    //
    // Ignore the advertisement if this is an advertising interface.
    //
    if (IF->Flags & IF_FLAG_ADVERTISES)
        return NULL;

    //
    // Validate the advertisement.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the length, source address, hop limit, and ICMP code.
    //
    if (Packet->IP->HopLimit != 255) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrint(("RouterAdvertReceive: "
                 "Received a routed router advertisement\n"));
        return NULL;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted router advertisement.  Drop the packet.
        //
        KdPrint(("RouterAdvertReceive: "
                 "Received a corrupted router advertisement\n"));
        return NULL;
    }
    if (!IsLinkLocal(&Packet->IP->Source)) {
        //
        // Source address should always be link-local. Drop the packet.
        //
        KdPrint(("RouterAdvertReceive: "
                 "Non-link-local source in router advertisement\n"));
        return NULL;
    }

    //
    // Pull CurHopLimit, Flags, RouterLifetime,
    // AdvReachableTime, RetransTimer out of the packet.
    // REVIEW - Define a structure for this format?
    //
    if (Packet->ContigSize < 12) {
        if (PacketPullup(Packet, 12) == 0) {
            //
            // Packet too short to contain minimal RA.
            //
            KdPrint(("RouterAdvertReceive: "
                     "Received a too short router advertisement\n"));
            return NULL;
        }
    }

    CurHopLimit = ((uchar *)Packet->Data)[0];
    Flags = ((uchar *)Packet->Data)[1];
    RouterLifetime = net_short(((ushort *)Packet->Data)[1]);
    AdvReachableTime = net_long(((ulong *)Packet->Data)[1]);
    RetransTimer = net_long(((ulong *)Packet->Data)[2]);
    AdjustPacketParams(Packet, 12);

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (Packet->ContigSize != Packet->TotalSize) {
        if (PacketPullup(Packet, Packet->TotalSize) == 0) {
            // Can only fail if we run out of memory.
            return NULL;
        }
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a source link-layer address option.
    // Also sanity-check the options before doing anything permanent.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrint(("RouterAdvertReceive: "
                     "Received RA option with with bogus length\n"));
            return NULL;
        }

        switch (*(uchar *)Packet->Data) {
        case ND_OPTION_SOURCE_LINK_LAYER_ADDRESS:
            //
            // Parse the link-layer address option.
            //
            SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                 (uchar *)Packet->Data);
            if (SourceLinkAddress == NULL) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrint(("RouterAdvertReceive: "
                         "Received RA with bogus ll option\n"));
                return NULL;
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one.  We sanity-check all the options.
            //
            break;

        case ND_OPTION_MTU:
            //
            // Sanity-check the option.
            //
            if (OptionLength != 8) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrint(("RouterAdvertReceive: "
                         "Received RA with bogus mtu option\n"));
                return NULL;
            }
            break;

        case ND_OPTION_PREFIX_INFORMATION: {
            NDOptionPrefixInformation *option =
                (NDOptionPrefixInformation *)Packet->Data;

            //
            // Sanity-check the option.
            //
            if ((OptionLength != 32) ||
                (option->PrefixLength > 128)) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrint(("RouterAdvertReceive: Received RA with bogus prefix "
                         "information option\n"));
                return NULL;
            }
            break;
        }
        }

        //
        // Move forward to next option.
        // Note that we are not updating TotalSize here,
        // so we can use it below to back up.
        //
        (uchar *)Packet->Data += OptionLength;
        Packet->ContigSize -= OptionLength;
    }

    //
    // Reset Data pointer & ContigSize.
    //
    (uchar *)Packet->Data -= Packet->TotalSize;
    Packet->ContigSize += Packet->TotalSize;

    //
    // Get the Neighbor Cache Entry for the source of this RA.
    //
    NCE = FindOrCreateNeighbor(IF, &Packet->IP->Source);
    if (NCE == NULL) {
        //
        // Couldn't find or create NCE.  Drop the packet.
        //
        return NULL;
    }

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // If we just reconnected this interface,
    // then give all state learned from auto-configuration
    // a small "accelerated" lifetime.
    // The processing below might extend accelerated lifetimes.
    //
    if (IF->Flags & IF_FLAG_MEDIA_RECONNECTED) {
        KdPrint(("RouterAdvertReceive(IF %x) - reconnecting\n", IF));

        IF->Flags &= ~IF_FLAG_MEDIA_RECONNECTED;

        //
        // Reset auto-configured address lifetimes.
        //
        AddrConfReset(IF, 2 * MAX_RA_DELAY_TIME);

        //
        // Similarly, reset auto-configured routes.
        //
        RouteTableReset(IF, 2 * MAX_RA_DELAY_TIME);
    }

    //
    // Stop sending router solicitations for this interface.
    // Note that we should always send at least one router solicitation,
    // even if we receive an unsolicited router advertisement first.
    //
    if (RouterLifetime != 0) {
        if (IF->RSCount > 0)
            IF->RSTimer = 0;
    }

    //
    // Update the BaseReachableTime and ReachableTime.
    // NB: We use a lock for coordinated updates, but other code
    // reads the ReachableTime field without a lock.
    //
    if ((AdvReachableTime != 0) &&
        (AdvReachableTime != IF->BaseReachableTime)) {

        IF->BaseReachableTime = AdvReachableTime;
        IF->ReachableTime = CalcReachableTime(AdvReachableTime);
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    //
    // Update the Neighbor Cache Entry for the source of this RA.
    //
    NeighborCacheUpdate(NCE, SourceLinkAddress, TRUE);

    //
    // Update the Default Router List.
    // Note that router lifetimes on the wire,
    // unlike prefix lifetimes, can not be infinite.
    //
    RouterLifetime = ConvertSecondsToTicks(RouterLifetime);
    if (RouterLifetime == INFINITE_LIFETIME)
        RouterLifetime--;
    RouteTableUpdate(IF, NCE,
                     &UnspecifiedAddr, 0, 0, RouterLifetime,
                     DEFAULT_NEXT_HOP_PREF, FALSE, FALSE);

    //
    // Update the hop limit for the interface.
    // NB: We rely on loads/stores of the CurHopLimit field being atomic.
    //
    if (CurHopLimit != 0) {
        IF->CurHopLimit = CurHopLimit;
    }

    //
    // Update the RetransTimer field.
    // NB: We rely on loads/stores of this field being atomic.
    //
    if (RetransTimer != 0)
        IF->RetransTimer = ConvertMillisToTicks(RetransTimer);

    //
    // Process any LinkMTU, PrefixInformation options.
    //

    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // The option length was validated in the first pass
        // over the options, above.
        //
        OptionLength = *((uchar *)Packet->Data + 1) << 3;

        switch (*(uchar *)Packet->Data) {
        case ND_OPTION_MTU: {
            NDOptionMTU *option = (NDOptionMTU *)Packet->Data;
            uint LinkMTU = net_long(option->MTU);

            UpdateLinkMTU(IF, LinkMTU);
            break;
        }

        case ND_OPTION_PREFIX_INFORMATION: {
            NDOptionPrefixInformation *option =
                (NDOptionPrefixInformation *)Packet->Data;
            uint PrefixLength, ValidLifetime, PreferredLifetime;
            IPv6Addr Prefix;

            //
            // Extract the prefix length and prefix from option.  We
            // MUST ignore any bits in the prefix after the prefix length.
            // REVIEW - RouteTableUpdate and SitePrefixUpdate also
            // ignore any excess bits. Is CopyPrefix needed here?
            //
            PrefixLength = option->PrefixLength;  // In bits.
            if (PrefixLength > 128) {
                //
                // REVIEW: What to do here.  Currently we ignore the option.
                //
                KdPrint(("RouterAdvertReceive: Bogus prefix length of %u\n",
                         PrefixLength));
                break;
            }
            CopyPrefix(&Prefix, &option->Prefix, PrefixLength);

            ValidLifetime = net_long(option->ValidLifetime);
            ValidLifetime = ConvertSecondsToTicks(ValidLifetime);
            PreferredLifetime = net_long(option->PreferredLifetime);
            PreferredLifetime = ConvertSecondsToTicks(PreferredLifetime);

            //
            // Silently ignore link-local and multicast prefixes.
            // REVIEW - Is this actually the required check?
            //
            if (IsLinkLocal(&Prefix) || IsMulticast(&Prefix))
                break;

            //
            // Generally at least one flag bit is set,
            // but we must process them independently.
            //

            if (option->Flags & ND_PREFIX_FLAG_ON_LINK)
                RouteTableUpdate(IF, NULL,
                                 &Prefix, PrefixLength, 0, ValidLifetime,
                                 DEFAULT_ON_LINK_PREF, FALSE, FALSE);

            if (option->Flags & ND_PREFIX_FLAG_ROUTE)
                RouteTableUpdate(IF, NCE,
                                 &Prefix, PrefixLength, 0, ValidLifetime,
                                 DEFAULT_NEXT_HOP_PREF, FALSE, FALSE);

            if (option->Flags & ND_PREFIX_FLAG_SITE_PREFIX) {
                uint SitePrefixLength = option->SitePrefixLength;

                //
                // Ignore site-local prefixes.
                // REVIEW - Is this actually the required check?
                // Ignore if the Site Prefix Length is zero.
                //
                if (!IsSiteLocal(&Prefix) && (SitePrefixLength != 0))
                    SitePrefixUpdate(IF,
                                     &Prefix, SitePrefixLength,
                                     ValidLifetime);
            }

            if (option->Flags & ND_PREFIX_FLAG_AUTONOMOUS) {
                //
                // Attempt autonomous address-configuration.
                //
                if (PreferredLifetime > ValidLifetime) {
                    //
                    // MAY log a system management error.
                    //
                    KdPrint(("RouterAdvertReceive: "
                             "Bogus valid/preferred lifetimes\n"));
                }
                else if (PrefixLength != 64) {
                    //
                    // If the sum of the prefix length and the interface
                    // identifier (always 64 bits in our implementation)
                    // is not 128 bits, we MUST ignore the prefix option.
                    // MAY log a system management error.
                    //
                    KdPrint(("RouterAdvertReceive: Got prefix length of %u, "
                             "must be 64 for auto-config\n", PrefixLength));
                }
                else {
                    IPv6Addr NewAddr;

                    //
                    // Create the new address from
                    // the prefix & interface token.
                    //
                    NewAddr = Prefix;
                    (*IF->CreateToken)(IF->LinkContext,
                                       IF->LinkAddress,
                                       &NewAddr);

                    //
                    // Sanity-check the address.
                    // REVIEW - Should these checks be done earlier,
                    // on the prefix?
                    //
                    if (IsUniqueAddress(&NewAddr) && !IsLoopback(&NewAddr)) {
                        //
                        // BUGBUG - If the RA is authenticated,
                        // then we should tell AddrConfUpdate.
                        //
                        (void) AddrConfUpdate(IF, &NewAddr, TRUE,
                                              ValidLifetime, PreferredLifetime,
                                              FALSE, NULL);
                    }
                    else {
                        KdPrint(("RouterAdvertReceive: "
                                 "formed bad address %s\n",
                                 FormatV6Address(&NewAddr)));
                    }
                }
            }
            break;
        }
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // Done with packet.
    //
    ReleaseNCE(NCE);
    return NULL;
}


//* NeighborSolicitReceive - Handle Neighbor Solicitation messages.
//
//  Validate message, update ND cache, and reply with Neighbor Advertisement.
//  See section 7.2.4 of RFC 1970.
//
char *
NeighborSolicitReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6Receive.
    ICMPv6Header *ICMP)         // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    IPv6Addr *TargetAddress;
    void *SourceLinkAddress;
    NDIS_STATUS Status;
    NDIS_PACKET *AdvertPacket;
    uint Offset;
    uint PayloadLength;
    void *Mem;
    uint MemLen;
    IPv6Header *AdvertIP;
    ICMPv6Header *AdvertICMP;
    ulong Flags;
    void *AdvertTargetOption;
    IPv6Addr *AdvertTargetAddress;
    void *DestLinkAddress;
    NetTableEntryOrInterface *TargetNTEorIF;
    ushort TargetType;

    //
    // Validate the solicitation.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if (Packet->IP->HopLimit != 255) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrint(("NeighborSolicitReceive: "
                 "Received a routed neighbor solicitation\n"));
        return NULL;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted neighbor solicitation message.  Drop the packet.
        //
        KdPrint(("NeighborSolicitReceive: "
                 "Received a corrupted neighbor solicitation\n"));
        return NULL;
    }

    //
    // Next we have a 4 byte reserved field, then an IPv6 addresses.
    //
    if (Packet->ContigSize < sizeof(IPv6Addr) + 4) {
        if (PacketPullup(Packet, sizeof(IPv6Addr) + 4) == 0) {
            //
            // Packet too short to contain minimal solicitation.
            //
            KdPrint(("NeighborSolicitReceive: "
                     "Received a too short solicitation\n"));
            return NULL;
        }
    }

    //
    // Skip over 4 byte "Reserved" field, ignoring whatever may be in it.
    // Get a pointer to the target address which follows, then skip over that.
    //
    TargetAddress = (IPv6Addr *) ((uchar *)Packet->Data + 4);
    AdjustPacketParams(Packet, 4 + sizeof(IPv6Addr));

    //
    // See if we're the target of the solicitation.  If the target address is
    // not a unicast or anycast address assigned to receiving interface,
    // then we must silently drop the packet.
    //
    TargetNTEorIF = FindAddressOnInterface(IF, TargetAddress, &TargetType);
    if (TargetNTEorIF == NULL) {
        KdPrint(("NeighborSolicitReceive: Did not find target address\n"));
        return NULL;
    }
    else if (TargetType == ADE_MULTICAST) {
        KdPrint(("NeighborSolicitReceive: Target address not uni/anycast\n"));
        goto Return;
    }

    ASSERT(((TargetType == ADE_UNICAST) &&
            IsNTE(TargetNTEorIF) &&
            IP6_ADDR_EQUAL(&CastToNTE(TargetNTEorIF)->Address,
                           TargetAddress)) ||
           (TargetType == ADE_ANYCAST));

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (Packet->ContigSize != Packet->TotalSize) {
        if (PacketPullup(Packet, Packet->TotalSize) == 0) {
            // Can only fail if we run out of memory.
            return NULL;
        }
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // We may have a source link-layer address option present.
    // Check for it and silently ignore all others.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrint(("NeighborSolicitReceive: "
                     "Received option with bogus length\n"));
            goto Return;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_SOURCE_LINK_LAYER_ADDRESS) {
            //
            // Parse the link-layer address option.
            //
            SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                 (uchar *)Packet->Data);
            if (SourceLinkAddress == NULL) {
                //
                // Invalid option format. Drop the packet.
                //
                KdPrint(("NeighborSolicitReceive: "
                         "Received bogus ll option\n"));
                goto Return;
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one. We must sanity-check all option lengths.
            //
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    if (IsUnspecified(&Packet->IP->Source)) {
        //
        // Checks that were added post-RFC 1970.
        //

        if (!IsSolicitedNodeMulticast(&Packet->IP->Dest)) {
            KdPrint(("NeighborSolicitReceive: NS with null src, bad dst\n"));
            goto Return;
        }

#if 0
        //
        // REVIEW - For backwards-compatibility, disable this one.
        //
        if (SourceLinkAddress != NULL) {
            KdPrint(("NeighborSolicitReceive: NS with null src and SLA\n"));
            goto Return;
        }
#endif
    }

    //
    // We've received and parsed a valid neighbor solicitation.
    //
    // First, we check our Duplicate Address Detection state.
    // (See RFC 2461 section 5.4.3.)
    //
    if ((TargetType != ADE_ANYCAST) && !IsValidNTE(CastToNTE(TargetNTEorIF))) {
        NetTableEntry *TargetNTE = CastToNTE(TargetNTEorIF);

        if ((TargetNTE->DADState == DAD_STATE_TENTATIVE) &&
            IsUnspecified(&Packet->IP->Source)) {
            //
            // This appears to be a DAD solicit for our own tentative address.
            // However it might be a loopback of the DAD solicit
            // that we sent ourselves. Use DADCountLB to detect this.
            //
            if (TargetNTE->DADCountLB == 0) {
                //
                // We've received more DAD solicits than we sent.
                // Must be a duplicate address out there.
                //
                TargetNTE->DADState = DAD_STATE_DUPLICATE;
                TargetNTE->DADTimer = 0;
                KdPrint(("NeighborSolicitReceive: DAD found duplicate!\n"));
            }
            else {
                //
                // Remember that we received a DAD solicit,
                // presumably a loopback of our own packet.
                //
                TargetNTE->DADCountLB--;
            }
        }

        //
        // In any case, we should not respond to the solicit.
        //
        goto Return;
    }

    if (IsNTE(Packet->NTEorIF) && !IsValidNTE(CastToNTE(Packet->NTEorIF)))
        goto Return;

    //
    // Update the Neighbor Cache Entry for the source of this solicit.
    // In this case, only bother if the SourceLinkAddress is present;
    // if it's not, NeighborCacheUpdate won't do anything.
    //
    if (!IsUnspecified(&Packet->IP->Source) && (SourceLinkAddress != NULL)) {
        NeighborCacheEntry *NCE;

        NCE = FindOrCreateNeighbor(IF, &Packet->IP->Source);
        if (NCE != NULL) {
            NeighborCacheUpdate(NCE, SourceLinkAddress, FALSE);
            ReleaseNCE(NCE);
        }
    }

    //
    // Send a solicited neighbor advertisement back to the source.
    //
    // Interface to send on is 'IF' (I/F solicitation received on).
    //

    //
    // Allocate a packet for advertisement.
    // In addition to the 24 bytes for the solicit proper, leave space
    // for target link-layer address option (round option length up to
    // 8-byte multiple).
    //
    PayloadLength = 24 + ((IF->LinkAddressLength + 2 + 7) & ~7);
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &AdvertPacket, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        goto Return;
    }

    //
    // Prepare IP header of advertisement.
    //
    AdvertIP = (IPv6Header *)((uchar *)Mem + Offset);
    AdvertIP->VersClassFlow = IP_VERSION;
    AdvertIP->PayloadLength = net_short((ushort)PayloadLength);
    AdvertIP->NextHeader = IP_PROTOCOL_ICMPv6;
    AdvertIP->HopLimit = 255;

    //
    // Prepare ICMP header.
    //
    AdvertICMP = (ICMPv6Header *)(AdvertIP + 1);
    AdvertICMP->Type = ICMPv6_NEIGHBOR_ADVERT;
    AdvertICMP->Code = 0;
    AdvertICMP->Checksum = 0;
    Flags = 0;
    if (IF->Flags & IF_FLAG_FORWARDS)
        Flags |= ND_NA_FLAG_ROUTER;

    //
    // Check source of solicitation to see where we should direct our
    // advertisement (and determine what type of advertisement to send).
    //
    if (IsUnspecified(&Packet->IP->Source)) {
        //
        // Solicitation came from an unspecified address (presumably a node
        // undergoing initialization), so we need to multicast our response.
        // We also don't set the solicited flag since we can't specify the
        // specific node our advertisement is directed at.
        //
        AdvertIP->Dest = AllNodesOnLinkAddr;

        DestLinkAddress = alloca(IF->LinkAddressLength);
        (*IF->ConvertMCastAddr)(IF->LinkContext, &AllNodesOnLinkAddr,
                                DestLinkAddress);
    } else {
        //
        // We know who sent the solicitation, so we can respond by unicasting
        // our solicited advertisement back to the soliciter.
        //
        AdvertIP->Dest = Packet->IP->Source;
        Flags |= ND_NA_FLAG_SOLICITED;

        //
        // Find link-level address for above.  We should have it cached (note
        // that it will be cached if it just came in on this solicitation).
        //
        if (SourceLinkAddress != NULL) {
            //
            // This is faster than checking the ND cache and
            // it should be the common case.
            //
            DestLinkAddress = SourceLinkAddress;
        } else {
            DestLinkAddress = alloca(IF->LinkAddressLength);

            if (!NeighborCacheSearch(IF, &Packet->IP->Source,
                                     DestLinkAddress)) {
                //
                // No entry found in ND cache.
                // REVIEW: Queuing for ND does not seem correct -
                // just drop the solicit.
                //
                KdPrint(("NeighborSolicitReceive: Didn't find NCE!?!\n"));
                IPv6FreePacket(AdvertPacket);
                goto Return;
            }
        }
    }

    if (IsNTE(TargetNTEorIF)) {
        //
        // Take our source address from the target NTE.
        //
        AdvertIP->Source = CastToNTE(TargetNTEorIF)->Address;
    }
    else {
        NetTableEntry *BestNTE;

        //
        // Find the best source address for the destination.
        //
        BestNTE = FindBestSourceAddress(IF, &AdvertIP->Dest);
        if (BestNTE == NULL)
            goto Return;
        AdvertIP->Source = BestNTE->Address;
        ReleaseNTE(BestNTE);
    }

    //
    // Take the target address from the solicitation.
    //
    AdvertTargetAddress = (IPv6Addr *)((char *)AdvertICMP +
                                       sizeof(ICMPv6Header) + sizeof(ulong));
    *AdvertTargetAddress = *TargetAddress;

    //
    // Always include target link-layer address option.
    //
    AdvertTargetOption = (void *)((char *)AdvertTargetAddress +
                                  sizeof(IPv6Addr));

    ((uchar *)AdvertTargetOption)[0] = ND_OPTION_TARGET_LINK_LAYER_ADDRESS;
    ((uchar *)AdvertTargetOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

    (*IF->WriteLLOpt)(IF->LinkContext, AdvertTargetOption, IF->LinkAddress);

    //
    // We set the override flag unless the target is an anycast address.
    //
    if (TargetType != ADE_ANYCAST)
        Flags |= ND_NA_FLAG_OVERRIDE;

    //
    // Fill in Flags field in advertisement.
    //
    *(ulong *)((char *)AdvertICMP + sizeof(ICMPv6Header)) = net_long(Flags);

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    AdvertICMP->Checksum = ChecksumPacket(
        AdvertPacket, Offset + sizeof *AdvertIP, NULL, PayloadLength,
        &AdvertIP->Source, &AdvertIP->Dest,
        IP_PROTOCOL_ICMPv6);

    //
    // Call the interface's send routine to transmit the packet.
    //
    // BUGBUG: For anycast target addresses, should delay.
    //
    (*IF->Transmit)(IF->LinkContext,
                    AdvertPacket, Offset, DestLinkAddress);

  Return:
    if (IsNTE(TargetNTEorIF))
        ReleaseNTE(CastToNTE(TargetNTEorIF));
    return NULL;
}


//* NeighborAdvertReceive - Handle Neighbor Advertisement messages.
//
// Validate message and update neighbor cache if necessary.
//
char *
NeighborAdvertReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6Receive.
    ICMPv6Header *ICMP)         // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    ulong Flags;
    IPv6Addr *TargetAddress;
    void *TargetLinkAddress;
    NeighborCacheEntry *NCE;
    NetTableEntryOrInterface *TargetNTEorIF;
    ushort TargetType;

    //
    // Validate the advertisement.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if (Packet->IP->HopLimit != 255) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrint(("NeighborAdvertReceive: "
                 "Received a routed neighbor advertisement\n"));
        return NULL;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted neighbor advertisement message.  Drop the packet.
        //
        KdPrint(("NeighborAdvertReceive: "
                 "Received a corrupted neighbor advertisement\n"));
        return NULL;
    }

    //
    // Next we have a 4 byte reserved field, then an IPv6 addresses.
    //
    if (Packet->ContigSize < sizeof(IPv6Addr) + 4) {
        if (PacketPullup(Packet, sizeof(IPv6Addr) + 4) == 0) {
            //
            // Packet too short to contain minimal advertisement.
            //
            KdPrint(("NeighborAdvertReceive: "
                     "Received a too short advertisement\n"));
            return NULL;
        }
    }

    //
    // Next 4 bytes contains 3 bits of flags and 29 bits of "Reserved".
    // Pull this out as a 32 bit long, we're required to ignore the "Reserved"
    // bits.  Get a pointer to the target address which follows, then skip
    // over that.
    //
    Flags = net_long(*(ulong *)Packet->Data);
    TargetAddress = (IPv6Addr *)((ulong *)Packet->Data + 1);
    AdjustPacketParams(Packet, sizeof(ulong) + sizeof(IPv6Addr));

    //
    // Check that the target address is not a multicast address.
    //
    if (IsMulticast(TargetAddress)) {
        //
        // Nodes are not supposed to send neighbor advertisements for
        // multicast addresses.  We're required to drop any such advertisements
        // we receive.
        //
        KdPrint(("NeighborAdvertReceive: "
                 "Received advertisement for a multicast address\n"));
        return NULL;
    }

    //
    // Check that Solicited flag is zero
    // if the destination address is multicast.
    //
    if ((Flags & ND_NA_FLAG_SOLICITED) && IsMulticast(&Packet->IP->Dest)) {
        //
        // We're required to drop the advertisement.
        //
        KdPrint(("NeighborAdvertReceive: "
                 "Received solicited advertisement to a multicast address\n"));
        return NULL;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (Packet->ContigSize != Packet->TotalSize) {
        if (PacketPullup(Packet, Packet->TotalSize) == 0) {
            // Can only fail if we run out of memory.
            return NULL;
        }
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a target link-layer address option.
    //
    TargetLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrint(("NeighborAdvertReceive: "
                     "Received option with bogus length\n"));
            return NULL;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_TARGET_LINK_LAYER_ADDRESS) {
            //
            // Parse the link-layer address option.
            //
            TargetLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                 (uchar *)Packet->Data);
            if (TargetLinkAddress == NULL) {
                //
                // Invalid option format. Drop the packet.
                //
                KdPrint(("NeighborAdvertReceive: "
                         "Received bogus ll option\n"));
                return NULL;
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one. We must sanity-check all option lengths.
            //
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We've received and parsed a valid neighbor advertisement.
    //
    // First, we check our Duplicate Address Detection state.
    // (See RFC 2461 section 5.4.4.)
    //
    TargetNTEorIF = FindAddressOnInterface(IF, TargetAddress, &TargetType);
    if ((TargetNTEorIF != NULL) && IsNTE(TargetNTEorIF)) {
        NetTableEntry *TargetNTE = CastToNTE(TargetNTEorIF);

        if (!IsValidNTE(TargetNTE)) {
            if ((TargetNTE->DADState == DAD_STATE_TENTATIVE) &&
                IP6_ADDR_EQUAL(TargetAddress, &TargetNTE->Address)) {
                //
                // Someone out there is already using our address;
                // they responded to our solicit.
                //
                TargetNTE->DADState = DAD_STATE_DUPLICATE;
                TargetNTE->DADTimer = 0;
                KdPrint(("NeighborAdvertReceive: DAD found duplicate!\n"));
            }

            //
            // In any case, no further processing is required.
            //
            ReleaseNTE(TargetNTE);
            return NULL;
        }

        ReleaseNTE(TargetNTE);
    }

    if (IsNTE(Packet->NTEorIF) && !IsValidNTE(CastToNTE(Packet->NTEorIF)))
        return NULL;

    //
    // Process the advertisement.
    //
    NeighborCacheAdvert(IF, TargetAddress, TargetLinkAddress, Flags);
    return NULL;
}


//* RedirectReceive - Handle Redirect messages.
//
//  See RFC 1970, sections 8.1 and 8.3.
//
char *
RedirectReceive(
    IPv6Packet *Packet,      // Packet handed to us by ICMPv6Receive.
    ICMPv6Header *ICMP)      // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    IPv6Addr *TargetAddress;
    void *TargetLinkAddress;
    IPv6Addr *DestinationAddress;
    NeighborCacheEntry *TargetNCE;
    IP_STATUS Status;

    //
    // Ignore the redirect if this is a forwarding interface.
    //
    if (IF->Flags & IF_FLAG_FORWARDS)
        return NULL;

    //
    // Validate the redirect.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if (Packet->IP->HopLimit != 255) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrint(("RedirectReceive: Received a routed redirect\n"));
        return NULL;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted redirect message.  Drop the packet.
        //
        KdPrint(("RedirectReceive: Received a corrupted redirect\n"));
        return NULL;
    }

    //
    // Check that the source address is a link-local address.
    // We need a link-local source address to identify the router
    // that sent the redirect.
    //
    if (!IsLinkLocal(&Packet->IP->Source)) {
        //
        // Drop the packet.
        //
        KdPrint(("RedirectReceive: "
                 "Received redirect from non-link-local source\n"));
        return NULL;
    }

    //
    // Next we have a 4 byte reserved field, then two IPv6 addresses.
    //
    if (Packet->ContigSize < 2 * sizeof(IPv6Addr) + 4) {
        if (PacketPullup(Packet, 2 * sizeof(IPv6Addr) + 4) == 0) {
            //
            // Packet too short to contain minimal redirect.
            //
            KdPrint(("RedirectReceive: Received a too short redirect\n"));
            return NULL;
        }
    }

    //
    // Skip over the 4 byte reserved field.
    // Pick up the Target and Destination addresses.
    //
    AdjustPacketParams(Packet, 4);
    TargetAddress = (IPv6Addr *)Packet->Data;
    DestinationAddress = TargetAddress + 1;
    AdjustPacketParams(Packet, 2 * sizeof(IPv6Addr));

    //
    // Check that the destination address is not a multicast address.
    //
    if (IsMulticast(DestinationAddress)) {
        //
        // Drop the packet.
        //
        KdPrint(("RedirectReceive: "
                 "Received redirect for multicast address\n"));
        return NULL;
    }

    //
    // Check that either the target address is link-local
    // (redirecting to a router) or the target and the destination
    // are the same (redirecting to an on-link destination).
    //
    if (!IsLinkLocal(TargetAddress) &&
        !IP6_ADDR_EQUAL(TargetAddress, DestinationAddress)) {
        //
        // Drop the packet.
        //
        KdPrint(("RedirectReceive: "
                 "Received redirect with non-link-local target\n"));
        return NULL;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (Packet->ContigSize != Packet->TotalSize) {
        if (PacketPullup(Packet, Packet->TotalSize) == 0) {
            // Can only fail if we run out of memory.
            return NULL;
        }
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a target link-layer address option,
    // checking that all included options have a valid length.
    //
    TargetLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrint(("RedirectReceive: Received option with length %u "
                     "while we only have %u data\n", OptionLength,
                     Packet->ContigSize));
            return NULL;
        }

        if (*(uchar *)Packet->Data == ND_OPTION_TARGET_LINK_LAYER_ADDRESS) {
            //
            // Parse the link-layer address option.
            //
            TargetLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                 (uchar *)Packet->Data);
            if (TargetLinkAddress == NULL) {
                //
                // Invalid option format. Drop the packet.
                //
                KdPrint(("RedirectReceive: Received bogus ll option\n"));
                return NULL;
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one. We must sanity-check all option lengths.
            //
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We have a valid redirect message (except for checking that the
    // source of the redirect is the current first-hop neighbor
    // for the destination - RedirectRouteCache does that).
    //
    // First we get an NCE for the target of the redirect.  Then we
    // update the route cache.  If RedirectRouteCache doesn't invalidate
    // the redirect, then we update the neighbor cache.
    //

    //
    // Get the Neighbor Cache Entry for the target.
    //
    TargetNCE = FindOrCreateNeighbor(IF, TargetAddress);
    if (TargetNCE == NULL) {
        //
        // Couldn't find or create NCE.  Drop the packet.
        //
        return NULL;
    }

    //
    // Update the route cache to reflect this redirect.
    //
    Status = RedirectRouteCache(&Packet->IP->Source, DestinationAddress,
                                IF, TargetNCE);

    if (Status == IP_SUCCESS) {
        //
        // Update the Neighbor Cache Entry for the target.
        // We know the target is a router if the target address
        // is not the same as the destination address.
        //
        NeighborCacheUpdate(TargetNCE, TargetLinkAddress,
                            !IP6_ADDR_EQUAL(TargetAddress,
                                            DestinationAddress));
    }

    ReleaseNCE(TargetNCE);
    return NULL;
}


//* RouterSolicitSend
//
//  Sends a Router Solicitation.
//  The solicit is always sent to the all-routers multicast address.
//  Chooses a valid source address for the interface.
//
//  Called with no locks held.
//
void
RouterSolicitSend(Interface *IF)
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header *IP;
    ICMPv6Header *ICMP;
    void *SourceOption;
    void *LLDest;
    IPv6Addr Source;

    //
    // Allocate a packet for solicitation.
    // In addition to the 8 bytes for the solicit proper, leave space
    // for source link-layer address option (round option length up to
    // 8-byte multiple) IF we have a valid source address.
    //
    PayloadLength = 8;
    if (GetLinkLocalAddress(IF, &Source))
        PayloadLength += (IF->LinkAddressLength + 2 + 7) &~ 7;
    else
        Source = UnspecifiedAddr;
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = Source;
    IP->Dest = AllRoutersOnLinkAddr;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header *)(IP + 1);
    ICMP->Type = ICMPv6_ROUTER_SOLICIT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Must zero the reserved field.
    //
    *(ulong *)(ICMP + 1) = 0;

    if (PayloadLength != 8) {
        //
        // Include source link-layer address option.
        //
        SourceOption = (void *)((ulong *)(ICMP + 1) + 1);

        ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
        ((uchar *)SourceOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);
    }

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    ICMP->Checksum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        &IP->Source, &IP->Dest,
        IP_PROTOCOL_ICMPv6);

    //
    // Calculate the link-level destination address.
    // (The IPv6 destination is a multicast address.)
    //
    LLDest = alloca(IF->LinkAddressLength);
    (*IF->ConvertMCastAddr)(IF->LinkContext, &IP->Dest, LLDest);

    //
    // Call the interface's send routine to transmit the packet.
    //
    (*IF->Transmit)(IF->LinkContext, Packet, Offset, LLDest);
}

//* RouterSolicitTimeout
//
//  Called periodically from IPv6Timeout to handle timeouts
//  for sending router solicitations for an interface.
//
//  Callable from DPC context, not from thread context.
//
void
RouterSolicitTimeout(Interface *IF)
{
    int SendRouterSolicit = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (IF->RSTimer != 0) {
        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--IF->RSTimer == 0) {
            //
            // Timer went off. Generate a router solicitation.
            //
            IF->RSCount++;
            SendRouterSolicit = TRUE;

            //
            // Re-arm the timer, if we should keep trying.
            //
            if (IF->RSCount < MAX_RTR_SOLICITATIONS)
                IF->RSTimer = RTR_SOLICITATION_INTERVAL;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendRouterSolicit)
        RouterSolicitSend(IF);
}

//* NeighborSolicitSend0
//
//  Low-level version of NeighborSolicitSend -
//  uses explicit source/destination/target addresses
//  instead of an NCE.
//
void
NeighborSolicitSend0(
    Interface *IF,      // Interface for the solicit
    void *LLDest,       // Link-level destination
    IPv6Addr *Source,   // IP-level source
    IPv6Addr *Dest,     // IP-level destination
    IPv6Addr *Target)   // IP-level target of the solicit
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header *IP;
    ICMPv6Header *ICMP;
    IPv6Addr *TargetAddress;
    void *SourceOption;

    //
    // Allocate a packet for solicitation.
    // In addition to the 24 bytes for the solicit proper, leave space
    // for source link-layer address option (round option length up to
    // 8-byte multiple) IF we have a valid source address.
    //
    PayloadLength = 24;
    if (!IsUnspecified(Source))
        PayloadLength += (IF->LinkAddressLength + 2 + 7) &~ 7;
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = *Source;
    IP->Dest = *Dest;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header *)(IP + 1);
    ICMP->Type = ICMPv6_NEIGHBOR_SOLICIT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Must zero the reserved field.
    //
    *(ulong *)(ICMP + 1) = 0;

    //
    // Initialize the target address.
    //
    TargetAddress = (IPv6Addr *)((ulong *)(ICMP + 1) + 1);
    *TargetAddress = *Target;

    if (PayloadLength != 24) {
        //
        // Include source link-layer address option.
        //
        SourceOption = (void *)(TargetAddress + 1);

        ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
        ((uchar *)SourceOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);
    }

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    ICMP->Checksum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        &IP->Source, &IP->Dest,
        IP_PROTOCOL_ICMPv6);

    //
    // Call the interface's send routine to transmit the packet.
    //
    // BUGBUG: For anycast target addresses, should delay.
    //
    (*IF->Transmit)(IF->LinkContext, Packet, Offset, LLDest);
}

//* NeighborSolicitSend - Send a Neighbor Solicitation message.
//
//  Chooses source/destination/target addresses depending
//  on the NCE state and sends the solicit packet.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
NeighborSolicitSend(
    NeighborCacheEntry *NCE,  // Neighbor to solicit.
    IPv6Addr *Source)         // Source address to send from (optional).
{
    Interface *IF = NCE->IF;
    IPv6Addr Dest;
    void *LLDest;

    //
    // Check Neighbor Discovery protocol state of our neighbor in order to
    // determine whether we should multicast or unicast our solicitation.
    //
    // Note that we do not take the interface lock to make this check.
    // The worst that can happen is that we'll think the NDState is not
    // incomplete and then use a bogus/changing LinkAddress.
    // This is rare enough and benign enough to be OK.
    // Similar reasoning in IPv6Send0.
    //
    if (NCE->NDState == ND_STATE_INCOMPLETE) {
        //
        // This is our initial solicitation to this neighbor, so we don't
        // have a link-layer address cached.  Multicast our solicitation
        // to the solicited-node multicast address corresponding to our
        // neighbor's address.
        //
        CreateSolicitedNodeMulticastAddress(&NCE->NeighborAddress, &Dest);

        LLDest = alloca(IF->LinkAddressLength);
        (*IF->ConvertMCastAddr)(IF->LinkContext, &Dest, LLDest);
    } else {
        //
        // We have a cached link-layer address that has gone stale.
        // Probe this address via a unicast solicitation.
        //
        Dest = NCE->NeighborAddress;

        LLDest = NCE->LinkAddress;
    }

    //
    // If we were given a specific source address to use then do so (normal
    // case for our initial multicasted solicit), otherwise use the
    // outgoing interface's link-local address.
    // REVIEW - What if the link-local address is tentative/duplicate?
    //
    if (Source == NULL)
        Source = &IF->LinkLocalNTE->Address;

    //
    // Now that we've determined the source/destination/target addresses,
    // we can actually send the solicit.
    //
    NeighborSolicitSend0(IF, LLDest, Source, &Dest, &NCE->NeighborAddress);
}


//* DADSolicitSend - Sends a DAD Neighbor Solicit.
//
//  Like NeighborSolicitSend, but specialized for DAD.
//
void
DADSolicitSend(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    IPv6Addr Dest;
    void *LLDest;

    //
    // Multicast our solicitation to the solicited-node multicast
    // address corresponding to our own address.
    //
    CreateSolicitedNodeMulticastAddress(&NTE->Address, &Dest);

    //
    // Convert the IP-level multicast destination address
    // to a link-level multicast address.
    //
    LLDest = alloca(IF->LinkAddressLength);
    (*IF->ConvertMCastAddr)(IF->LinkContext, &Dest, LLDest);

    //
    // Now that we've created the destination addresses,
    // we can actually send the solicit.
    //
    NeighborSolicitSend0(IF, LLDest, &UnspecifiedAddr,
                         &Dest, &NTE->Address);
}


//* DADTimeout - handle a Duplicate Address Detection timeout.
//
//  Called from IPv6Timeout to handle DAD timeouts on an NTE.
//
void
DADTimeout(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    int SendDADSolicit = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (NTE->DADTimer != 0) {
        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--NTE->DADTimer == 0) {
            //
            // Timer went off.
            //

            ASSERT(NTE->DADState == DAD_STATE_TENTATIVE);

            if (NTE->DADCount == 0) {
                //
                // The address has passed Duplicate Address Detection.
                // Transition to a valid state.
                //
                if (NTE->ValidLifetime == 0)
                    NTE->DADState = DAD_STATE_DEPRECATED;
                else
                    NTE->DADState = DAD_STATE_PREFERRED;
            }
            else {
                //
                // Time to send another solicit.
                //
                NTE->DADCount--;
                NTE->DADCountLB++;
                NTE->DADTimer = IF->RetransTimer;
                SendDADSolicit = TRUE;
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendDADSolicit)
        DADSolicitSend(NTE);
}

//* CalcReachableTime
//
//  Calculate a pseudo-random ReachableTime from BaseReachableTime
//  (this prevents synchronization of Neighbor Unreachability Detection
//  messages from different hosts), and convert it to units of IPv6
//  timer ticks (cheaper to do once here than at every packet send).
//
uint                         // IPv6 timer ticks
CalcReachableTime(
    uint BaseReachableTime)  // Learned from Router Advertisements (in ms).
{
    uint Factor;
    uint ReachableTime;

    //
    // Calculate a uniformly-distributed random value between
    // MIN_RANDOM_FACTOR and MAX_RANDOM_FACTOR of the BaseReachableTime.
    // To keep the arithmetic integer, *_RANDOM_FACTOR (and thus the
    // 'Factor' variable) are defined as percentage values.
    //
    Factor = RandomNumber(MIN_RANDOM_FACTOR, MAX_RANDOM_FACTOR);

    //
    // Now that we have a random value picked out of our percentage spread,
    // take that percentage of the BaseReachableTime.
    //
    // BaseReachableTime has a maximum value of 3,600,000 milliseconds
    // (see RFC 1970, section 6.2.1), so Factor would have to exeed 1100 %
    // in order to overflow a 32-bit unsigned integer.
    //
    ReachableTime = (BaseReachableTime * Factor) / 100;

    //
    // Convert from milliseconds (which is what BaseReachableTime is in) to
    // IPv6 timer ticks (which is what we keep ReachableTime in).
    //
    return ConvertMillisToTicks(ReachableTime);
}

//* RedirectSend
//
//  Send a Redirect message to a neighbor,
//  telling it to use a better first-hop neighbor
//  in the future for the specified destination.
//
//  RecvNTEorIF (optionally) specifies a source address
//  to use in the Redirect message.
//
//  Packet (optionally) specifies data to include
//  in a Redirected Header option.
//  BUGBUG - Redirected Header option is never sent.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
RedirectSend(
    NeighborCacheEntry *NCE,               // Neighbor getting the Redirect.
    NeighborCacheEntry *TargetNCE,         // Better first-hop to use
    IPv6Addr *Destination,                 // for this Destination address.
    NetTableEntryOrInterface *NTEorIF,     // Source of the Redirect.
    PNDIS_PACKET FwdPacket,                // Packet triggering the redirect.
    uint FwdOffset,
    uint FwdPayloadLength)
{
    Interface *IF = NCE->IF;
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header *IP;
    ICMPv6Header *ICMP;
    void *TargetLinkAddress;
    KIRQL OldIrql;
    IPv6Addr *Source;

    ASSERT((IF == TargetNCE->IF) && (IF == NTEorIF->IF));

    //
    // Do we know the Target neighbor's link address?
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if (TargetNCE->NDState == ND_STATE_INCOMPLETE) {
        TargetLinkAddress = NULL;
    }
    else {
        TargetLinkAddress = alloca(IF->LinkAddressLength);
        RtlCopyMemory(TargetLinkAddress, TargetNCE->LinkAddress,
                      IF->LinkAddressLength);
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Calculate the Redirect's payload size.
    //
    PayloadLength = 40;
    if (TargetLinkAddress != NULL)
        PayloadLength += (IF->LinkAddressLength + 2 + 7) &~ 7;

    //
    // Allocate a packet for the Redirect.
    //
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Dest = NCE->NeighborAddress;

    //
    // If our caller specified a source address, use it.
    // Otherwise (common case) we use our link-local address.
    // REVIEW - What if the link-local address is tentative/duplicate?
    //
    Source = &(IsNTE(NTEorIF) ? CastToNTE(NTEorIF) : IF->LinkLocalNTE)
                        ->Address;
    IP->Source = *Source;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header *)(IP + 1);
    ICMP->Type = ICMPv6_REDIRECT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Must zero the reserved field.
    //
    *(ulong *)(ICMP + 1) = 0;

    //
    // Initialize the target and destination addresses.
    //
    ((IPv6Addr *)((ulong *)(ICMP + 1) + 1))[0] = TargetNCE->NeighborAddress;
    ((IPv6Addr *)((ulong *)(ICMP + 1) + 1))[1] = *Destination;

    if (TargetLinkAddress != NULL) {
        void *TargetOption;

        //
        // Include a Target Link-Layer Address option.
        //
        TargetOption = (void *)((IPv6Addr *)((ulong *)(ICMP + 1) + 1) + 2);
        ((uchar *)TargetOption)[0] = ND_OPTION_TARGET_LINK_LAYER_ADDRESS;
        ((uchar *)TargetOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, TargetOption, TargetLinkAddress);
    }

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    ICMP->Checksum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        &IP->Source, &IP->Dest,
        IP_PROTOCOL_ICMPv6);

    //
    // Send the Redirect packet.
    //
    IPv6Send0(Packet, Offset, NCE, Source);
}

//* RouterAdvertTimeout
//
//  Called periodically from IPv6Timeout to handle timeouts
//  for sending router advertisements for an interface.
//
//  Callable from DPC context, not from thread context.
//
void
RouterAdvertTimeout(Interface *IF, int Force)
{
    int SendRouterAdvert = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (IF->RATimer != 0) {
        ASSERT(IF->Flags & IF_FLAG_ADVERTISES);

        if (Force) {
            //
            // Enter "fast" mode if this is a forced RA.
            //
            IF->RACount = MAX_INITIAL_RTR_ADVERTISEMENTS;
            goto GenerateRA;
        }

        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--IF->RATimer == 0) {
          GenerateRA:
            //
            // Timer went off. Generate a router solicitation.
            //
            SendRouterAdvert = TRUE;

            //
            // Re-arm the timer.
            //
            IF->RATimer = RandomNumber(MinRtrAdvInterval, MaxRtrAdvInterval);

            //
            // If we are in "fast" mode, then ensure
            // that the next RA is sent quickly.
            //
            if (IF->RACount != 0) {
                IF->RACount--;
                if (IF->RATimer > MAX_INITIAL_RTR_ADVERT_INTERVAL)
                    IF->RATimer = MAX_INITIAL_RTR_ADVERT_INTERVAL;
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendRouterAdvert)
        RouterAdvertSend(IF);
}
