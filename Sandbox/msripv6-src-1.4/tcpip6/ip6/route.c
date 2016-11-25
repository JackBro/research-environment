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
// Routing routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "icmp.h"
#include "neighbor.h"
#include "alloca.h"

//
// Forward declarations of internal functions.
//

extern void
DestroyBCE(BindingCacheEntry **PrevBCE, BindingCacheEntry *BCE);

//
// The Route Cache contains RCEs. RCEs with reference count of one
// can still be cached, but they may also be reclaimed.
// (The lone reference is from the cache itself.)
//
// An RCE records a route from an NTE (think source address)
// to a destination address. It contains a pointer to an NCE
// (next-hop neighbor) that should be used to reach the destination.
// It can also record path MTU.
//
// The current implementation is a simple circular linked-list of RCEs.
//
KSPIN_LOCK RouteCacheLock;
struct RouteCache RouteCache;
struct RouteTable RouteTable;
ulong RouteCacheValidationCounter;
BindingCacheEntry *BindingCache = NULL;
SitePrefixEntry *SitePrefixTable = NULL;

int ForceRouterAdvertisements = FALSE;

//* RemoveRTE
//
//  Remove the RTE from the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
RemoveRTE(RouteTableEntry **PrevRTE, RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    ASSERT(*PrevRTE == RTE);
    *PrevRTE = RTE->Next;
    if (RouteTable.Last == &RTE->Next)
        RouteTable.Last = PrevRTE;
}

//* InsertRTEAtFront
//
//  Insert the RTE at the front of the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRTEAtFront(RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    RTE->Next = RouteTable.First;
    RouteTable.First = RTE;
    if (RouteTable.Last == &RouteTable.First)
        RouteTable.Last = &RTE->Next;
}

//* InsertRTEAtBack
//
//  Insert the RTE at the back of the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRTEAtBack(RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    RTE->Next = NULL;
    *RouteTable.Last = RTE;
    RouteTable.Last = &RTE->Next;
    if (RouteTable.First == NULL)
        RouteTable.First = RTE;
}

//* InsertRCE
//
//  Insert the RCE in the route cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *AfterRCE = SentinelRCE;

    RCE->Prev = AfterRCE;
    (RCE->Next = AfterRCE->Next)->Prev = RCE;
    AfterRCE->Next = RCE;
    RouteCache.Count++;
}

//* RemoveRCE
//
//  Remove the RCE from the route cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
RemoveRCE(RouteCacheEntry *RCE)
{
    RCE->Prev->Next = RCE->Next;
    RCE->Next->Prev = RCE->Prev;
    RouteCache.Count--;
}

//* ValidateRCE
//
//  Check that an RCE is still valid, and if not, releases
//  the RCE and returns a reference for a new RCE.
//  In any case, returns a pointer to an RCE.
//
RouteCacheEntry *
ValidateRCE(RouteCacheEntry *RCE)
{
    if (RCE->Valid != RouteCacheValidationCounter) {
        RouteCacheEntry *NewRCE;
        uint ScopeId;
        IP_STATUS Status;

        //
        // Get a new RCE to replace the current RCE.
        // REVIEW: If this fails, then continue to use the current RCE.
        // This way our callers don't have to check for errors.
        //
        ScopeId = DetermineScopeId(&RCE->Destination, RCE->NTE->IF);
        Status = RouteToDestination(&RCE->Destination, ScopeId,
                                    CastFromNTE(RCE->NTE), RTD_FLAG_NORMAL,
                                    &NewRCE);
        if (Status == IP_SUCCESS) {
            ReleaseRCE(RCE);
            RCE = NewRCE;
        } else {
            KdPrint(("ValidateRCE(%x): RouteToDestination failed: %x\n",
                     RCE, Status));
        }
    }

    return RCE;
}


//* CreateOrReuseRoute
//
//  Creates a new RCE. Attempts to reuse an existing RCE.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
//  Returns NULL if a new RCE can not be allocated.
//  The RefCnt field in the returned RCE is initialized to one.
//
//  REVIEW: Immediately reuses any RCE with one reference.
//  This will aid debugging. But it would be better to cache
//  unused RCEs for some time.
//
RouteCacheEntry *
CreateOrReuseRoute(void)
{
    RouteCacheEntry *RCE;

    if (RouteCache.Count >= RouteCache.Limit) {
        //
        // First search backwards for an unused RCE.
        //
        for (RCE = RouteCache.Last; RCE != SentinelRCE; RCE = RCE->Prev) {

            if (RCE->RefCnt == 1) {
                //
                // We can reuse this RCE.
                //
                RemoveRCE(RCE);
                ReleaseNCE(RCE->NCE);
                ReleaseNTE(RCE->NTE);
                if (RCE->CareOfRCE) {
                    ReleaseRCE(RCE->CareOfRCE);
                    RCE->CareOfRCE = NULL;
                }
                return RCE;
            }
        }
    }

    //
    // Create a new RCE.
    //
    RCE = ExAllocatePool(NonPagedPool, sizeof *RCE);
    if (RCE == NULL)
        return NULL;

    RCE->RefCnt = 1;
    return RCE;
}


//* RouteCacheCheck
//
//  Check the route cache's consistency. Ensure that
//  a) There is only one RCE for an interface/destination pair, and
//  b) There is at most one valid unconstrained RCE for the destination.
//
//  Called with the route table locked.
//
#if DBG
void
RouteCacheCheck(RouteCacheEntry *CheckRCE, ulong CurrentValidationCounter)
{
    IPv6Addr *Dest = &CheckRCE->Destination;
    Interface *IF = CheckRCE->NTE->IF;
    uint ScopeId = DetermineScopeId(Dest, IF);
    uint NumTotal = 0;
    uint NumUnconstrained = 0;
    RouteCacheEntry *RCE;

    //
    // Scan the route cache looking for problems.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {
        NumTotal++;

        if (IP6_ADDR_EQUAL(&RCE->Destination, Dest)) {
            if (RCE->NTE->IF == IF) {
                //
                // There should only be one RCE in the cache
                // for an interface/destination pair.
                // (There may be other, invalid RCEs not in the cache.)
                //
                ASSERT(RCE == CheckRCE);
            }

            if (!(RCE->Flags & RCE_FLAG_CONSTRAINED) &&
                (RCE->Valid == CurrentValidationCounter) &&
                (DetermineScopeId(&RCE->Destination, RCE->NTE->IF) == ScopeId))
                NumUnconstrained++;
        }
    }

    //
    // There should be at most one valid unconstrained RCE
    // for this scope-id/destination.
    //
    ASSERT(NumUnconstrained <= 1);

    //
    // The total should be correct.
    //
    ASSERT(NumTotal == RouteCache.Count);
}
#else // DBG
__inline void
RouteCacheCheck(RouteCacheEntry *CheckRCE, ulong CurrentValidationCounter)
{
}
#endif // DBG


//* RouteToDestination - Find a route to a particular destination.
//
//  Finds an existing, or creates a new, route cache entry for
//  a particular destination.  Note the destination address may
//  only be valid in a particular scope.
//
//  The optional NTEorIF argument specifies the interface
//  and/or the source address that should be used to reach the destination.
//
//  Return codes:
//      IP_NO_RESOURCES         Couldn't allocate memory.
//      IP_PARAMETER_PROBLEM    Illegal Dest/ScopeId
//      IP_BAD_ROUTE            Bad NTEorIF for this destination.
//      IP_DEST_NO_ROUTE        No way to reach the destination.
//
//  IP_DEST_NO_ROUTE can only be returned if NTEorIF is NULL.
//
IP_STATUS  // Returns: whether call was sucessful and/or why not.
RouteToDestination(
    IPv6Addr *Dest,                     // Destination address to route to.
    uint ScopeId,                       // Scope id for Dest (0 if non-scoped).
    NetTableEntryOrInterface *NTEorIF,  // IF to send from (may be NULL).
    uint Flags,                         // Control optional behaviors.
    RouteCacheEntry **ReturnRCE)        // Returns pointer to cached route.
{
    ulong CurrentValidationCounter;
    RouteCacheEntry *SaveRCE = NULL;
    RouteCacheEntry *RCE;
    Interface *IF, *TmpIF;
    NeighborCacheEntry *NCE;
    NetTableEntry *NTE;
    ushort Constrained;
    KIRQL OldIrql, OldIrql2;
    IP_STATUS ReturnValue;
    int Scope;

    //
    // Pre-calculate Scope for scoped addresses (saves time in the loop).
    // Note that callers can supply ScopeId == 0 for a scoped address.
    //
    if (IsLinkLocal(Dest))
        Scope = ADE_LINK_LOCAL;
    else if (IsSiteLocal(Dest))
        Scope = ADE_SITE_LOCAL;
    else {
        if (ScopeId != 0)
            return IP_PARAMETER_PROBLEM;
        Scope = ADE_GLOBAL;
    }

    if (NTEorIF != NULL) {
        //
        // Our caller is constraining the originating interface.
        //
        IF = NTEorIF->IF;

        //
        // First, check this against ScopeId.
        //
        switch (Scope) {
        case ADE_LINK_LOCAL:
            if (ScopeId == 0)
                ScopeId = IF->Index;
            else if (ScopeId != IF->Index)
                return IP_BAD_ROUTE;
            break;

        case ADE_SITE_LOCAL:
            if (ScopeId == 0)
                ScopeId = IF->Site;
            else if (ScopeId != IF->Site)
                return IP_BAD_ROUTE;
            break;

        case ADE_GLOBAL:
            break;

        default:
            ASSERT(! "bad Scope");
            break;
        }

        //
        // Depending on Flags and whether this is forwarding interface,
        // we may ignore this specification and look at all interfaces.
        // Logically, the packet is originated by the specified interface
        // but then internally forwarded to the outgoing interface.
        // (Although we will not decrement the Hop Count.)
        // As when forwarding, we check after finding the best route
        // if the route will cause the packet to leave
        // the scope of the source address.
        //
        // It is critical that the route cache lookup and FindNextHop
        // computation use only IF and not NTEorIF. This is necessary
        // for the maintenance of the cache invariants. Once we have
        // an RCE (or an error), then we can check against NTEorIF.
        //
        switch (Flags) {
        case RTD_FLAG_NORMAL:
            if (IF->Flags & IF_FLAG_FORWARDS)
                IF = NULL;
            break;

        case RTD_FLAG_STRICT:
            break;

        default:
            ASSERT(! "bad RouteToDestination Flags");
            break;
        }
    }
    else {
        //
        // Our caller is not constraining the originating interface.
        //
        IF = NULL;
    }

    //
    // For consistency, snapshot RouteCacheValidationCounter.
    //
    CurrentValidationCounter = RouteCacheValidationCounter;

    //
    // We'll branch back to the Retry label when NTEorIF specifies
    // a forwarding interface and so we set IF to null, performing
    // an unconstrained route lookup, but then either there is no route
    // or the unconstrained route would carry the packet outside
    // the scope of the source address. So we retry a constrained lookup.
    //
    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
Retry:

    //
    // Check for an existing route cache entry.
    //
    // We want to avoid calling FindNextHop/FindRoute more than once.
    // Partly because to avoid the cost, and partly to avoid performing
    // FindRoute's round-robining more than once. For example suppose
    // there are two possible routers for a destination and both are
    // currently unreachable. We want to round-robin between the two routers.
    // If we call FindRoute twice in the course of validating & regenerating
    // an RCE for this destination, we will round-robin back to where we
    // started and always use the same router.
    //
    // There are two main cases.
    //
    // If IF is not NULL, then there is at most one matching RCE
    // in the cache. If this RCE does not validate, then we can use
    // the results of FindNextHop/FindBestSourceAddress when creating
    // the new RCE.
    //
    // If IF is NULL, then there may be more than one matching RCE.
    // We can only reuse the results of the validating FindNextHop/
    // FindBestSourceAddress iff FindRoute returned Constrained == 0.
    //

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {

        //
        // We want a route to the requested destination, obviously.
        //
        if (!IP6_ADDR_EQUAL(Dest, &RCE->Destination))
            continue;

        //
        // For scoped addresses, we must check that the destination
        // matches in scope as well as actual address value.
        //
        if (ScopeId != 0) {
            switch (Scope) {
            case ADE_LINK_LOCAL:
                //
                // ScopeId is an interface index.
                //
                if (ScopeId != RCE->NTE->IF->Index)
                    continue;
                break;
            case ADE_SITE_LOCAL:
                //
                // ScopeId is a site identifier.
                //
                if (ScopeId != RCE->NTE->IF->Site)
                    continue;
                break;
            default:
                ASSERT(! "bad Scope");
                break;
            }
        }

        //
        // Check for caller-imposed interface constraints.
        //
        if (IF == NULL) {
            //
            // We're not constrained to a particular interface, so
            // there may be multiple routes to this destination in
            // the cache to choose from.  Don't pick a constrained RCE.
            //
            if (RCE->Flags & RCE_FLAG_CONSTRAINED) {
                //
                // If this RCE is invalid, then RCE_FLAG_CONSTRAINED
                // might be stale information. We do not want to pass
                // by this RCE and then later create another RCE
                // for the same interface/destination pair.
                //
                if (RCE->Valid != CurrentValidationCounter)
                    goto AttemptValidation;
                continue;
            }
        } else {
            //
            // We're constrained to a particular interface.
            // If this route uses a different one, keep looking.
            //
            if (IF != RCE->NTE->IF)
                continue;
        }

        //
        // At this point, we have a RCE that matches our criteria.
        // As long as the RCE is still valid, we're done.
        //
        if (RCE->Valid == CurrentValidationCounter) {
            IF = RCE->NTE->IF;
            goto ReturnRCE;
        }

    AttemptValidation:

        //
        // Something has changed in the routing state since the last
        // time this RCE was validated.  Attempt to revalidate it.
        // We calculate a new NTE and NCE for this destination,
        // restricting ourselves to sending from the same interface.
        //
        ReturnValue = FindNextHop(RCE->NTE->IF, Dest, ScopeId,
                                  &TmpIF, &NCE, &Constrained);
        if (ReturnValue != IP_SUCCESS)
            goto InvalidRCE; // BUGBUG - what here?

        ASSERT(TmpIF == RCE->NTE->IF);
        ASSERT((IF == NULL) || (IF == TmpIF));

        NTE = FindBestSourceAddress(TmpIF, Dest);
        if (NTE == NULL) {
            ReleaseNCE(NCE);
            goto InvalidRCE; // BUGBUG - what here?
        }

        //
        // If our new calculations yield the same NTE and NCE that
        // are present in the existing RCE, than we can just validate it.
        // REVIEW: If this is a Redirect RCE, should we check the NCE?
        //
        if ((RCE->NTE == NTE) &&
            (RCE->NCE == NCE) &&
            ((RCE->Flags & RCE_FLAG_CONSTRAINED) == Constrained)) {
            KdPrint(("RouteToDestination - validating RCE %x\n", RCE));

            RCE->Valid = CurrentValidationCounter;
            ReleaseNCE(NCE);
            ReleaseNTE(NTE);

            //
            // We need to check again that the RCE meets the IF criteria.
            // We may have checked the RCE validity because the RCE
            // appeared to be constrained and we need an unconstrained RCE.
            // So in that case, if the RCE validated we can't actually use it.
            //
            if ((IF == NULL) && (RCE->Flags & RCE_FLAG_CONSTRAINED))
                continue;

            IF = TmpIF;
            goto ReturnRCE;
        }

        //
        // We can't just validate the existing RCE, we need to update
        // it.  If the RCE has exactly one reference, we could update it
        // in place (this wouldn't work if it has more than one reference
        // since there is no way to signal the RCE's other users that the
        // NCE and/or NTE it caches has changed).  But this wouldn't help
        // the case where we are called from ValidateRCE.  And it would
        // require some care as to which information in the RCE is still
        // valid.  So we ignore this optimization opportunity and will
        // create a new RCE instead.
        //
        // However, we can take advantage of another optimization.  As
        // long as we're still limiting our interface choice to the one
        // that is present in the existing (invalid) RCE, and there isn't
        // a better route available, then we can use the NCE and NTE we
        // got from FindNextHop and FindBestSourceAddress above to create
        // our new RCE since we aren't going to find a better one.
        //
        if ((IF != NULL) || (Constrained == 0)) {
            //
            // Since some of the state information in the existing RCE
            // is still valid, we hang onto it so we can use it later
            // when creating the new RCE. We assume ownership of the
            // cache's reference for the invalid RCE.
            //
            KdPrint(("RouteToDestination - saving RCE %x\n", RCE));
            RemoveRCE(RCE);
            SaveRCE = RCE;
            IF = TmpIF;
            goto HaveNCEandNTE;
        }

        ReleaseNTE(NTE);
        ReleaseNCE(NCE);

      InvalidRCE:
        //
        // Not valid, we keep looking for a valid matching RCE.
        //
        KdPrint(("RouteToDestination - invalid RCE %x\n", RCE));
    }


    //
    // No existing RCE found. Before creating a new RCE,
    // we determine a next-hop neighbor (NCE) and
    // a best source address (NTE) for this destination.
    // The order is important: we want to avoid churning
    // the cache via CreateOrReuseRoute if we will just
    // get an error anyway.
    // This prevents a denial-of-service attack.
    //

    ReturnValue = FindNextHop(IF, Dest, ScopeId,
                              &IF, &NCE, &Constrained);
    if (ReturnValue != IP_SUCCESS) {
        if (ReturnValue == IP_DEST_NO_ROUTE) {
            ASSERT(IF == NULL);
            if (NTEorIF != NULL) {
                //
                // This can only happen when NTEorIF specifies
                // a forwarding interface. There was no route
                // for the destination. So retry, preserving
                // the interface constraint. This will allow
                // the destination to be considered on-link.
                // But we must check for an existing RCE.
                //
                IF = NTEorIF->IF;
                goto Retry;
            }
        }
        goto ReturnError;
    }

    //
    // Find the best source address for this destination.
    // (The NTE from our caller might not be the best.)
    // By restricting ourselves to the interface returned
    // by FindNextHop above, we know we haven't left our
    // particular scope.
    //
    NTE = FindBestSourceAddress(IF, Dest);
    if (NTE == NULL) {
        //
        // We have no valid source address to use!
        //
        ReturnValue = IP_BAD_ROUTE;
        ReleaseNCE(NCE);
        goto ReturnError;
    }

  HaveNCEandNTE:

    //
    // Get a new route cache entry.
    // Because SaveRCE was just removed from the cache,
    // CreateOrReuseRoute will not find it.
    //
    RCE = CreateOrReuseRoute();
    if (RCE == NULL) {
        ReturnValue = IP_NO_RESOURCES;
        ReleaseNTE(NTE);
        ReleaseNCE(NCE);
        goto ReturnError;
    }

    //
    // FindOrCreateNeighbor/FindRoute (called from FindNextHop) gave
    // us a reference for the NCE.  We donate that reference to the RCE.
    // Similarly, FindBestSourceAddress gave us a reference
    // for the NTE and we donate the reference to the RCE.
    //
    RCE->NCE = NCE;
    RCE->NTE = NTE;
    RCE->PathMTU = IF->LinkMTU;
    RCE->PMTULastSet = 0;  // PMTU timer not running.
    RCE->Destination = *Dest;
    RCE->Type = RCE_TYPE_COMPUTED;
    RCE->Flags = Constrained;
    RCE->LastError = 0;
    RCE->CareOfRCE = FindBindingCacheRCE(Dest);
    RCE->Valid = CurrentValidationCounter;

    //
    // Copy state from a previous RCE for this destination,
    // if we have it and the state is relevant.
    //
    if (SaveRCE != NULL) {
        ASSERT(SaveRCE->NTE->IF == RCE->NTE->IF);

        //
        // PathMTU is relevant if the next-hop neighbor is unchanged.
        //
        if (RCE->NCE == SaveRCE->NCE) {
            RCE->PathMTU = SaveRCE->PathMTU;
            RCE->PMTULastSet = SaveRCE->PMTULastSet;
            RCE->Flags |= (SaveRCE->Flags & RCE_FLAG_FRAGMENT_HEADER_NEEDED);
        }

        //
        // ICMP rate-limiting information is always relevant.
        //
        RCE->LastError = SaveRCE->LastError;
    }

    //
    // Add the new route cache entry to the cache.
    //
    goto InsertAndReturnRCE;

  ReturnRCE:
    //
    // If the RCE is not at the front of the cache, move it there.
    //
    if (RCE->Prev != SentinelRCE) {
        RemoveRCE(RCE);
      InsertAndReturnRCE:
        InsertRCE(RCE);
    }

    ASSERT(IF == RCE->NTE->IF);

    //
    // Check route cache consistency.
    //
    RouteCacheCheck(RCE, CurrentValidationCounter);

    //
    // But does this route carry the packet outside
    // the scope of the specified source address?
    // This can only happen if the specified source address
    // was on a forwarding interface.
    //
    if ((NTEorIF != NULL) && (NTEorIF->IF != IF) && IsNTE(NTEorIF)) {
        NetTableEntry *TmpNTE = CastToNTE(NTEorIF);

        if ((IsLinkLocal(&TmpNTE->Address) &&
             (TmpNTE->IF->Index != IF->Index)) ||
            (IsSiteLocal(&TmpNTE->Address) &&
             (TmpNTE->IF->Site != IF->Site))) {
            //
            // The route would internally forward the packet
            // outside the scope of the specified source address.
            // Instead of doing this, redo the route lookup
            // constraining to the specified source interface.
            //
            IF = NTEorIF->IF;
            goto Retry;
        }
    }

    AddRefRCE(RCE);
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    *ReturnRCE = RCE;
    ReturnValue = IP_SUCCESS;
    goto Return;

  ReturnError:
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

  Return:
    if (SaveRCE != NULL)
        ReleaseRCE(SaveRCE);
    return ReturnValue;
}


//* FlushRouteCache
//
//  Flushes entries from the route cache.
//  The Interface or the Address can be left unspecified.
//  in which case all relevant entries are flushed.
//
//  Note that even if an RCE has references,
//  we can still remove it from the route cache.
//  It will continue to exist until its ref count falls to zero,
//  but subsequent calls to RouteToDestination will not find it.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
FlushRouteCache(Interface *IF, IPv6Addr *Addr)
{
    RouteCacheEntry *Delete = NULL;
    RouteCacheEntry *RCE, *NextRCE;
    KIRQL OldIrql;

    //
    // REVIEW: If both IF and Addr are specified,
    // we can bail out of the loop early.
    //

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = NextRCE) {
        NextRCE = RCE->Next;

        if (((IF == NULL) ||
             (IF == RCE->NTE->IF)) &&
            ((Addr == NULL) ||
             IP6_ADDR_EQUAL(Addr, &RCE->Destination))) {
            //
            // We can remove this RCE from the cache.
            //
            RemoveRCE(RCE);

            //
            // Put it on our delete list.
            //
            RCE->Next = Delete;
            Delete = RCE;
        }
    }
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    //
    // Release the RCE references that were held by the route cache.
    //
    while (Delete != NULL) {
        RCE = Delete;
        Delete = RCE->Next;

        //
        // Prevent use of this RCE by anyone who has it cached.
        //
        InvalidateRCE(RCE);
        ReleaseRCE(RCE);
    }
}

//* FindNextHop
//
//  Calculate the next hop to use for the destination.
//
//  A non-null IF specifies the interface that will actually
//  get used to send the packet. Otherwise the outgoing interface
//  is not constrained.
//
//  Note that FindNextHop only returns IP_DEST_NO_ROUTE
//  when IF == NULL. RouteToDestination relies on this.
//
//  Called with the route table locked.
//
IP_STATUS
FindNextHop(
    Interface *IF,                      // Outgoing IF (may be NULL).
    IPv6Addr *Dest,                     // Destination address to route to.
    uint ScopeId,                       // Scope id for Dest (0 if non-scoped).
    Interface **ReturnIF,               // IF we picked to send from.
    NeighborCacheEntry **ReturnNCE,     // NCE for next hop.
    ushort *ReturnConstrained)
{
    NeighborCacheEntry *NCE;
    IP_STATUS Status;

    //
    // An unspecified destination is never legal here.
    //
    if (IsUnspecified(Dest)) {
        KdPrint(("FindNextHop - inappropriate dest?\n"));
        return IP_PARAMETER_PROBLEM;
    }

    //
    // We have a hardcoded case here for the loopback address
    // because it's slightly simpler than having an RTE for it.
    // It's the easiest way to prevent strange behavior,
    // like sending Neighbor Solicits for the loopback address
    // out an ethernet interface. (Sending to the loopback address
    // but specifying a source address on an ethernet interface.)
    //
    if (IsLoopback(Dest) || IsNodeLocalMulticast(Dest)) {
        if ((IF != NULL) && (IF != &LoopInterface)) {
            KdPrint(("FindNextHop: bad loopback source?\n"));
            return IP_BAD_ROUTE;
        }

        IF = &LoopInterface;
        NCE = LoopNCE;
        AddRefNCE(NCE);
        *ReturnConstrained = 0;
        goto ReturnSuccess;
    }

    //
    // FindRoute treats link-local and multicast destinations
    // somewhat specially. It also checks for on-link prefixes.
    //
    Status = FindRoute(IF, Dest, ScopeId, &IF, &NCE, ReturnConstrained);
    if (Status == IP_SUCCESS)
        goto ReturnSuccess;

    //
    // We have no usable default router, or at least not one
    // that is compatible with the specified interface.
    // The Neighbor Discovery draft (section 5.2) says to
    // assume the destination is on-link when the default
    // router list is empty. But this only works
    // if we have an interface specified.
    //
    if (IF == NULL)
        return IP_DEST_NO_ROUTE;

    //
    // Check that this is not a pseudo-interface.
    // For example, if we receive an Echo Request
    // with v4-compatible destination and link-local source,
    // then this would happen.
    //
    if (!(IF->Flags & IF_FLAG_DISCOVERS)) {
        KdPrint(("FindNextHop: dest on-link pseudo-interface?\n"));
        return IP_BAD_ROUTE;
    }

    //
    // Assume that the destination address is on-link.
    // Search the interface's neighbor cache for the destination.
    //
    NCE = FindOrCreateNeighbor(IF, Dest);
    if (NCE == NULL)
        return IP_NO_RESOURCES;

    *ReturnConstrained = RCE_FLAG_CONSTRAINED;

  ReturnSuccess:
    *ReturnIF = IF;
    *ReturnNCE = NCE;
    return IP_SUCCESS;
}


//* FindRoute
//
//  Given a destination address, checks the list of routes
//  using the longest-matching-prefix algorithm
//  to decide if we have a route to this address.
//  If so, returns the neighbor through which we should route.
//
//  If the optional IF is supplied, then this constrains the lookup
//  to only use routes via the specified outgoing interface.
//
//  The ReturnConstrained parameter returns an indication of whether the
//  IF parameter constrained the returned NCE. That is, if IF is NULL
//  then Constrained is always returned as zero. If IF is non-NULL and
//  the same NCE is returned as would have been returned if IF were
//  NULL, then Constrained is returned as zero. Otherwise Constrained
//  is returned as RCE_FLAG_CONSTRAINED.
//
//  NOTE: We can return RCE_FLAG_CONSTRAINED for link-local destinations
//  when the ScopeId is zero (this is what makes round-robin work in
//  this case), but RCEs for link-local destinations are never constrained.
//
//  The RouteCacheLock must already be held.
//
//  NOTE: Any code path that changes any state used by FindRoute
//  must use InvalidateRouteCache.
//
IP_STATUS
FindRoute(
    Interface *IF,
    IPv6Addr *Dest,
    uint ScopeId,
    Interface **ReturnIF,
    NeighborCacheEntry **ReturnNCE,
    ushort *ReturnConstrained)
{
    RouteTableEntry *RTE, **PrevRTE;
    NeighborCacheEntry *NCE;
    uint MinPrefixLength;
    int Reachable;
    int Scope;

    //
    // These variables track the best route that we can actually return.
    // So either IF == NULL or IF == RTE->IF.
    //
    RouteTableEntry *BestRTE;
    NeighborCacheEntry *BestNCE = NULL; // Holds a reference.
    uint BestPrefixLength;      // Initialized when BestNCE is non-NULL.
    uint BestPref;              // Initialized when BestNCE is non-NULL.
    int BestReachable;          // Initialized when BestNCE is non-NULL.

    //
    // These variables track the best unconstrained route.
    // They are only used if IF != NULL.
    //
    NeighborCacheEntry *BallNCE = NULL; // Does NOT hold a reference.
    uint BallPrefixLength;      // Initialized when BallNCE is non-NULL.
    uint BallPref;              // Initialized when BallNCE is non-NULL.
    int BallReachable;          // Initialized when BallNCE is non-NULL.

    //
    // We enforce a minimum prefix length for "on-link" addresses.
    // Routes for those address types must be specifically for them,
    // so that we don't send those packets to a default router.
    //
    if (IsMulticast(Dest)) {
        MinPrefixLength = 8;
        ASSERT(ScopeId == 0); // So we don't need to initialize Scope.
    } else if (IsLinkLocal(Dest)) {
        MinPrefixLength = 10;
        Scope = ADE_LINK_LOCAL;
    } else if (IsSiteLocal(Dest)) {
        MinPrefixLength = 0;
        Scope = ADE_SITE_LOCAL;
    } else {
        MinPrefixLength = 0;
        ASSERT(ScopeId == 0); // So we don't need to initialize Scope.
    }

    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        //
        // Does this route's prefix match that of our destination address?
        // And is this route's interface in our specified scope (if any)?
        //
        if (HasPrefix(Dest, &RTE->Prefix, RTE->PrefixLength) &&
            (RTE->PrefixLength >= MinPrefixLength) &&
            ((ScopeId == 0) || 
             ((Scope == ADE_LINK_LOCAL) && (ScopeId == RTE->IF->Index)) ||
             ((Scope == ADE_SITE_LOCAL) && (ScopeId == RTE->IF->Site)))) {

            //
            // We have a match against a potential route.
            // Get a pointer to the next hop.
            //
            if (IsOnLinkRTE(RTE)) {
                //
                // Note that in some situations we will create an NCE
                // that we will end up not using. That's OK.
                //
                if (RTE->IF == TunnelGetPseudoIF()) {
                    ASSERT((RTE->PrefixLength <= 96) &&
                           (RTE->PrefixLength % 8 == 0));
                    NCE = CreatePermanentNeighbor(RTE->IF, Dest,
                                        &Dest->u.Byte[RTE->PrefixLength / 8]);
                } else {
                    ASSERT(RTE->IF->Flags & IF_FLAG_DISCOVERS);
                    NCE = FindOrCreateNeighbor(RTE->IF, Dest);
                }
                if (NCE == NULL) {
                    //
                    // Couldn't create a new neighbor.
                    // Just bail out now.
                    //
                    break;
                }
            } else {
                NCE = RTE->NCE;
                AddRefNCE(NCE);
            }

            //
            // Note that reachability state transitions
            // must invalidate the route cache.
            // A negative return value indicates
            // that the neighbor was just found to be unreachable
            // so we should round-robin.
            //
            Reachable = GetReachability(NCE);
            if (Reachable < 0) {
                //
                // Time for round-robin. Move this route to the end
                // and continue. The next time we get to this route,
                // GetReachability will return a non-negative value.
                //
                // Because round-robin perturbs route table state,
                // it "should" invalidate the route cache. However,
                // this isn't necessary. The route cache is invalidated
                // when NCE->DoRoundRobin is set to TRUE, and the
                // round-robin is actually performed by FindRoute before
                // returning any result that could depend on this
                // route's position in the route table.
                //
                ReleaseNCE(NCE);
                RemoveRTE(PrevRTE, RTE);
                InsertRTEAtBack(RTE);
                continue;
            }

            //
            // Track the best route that we can actually return,
            // either because we don't have an interface constraint
            // or because the route uses the specified interface.
            //
            if ((IF == NULL) || (IF == RTE->IF)) {
                if (BestNCE == NULL) {
                    //
                    // This is the first suitable next hop,
                    // so remember it.
                    //
                  RememberBest:
                    AddRefNCE(NCE);
                    BestNCE = NCE;
                    BestPrefixLength = RTE->PrefixLength;
                    BestReachable = Reachable;
                    BestPref = RTE->Preference;
                    BestRTE = RTE;
                }
                else if ((RTE->PrefixLength > BestPrefixLength) ||
                         ((RTE->PrefixLength == BestPrefixLength) &&
                          ((Reachable > BestReachable) ||
                           ((Reachable == BestReachable) &&
                            (RTE->Preference < BestPref))))) {
                    //
                    // This next hop looks better.
                    //
                    ReleaseNCE(BestNCE);
                    goto RememberBest;
                }
            }

            //
            // If there is a constraining interface, we also track
            // the best overall next hop. Note that we do NOT
            // hold a reference for BallNCE. All we need to know
            // eventually is whether BallNCE == BestNCE and
            // we hold a ref for BestNCE, which is good enough.
            //
            if (IF != NULL) {
                if (BallNCE == NULL) {
                    //
                    // This is the first suitable next hop,
                    // so remember it.
                    //
                  RememberBall:
                    BallNCE = NCE;
                    BallPrefixLength = RTE->PrefixLength;
                    BallReachable = Reachable;
                    BallPref = RTE->Preference;
                }
                else if ((RTE->PrefixLength > BallPrefixLength) ||
                         ((RTE->PrefixLength == BallPrefixLength) &&
                          ((Reachable > BallReachable) ||
                           ((Reachable == BallReachable) &&
                            (RTE->Preference < BallPref))))) {
                    //
                    // This next hop looks better.
                    //
                    goto RememberBall;
                }
            }

            ReleaseNCE(NCE);
        }

        //
        // Move on to the next route.
        //
        PrevRTE = &RTE->Next;
    }
    ASSERT(PrevRTE == RouteTable.Last);

    if (BestNCE != NULL) {
        *ReturnIF = BestRTE->IF;
        *ReturnNCE = BestNCE;

        if ((IF == NULL) || (BestNCE == BallNCE))
            *ReturnConstrained = 0;
        else
            *ReturnConstrained = RCE_FLAG_CONSTRAINED;

        return IP_SUCCESS;
    } else {
        //
        // Didn't find a suitable next hop.
        //
        return IP_DEST_NO_ROUTE;
    }
}


//* ReleaseRCE
//
//  Releases a reference to an RCE.
//  Sometimes called with the route cache lock held.
//
void
ReleaseRCE(RouteCacheEntry *RCE)
{
    if (InterlockedDecrement(&RCE->RefCnt) == 0) {
        //
        // This RCE should be deallocated.
        // It has already been removed from the cache.
        //
        ReleaseNTE(RCE->NTE);
        ReleaseNCE(RCE->NCE);
        if (RCE->CareOfRCE)
            ReleaseRCE(RCE->CareOfRCE);
        KdPrint(("Freeing RCE: %x\n",RCE));
        ExFreePool(RCE);
    }
}


//* FindNetworkWithAddress - Locate NTE with corresponding address and scope.
//
//  Convert a source address to an NTE by scanning the list of NTEs,
//  looking for an NTE with this address.  If the address is scope
//  specific, the ScopeId provided should identify the scope.
//
//  Returns NULL if no matching NTE is found.
//  If found, returns a reference for the NTE.
//
NetTableEntry *
FindNetworkWithAddress(IPv6Addr *Source, uint ScopeId)
{
    NetTableEntry *NTE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&NetTableListLock, &OldIrql);

    //
    // Loop through all the NTEs on the NetTableList.
    //
    for (NTE = NetTableList; ; NTE = NTE->NextOnNTL) {
        if (NTE == NULL)
            goto Return;

        if (IP6_ADDR_EQUAL(&NTE->Address, Source)) {
            //
            // Address matches, now check for matching scope id
            // if it's a scoped address.
            //
            switch (NTE->Scope) {
            case ADE_LINK_LOCAL:
                //
                // For link-local addresses, the scope identifier
                // is just the interface identifier.
                //
                if (ScopeId != NTE->IF->Index)
                    continue;
                break;

            case ADE_SITE_LOCAL:
                //
                // For site-local addresses, the scope identifier
                // is the site identifier.
                //
                if (ScopeId != NTE->IF->Site)
                    continue;
                break;

            default:
                //
                // The scope identifier is meaningless in these cases.
                // We insist that it is zero to get a match.
                //
                if (ScopeId != 0)
                    continue;
                break;
            }

            //
            // Check that this NTE is valid.
            // (For example, an NTE still doing DAD is invalid.)
            //
            if (!IsValidNTE(NTE)) {
                KdPrint(("FindNetworkWithAddress: invalid NTE\n"));
                NTE = NULL;
                goto Return;
            }

            break;
        }
    }

    AddRefNTE(NTE);
  Return:
    KeReleaseSpinLock(&NetTableListLock, OldIrql);
    return NTE;
}


//* InvalidateRouter
//
//  Called when we know a neighbor is no longer a router.
//
//  Callable from a thread or DPC context.
//  Called with no locks held.
//
void
InvalidateRouter(NeighborCacheEntry *NCE)
{
    RouteTableEntry *RTE, **PrevRTE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    KdPrint(("Invalidating routes with NCE=%x\n", NCE));

    PrevRTE = &RouteTable.First;
    while((RTE = *PrevRTE) != NULL) {

        if (RTE->NCE == NCE) {
            //
            // Remove the RTE from the list.
            //
            KdPrint(("Removed route RTE %x from route table\n", RTE));
            RemoveRTE(PrevRTE, RTE);

            //
            // Release the RTE.
            //
            ReleaseNCE(NCE);
            ExFreePool(RTE);
        }
        else {
            PrevRTE = &RTE->Next;
        }
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // Remove all cached routes using this neighbor as a router.
    // Note we must do this whether or not the neighbor
    // was a default router.
    //
    InvalidateRouteCache();

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* RouteTableUpdate - Update the route table.
//
//  Updates the route table by creating a new route
//  or modifying the lifetime of an existing route.
//
//  If the NCE is null, then the prefix is on-link.
//  Otherwise the NCE specifies the next hop.
//
//  REVIEW - Should we do anything special when we get identical
//  routes with different next hops? Currently they both end up
//  in the table, and FindRoute tries to pick the best one.
//
//  Note that the ValidLifetime may be INFINITE_LIFETIME,
//  whereas Neighbor Discovery does not allow an infinite value
//  for router lifetimes on the wire.
//
//  The Publish and Immortal boolean arguments control
//  the respective flag bits in the RTE.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
RouteTableUpdate(
    Interface *IF,
    NeighborCacheEntry *NCE,
    IPv6Addr *RoutePrefix,
    uint PrefixLength,
    uint SitePrefixLength,
    uint ValidLifetime,
    uint Pref,
    int Publish,
    int Immortal)
{
    IPv6Addr Prefix;
    RouteTableEntry *RTE, **PrevRTE;
    KIRQL OldIrql;

    ASSERT((NCE == NULL) || (NCE->IF == IF) || (NCE == LoopNCE));
    ASSERT((NCE != NULL) || (IF->Flags & IF_FLAG_DISCOVERS) || (IF == TunnelGetPseudoIF()));
    ASSERT(SitePrefixLength <= PrefixLength);

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, RoutePrefix, PrefixLength);

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    //
    // Search for an existing Route Table Entry.
    //
    for (PrevRTE = &RouteTable.First; ; PrevRTE = &RTE->Next) {
        RTE = *PrevRTE;

        if (RTE == NULL) {
            ASSERT(PrevRTE == RouteTable.Last);

            //
            // No existing entry for this prefix.
            // Create an entry if the lifetime is non-zero.
            //
            if (ValidLifetime != 0) {

                RTE = ExAllocatePool(NonPagedPool, sizeof *RTE);
                if (RTE == NULL)
                    break;

                RTE->Flags = 0;
                RTE->IF = IF;
                if (NCE != NULL)
                    AddRefNCE(NCE);
                RTE->NCE = NCE;
                RTE->Prefix = Prefix;
                RTE->PrefixLength = PrefixLength;
                RTE->SitePrefixLength = SitePrefixLength;
                RTE->ValidLifetime = ValidLifetime;
                RTE->Preference = Pref;
                if (Publish) {
                    RTE->Flags |= RTE_FLAG_PUBLISH;
                    ForceRouterAdvertisements = TRUE;
                }
                if (Immortal)
                    RTE->Flags |= RTE_FLAG_IMMORTAL;

                //
                // Add the new entry to the route table.
                //
                InsertRTEAtFront(RTE);

                //
                // Invalidate the route cache, so the new route
                // actually gets used.
                //
                InvalidateRouteCache();
            }
            break;
        }

        if ((RTE->IF == IF) && (RTE->NCE == NCE) &&
            IP6_ADDR_EQUAL(&RTE->Prefix, &Prefix) && 
            (RTE->PrefixLength == PrefixLength)) {
            //
            // We have an existing route.
            // Remove the route if the new lifetime is zero,
            // otherwise update the route.
            //
            if (ValidLifetime == 0) {
                //
                // Remove the RTE from the list.
                // See similar code in RouteTableTimeout.
                //
                RemoveRTE(PrevRTE, RTE);

                if (IsOnLinkRTE(RTE)) {
                    KdPrint(("Route RTE %x %s/%u -> IF %x removed\n", RTE,
                             FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                             RTE->IF));
                }
                else {
                    KdPrint(("Route RTE %x %s/%u -> NCE %x removed\n", RTE,
                             FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                             RTE->NCE));

                    ReleaseNCE(RTE->NCE);
                }

                //
                // If we are removing a published route,
                // resend Router Advertisements promptly.
                //
                if (RTE->Flags & RTE_FLAG_PUBLISH)
                    ForceRouterAdvertisements = TRUE;

                //
                // Release the RTE.
                //
                ExFreePool(RTE);

                //
                // Invalidate all cached routes.
                //
                InvalidateRouteCache();
            }
            else {
                //
                // If we are changing a published attribute of a route,
                // or if we are changing the publishing status of a route,
                // then resend Router Advertisements promptly.
                //
                if ((Publish &&
                     ((RTE->ValidLifetime != ValidLifetime) ||
                      (RTE->SitePrefixLength != SitePrefixLength))) ||
                    (!Publish != !(RTE->Flags & RTE_FLAG_PUBLISH)))
                    ForceRouterAdvertisements = TRUE;

                //
                // Pick up new attributes.
                //
                RTE->SitePrefixLength = SitePrefixLength;
                RTE->ValidLifetime = ValidLifetime;
                RTE->Flags = ((Publish ? RTE_FLAG_PUBLISH : 0) |
                              (Immortal ? RTE_FLAG_IMMORTAL : 0));
                if (RTE->Preference != Pref) {
                    RTE->Preference = Pref;
                    InvalidateRouteCache();
                }
            }
            break;
        }
    }

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* RouteTableReset
//
//  Reset the lifetimes of all auto-configured routes for an interface.
//  Also resets prefixes in the Site Prefix Table.
//
//  NB: We don't currently have an attribute on RTEs and SPEs
//  to tell us if we learned them from auto-configuration
//  or from manual configuration. Instead, we just reset
//  all RTEs and SPEs that do not have infinite lifetimes.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
RouteTableReset(Interface *IF, uint MaxLifetime)
{
    RouteTableEntry *RTE;
    SitePrefixEntry *SPE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql); 

    //
    // Reset all routes for this interface.
    //
    for (RTE = RouteTable.First; RTE != NULL; RTE = RTE->Next) {
        if (RTE->IF == IF) {
            //
            // Is this an auto-configured route?
            //
            if ((RTE->ValidLifetime != INFINITE_LIFETIME) &&
                (RTE->Flags == 0)) {
                //
                // Reset the lifetime to a small value.
                //
                if (RTE->ValidLifetime > MaxLifetime)
                    RTE->ValidLifetime = MaxLifetime;
            }
        }
    }

    //
    // Reset all site prefixes for this interface.
    //
    for (SPE = SitePrefixTable; SPE != NULL; SPE = SPE->Next) {
        if (SPE->IF == IF) {
            //
            // Is this an auto-configured site prefix?
            //
            if (SPE->ValidLifetime != INFINITE_LIFETIME) {
                //
                // Reset the lifetime to a small value.
                //
                if (SPE->ValidLifetime > MaxLifetime)
                    SPE->ValidLifetime = MaxLifetime;
            }
        }
    }

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* RouteTableRemove
//
//  Releases all routing state associated with the interface.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
RouteTableRemove(Interface *IF)
{
    RouteTableEntry *RTE, **PrevRTE;
    RouteCacheEntry *RCE, *NextRCE;
    SitePrefixEntry *SPE, **PrevSPE;
    BindingCacheEntry *BCE, **PrevBCE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql); 

    //
    // Remove routes for this interface.
    //
    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        if (RTE->IF == IF) {
            if (IsOnLinkRTE(RTE)) {
                KdPrint(("Route RTE %x %s/%u -> IF %x released\n", RTE,
                         FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                         RTE->IF));
            }
            else {
                KdPrint(("Route RTE %x %s/%u -> NCE %x released\n", RTE,
                         FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                         RTE->NCE));

                ReleaseNCE(RTE->NCE);
            }

            //
            // Remove the RTE from the list.
            //
            RemoveRTE(PrevRTE, RTE);

            //
            // Free the RTE.
            //
            ExFreePool(RTE);
        } 
        else {
            //
            // Move to the next RTE.
            //
            PrevRTE = &RTE->Next;
        }
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // Remove cached routes for this interface.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = NextRCE) {
        NextRCE = RCE->Next;
        if (RCE->NTE->IF == IF) {
            RemoveRCE(RCE);
            ReleaseRCE(RCE);
        }
    }

    //
    // Invalidate all cached routes.
    //
    InvalidateRouteCache();

    //
    // Remove all site prefixes for this interface.
    //
    PrevSPE = &SitePrefixTable;
    while ((SPE = *PrevSPE) != NULL) {

        if (SPE->IF == IF) {
            //
            // Remove the SPE from the list.
            //
            *PrevSPE = SPE->Next;

            //
            // Release the SPE.
            //
            ExFreePool(SPE);
        }
        else {
            //
            // Move to the next SPE.
            //
            PrevSPE = &SPE->Next;
        }
    }

    //
    // Remove binding cache entries for this interface.
    //
    PrevBCE = &BindingCache;
    while ((BCE = *PrevBCE) != NULL) {
        if (BCE->CareOfRCE->NTE->IF == IF)
            DestroyBCE(PrevBCE, BCE);
        else
            PrevBCE = &BCE->Next;
    }

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* FindBestSourceAddress
//
//  Given an outgoing interface and a destination address,
//  finds the best source address (NTE) to use.
//
//  May be called with the route cache lock held.
//
//  If found, returns a reference for the NTE.
//
NetTableEntry *
FindBestSourceAddress(
    Interface *IF,   // Interface we're sending from.
    IPv6Addr *Dest)  // Destination we're sending to.
{
    NetTableEntry *BestNTE = NULL;
    ushort Scope;
    uint Length, BestLength;
    NetTableEntry *NTE;
    AddressEntry *ADE;
    KIRQL OldIrql;

    Scope = AddressScope(Dest);

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
        //
        // Only consider unicast addresses.
        //
        if (ADE->Type != ADE_UNICAST)
            continue;
        NTE = (NetTableEntry *)ADE;

        //
        // We look for the source address which best matches
        // the destination address. If possible, we should use
        // a source address with the appropriate scope for
        // the destination. For example, if the destination address
        // is global, the source address should be global.
        // A link-local source address will not work, unless
        // the destination happens to be on-link.
        // Site-local addresses have the same concerns.
        //
        // If we have a choice of addresses of appropriate scope,
        // then we take a preferred address over a deprecated address.
        // We will use a deprecated address if necessary.
        //
        // As a final tie-breaker, we look at longest match
        // between the destination address and source address.
        //
        if (IsValidNTE(NTE)) { // PREFERRED or DEPRECATED
            Length = CommonPrefixLength(Dest, &NTE->Address);
            if (BestNTE == NULL) {
                //
                // We don't have a choice yet, so take what we can get.
                //
                AddRefNTE(NTE);
                BestNTE = NTE;
                BestLength = Length;
            }
            else if ((BestNTE->Scope == NTE->Scope) ?
                     ((BestNTE->DADState == NTE->DADState) ?
                      (BestLength < Length) :
                      (BestNTE->DADState < NTE->DADState)) :
                     ((BestNTE->Scope < NTE->Scope) ?
                      (BestNTE->Scope < Scope) :
                      (Scope <= NTE->Scope))) {
                //
                // This source address looks better.
                //
                AddRefNTE(NTE);
                ReleaseNTE(BestNTE);
                BestNTE = NTE;
                BestLength = Length;
            }
        }
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return BestNTE;
}


//* RouteTableTimeout
//
//  Called periodically from IPv6Timeout.
//  Handles lifetime expiration of routing table entries.
//
void
RouteTableTimeout(void)
{
    RouteTableEntry *RTE, **PrevRTE;
#if DBG
    RouteCacheEntry *RCE;
    uint RCECount;
#endif

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        if (RTE->ValidLifetime == 0) {
            //
            // Remove the RTE from the list.
            // See similar code in RouteTableUpdate.
            //
            RemoveRTE(PrevRTE, RTE);

            if (IsOnLinkRTE(RTE)) {
                KdPrint(("Route RTE %x %s/%u -> IF %x timed out\n", RTE,
                         FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                         RTE->IF));
            }
            else {
                KdPrint(("Route RTE %x %s/%u -> NCE %x timed out\n", RTE,
                         FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                         RTE->NCE));

                ReleaseNCE(RTE->NCE);
            }

            //
            // If we are removing a published route,
            // resend Router Advertisements promptly.
            //
            if (RTE->Flags & RTE_FLAG_PUBLISH)
                ForceRouterAdvertisements = TRUE;

            //
            // Release the RTE.
            //
            ExFreePool(RTE);

            //
            // Invalidate all cached routes.
            //
            InvalidateRouteCache();
        }
        else {
            if ((RTE->ValidLifetime != INFINITE_LIFETIME) &&
                !(RTE->Flags & RTE_FLAG_IMMORTAL))
                RTE->ValidLifetime--;

            PrevRTE = &RTE->Next;
        }
    }
    ASSERT(PrevRTE == RouteTable.Last);

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
}


//* SitePrefixUpdate
//
//  Updates the site prefix table by creating a new site prefix
//  or modifying the lifetime of an existing site prefix.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
SitePrefixUpdate(
    Interface *IF,
    IPv6Addr *SitePrefix,
    uint SitePrefixLength,
    uint ValidLifetime)
{
    IPv6Addr Prefix;
    SitePrefixEntry *SPE, **PrevSPE;
    KIRQL OldIrql;

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, SitePrefix, SitePrefixLength);

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    //
    // Search for an existing Site Prefix Entry.
    //
    for (PrevSPE = &SitePrefixTable; ; PrevSPE = &SPE->Next) {
        SPE = *PrevSPE;

        if (SPE == NULL) {
            //
            // No existing entry for this prefix.
            // Create an entry if the lifetime is non-zero.
            //
            if (ValidLifetime != 0) {

                SPE = ExAllocatePool(NonPagedPool, sizeof *SPE);
                if (SPE == NULL)
                    break;

                SPE->IF = IF;
                SPE->Prefix = Prefix;
                SPE->SitePrefixLength = SitePrefixLength;
                SPE->ValidLifetime = ValidLifetime;

                //
                // Add the new entry to the table.
                //
                SPE->Next = SitePrefixTable;
                SitePrefixTable = SPE;
            }
            break;
        }

        if ((SPE->IF == IF) &&
            IP6_ADDR_EQUAL(&SPE->Prefix, &Prefix) && 
            (SPE->SitePrefixLength == SitePrefixLength)) {
            //
            // We have an existing site prefix.
            // Remove the prefix if the new lifetime is zero,
            // otherwise update the prefix.
            //
            if (ValidLifetime == 0) {
                //
                // Remove the SPE from the list.
                // See similar code in SitePrefixTimeout.
                //
                *PrevSPE = SPE->Next;

                //
                // Release the SPE.
                //
                ExFreePool(SPE);
            }
            else {
                //
                // Pick up new attributes.
                //
                SPE->ValidLifetime = ValidLifetime;
            }
            break;
        }
    }

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* SitePrefixMatch
//
//  Checks the destination address against
//  the prefixes in the Site Prefix Table.
//  If there is a match, returns the site identifier
//  associated with the matching prefix.
//  If there is no match, returns zero.
//
//  Callable from a thread or DPC context.
//  Called with no locks held.
//  
uint
SitePrefixMatch(IPv6Addr *Destination)
{
    SitePrefixEntry *SPE;
    KIRQL OldIrql;
    uint MatchingSite = 0;

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    for (SPE = SitePrefixTable; SPE != NULL; SPE = SPE->Next) {
        //
        // Does this site prefix match the destination address?
        //
        if (HasPrefix(Destination, &SPE->Prefix, SPE->SitePrefixLength)) {
            //
            // We have found a matching site prefix.
            // No need to look further.
            //
            MatchingSite = SPE->IF->Site;
            break;
        }
    }
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    return MatchingSite;
}


//* SitePrefixTimeout
//
//  Called periodically from IPv6Timeout.
//  Handles lifetime expiration of site prefixes.
//
void
SitePrefixTimeout(void)
{
    SitePrefixEntry *SPE, **PrevSPE;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    PrevSPE = &SitePrefixTable;
    while ((SPE = *PrevSPE) != NULL) {

        if (SPE->ValidLifetime == 0) {
            //
            // Remove the SPE from the list.
            //
            *PrevSPE = SPE->Next;

            //
            // Release the SPE.
            //
            ExFreePool(SPE);
        }
        else {
            if (SPE->ValidLifetime != INFINITE_LIFETIME)
                SPE->ValidLifetime--;

            PrevSPE = &SPE->Next;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
}


//* ConfirmForwardReachability - tell ND that packets are getting through.
//
//  Upper layers call this routine upon receiving acknowledgements that
//  data sent by this node has arrived recently at the peer represented
//  by this RCE.  Such acknowledgements are considered to be proof of
//  forward reachability for the purposes of Neighbor Discovery.
//
//  Caller should be holding a reference on the RCE.
//  REVIEW: Assume caller is a DPC and use KeAcquireSpinLockAtDpcLevel?
//
void
ConfirmForwardReachability(RouteCacheEntry *RCE)
{
    NeighborCacheEntry *NCE;  // First-hop neighbor for this route.
    Interface *IF;            // Interface we use to reach this neighbor.
    KIRQL OldIrql;            // Temporary place to stash the interrupt level.

    NCE = RCE->NCE;
    IF = NCE->IF;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    switch (NCE->NDState) {
    case ND_STATE_INCOMPLETE:
        //
        // This is strange. Perhaps the reachability confirmation is
        // arriving very late and ND has already decided the neighbor
        // is unreachable? Or perhaps the upper-layer protocol is just
        // mistaken? In any case ignore the confirmation.
        //
        break;

    case ND_STATE_PROBE:
    case ND_STATE_DELAY:
        //
        // Cancel probe timeout.
        //
        NCE->EventTimer = 0;
        // fall-through

    case ND_STATE_STALE:
        //
        // We have forward reachability.
        //
        NCE->NDState = ND_STATE_REACHABLE;

        if (NCE->IsUnreachable) {
            //
            // We can get here if an NCE is reachable but goes INCOMPLETE.
            // Then we later receive passive information and the state
            // changes to STALE. Then we receive upper-layer confirmation
            // that the neighbor is reachable again.
            //
            // We had previously concluded that this neighbor
            // is unreachable. Now we know otherwise.
            //
            NCE->IsUnreachable = FALSE;
            InvalidateRouteCache();
        }
        // fall-through

    case ND_STATE_REACHABLE:
        //
        // Timestamp this reachability confirmation.
        //
        NCE->LastReachable = IPv6TickCount;
        // fall-through

    case ND_STATE_PERMANENT:
        //
        // Ignore the confirmation.
        //
        ASSERT(NCE->WaitQueue == NULL);
        ASSERT(NCE->EventTimer == 0);
        ASSERT(! NCE->IsUnreachable);
        break;

    default:
        ASSERT(!"Invalid ND state?");
        break;
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);
}


//* GetPathMTUFromRCE - lookup MTU to use in sending on this route.
//
//  Get the PathMTU from an RCE.
//
//  Note that PathMTU is volatile unless the RouteCacheLock
//  is held. Furthermore the Interface's LinkMTU may have changed
//  since the RCE was created, due to a Router Advertisement.
//  (LinkMTU is always volatile.)
//
//  Callable from a thread or DPC context.
//  Called with no locks held. (?)
//
uint
GetPathMTUFromRCE(RouteCacheEntry *RCE)
{
    uint PathMTU, LinkMTU;
    KIRQL OldIrql;

    LinkMTU = RCE->NCE->IF->LinkMTU;
    PathMTU = RCE->PathMTU;

    //
    // We lazily check to see if it's time to probe for an increased Path
    // MTU as this is perceived to be cheaper than routinely running through
    // all our RCEs looking for one whose PMTU timer has expired.
    //
    if ((RCE->PMTULastSet != 0) &&
        ((uint)(IPv6TickCount - RCE->PMTULastSet) >= PATH_MTU_RETRY_TIME)) {
        //
        // It's been at least 10 minutes since we last lowered our PMTU
        // as the result of receiving a Path Too Big message.  Bump it
        // back up to the Link MTU to see if the path is larger now.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        PathMTU = RCE->PathMTU = LinkMTU;
        RCE->PMTULastSet = 0;
        RCE->Flags &= ~RCE_FLAG_FRAGMENT_HEADER_NEEDED;
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    //
    // We lazily check to see if our Link MTU has shrunk below our Path MTU,
    // as this is perceived to be cheaper than running through all our RCEs 
    // looking for a too big Path MTU when a Link MTU shrinks.
    //
    // REVIEW: A contrarian might point out that Link MTUs rarely (if ever)
    // REVIEW: shrink, whereas we do this check on every packet sent.
    //
    if (PathMTU > LinkMTU) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        LinkMTU = RCE->NCE->IF->LinkMTU;
        PathMTU = RCE->PathMTU;
        if (PathMTU > LinkMTU) {
            PathMTU = RCE->PathMTU = LinkMTU;
            RCE->PMTULastSet = 0;
            //
            // A fragment header can only be required when the PathMTU
            // is IPv6_MINIMUM_MTU, which it can't be in this code path
            // because PathMTU is bigger than LinkMTU.
            //
            ASSERT((RCE->Flags & RCE_FLAG_FRAGMENT_HEADER_NEEDED) == 0);
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    return PathMTU;
}


//* UpdatePathMTU
//
//  Update the route cache with a new MTU obtained
//  from a Packet Too Big message.
//
//  Callable from DPC context, not from thread context.
//  Called with no locks held.
//
void
UpdatePathMTU(
    Interface *IF,
    IPv6Addr *Dest,
    uint MTU)
{
    RouteCacheEntry *RCE;
    uint Now;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the route cache for the appropriate RCE.
    // There will be at most one.
    //

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {

        if (IP6_ADDR_EQUAL(&RCE->Destination, Dest) &&
            (RCE->NTE->IF == IF)) {

            //
            // As a sanity check, exclude loop-back RCEs.
            //
            if (RCE->NCE == LoopNCE)
                break;

            //
            // Update the path MTU.
            //
            if (MTU < RCE->PathMTU) {
                KdPrint(("UpdatePathMTU(RCE %x): new MTU %u for %s\n",
                         RCE, MTU, FormatV6Address(Dest)));
                if (MTU < IPv6_MINIMUM_MTU) {
                    RCE->PathMTU = IPv6_MINIMUM_MTU;
                    RCE->Flags |= RCE_FLAG_FRAGMENT_HEADER_NEEDED;
                } else {
                    RCE->PathMTU = MTU;
                }
                Now = IPv6TickCount;
                if (Now == 0)
                    Now = 1;
                RCE->PMTULastSet = Now;  // Timestamp it (starts timer).
            }
            break;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
}


//* RedirectRouteCache
//
//  Update the route cache to reflect a Redirect message.
//
//  Callable from DPC context, not from thread context.
//  Called with no locks held.
//
IP_STATUS  // Returns: IP_SUCCESS if redirect was legit, otherwise failure.
RedirectRouteCache(
    IPv6Addr *Source,           // Source of the redirect.
    IPv6Addr *Dest,             // Destination that is being redirected.
    Interface *IF,              // Interface that this all applies to.
    NeighborCacheEntry *NCE)    // New router for the destination.
{
    NetTableEntry *NTE;
    RouteCacheEntry *RCE;
    RouteCacheEntry *OldRCE = NULL;
    IP_STATUS ReturnValue = IP_SUCCESS;
#if DBG
    char Buffer1[40], Buffer2[40];
#endif

    //
    // Our caller guarantees this.
    //
    ASSERT(IF == NCE->IF);

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the route cache for the appropriate RCE.
    // There will be at most one.
    //

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {

        if (IP6_ADDR_EQUAL(&RCE->Destination, Dest) &&
            (RCE->NTE->IF == IF)) {

            //
            // We must check that the source of the redirect
            // is the current next-hop neighbor.
            // (This prevents other nodes on a link from hijacking.)
            //
            if (!IP6_ADDR_EQUAL(&RCE->NCE->NeighborAddress, Source)) {
                KdPrint(("RedirectRouteCache - hijack attempt\n"));
                ReturnValue = IP_GENERAL_FAILURE;  // REVIEW: What error code?
                goto Return;
            }

            //
            // No need to check for a null redirect (NCE == RCE->NCE).
            // Shouldn't happen and if it does nothing bad happens.
            // As an optimization, we could check if the RCE
            // has only one reference and if so, update it in place.
            // However I suspect that it isn't worth the code.
            //

            //
            // Cause anyone who is using/caching this RCE to notice
            // that it is no longer valid, so they will find
            // the new RCE that we create below.
            //
            RCE->Valid--;

            //
            // Remove the RCE from the cache (CreateOrReuseRoute below
            // will not find it) but hold onto it for the moment.
            //
            RemoveRCE(RCE);
            OldRCE = RCE;
            break;
        }
    }

#if DBG
    FormatV6AddressWorker(Buffer1, Dest);
    FormatV6AddressWorker(Buffer2, &NCE->NeighborAddress);
    KdPrint(("RedirectRouteCache - redirect %s to %s\n", Buffer1, Buffer2));
#endif

    //
    // Create a route cache entry for the redirect.
    // (If we get any errors, we just ignore the redirect,
    // except for removing the old RCE.)
    //
    RCE = CreateOrReuseRoute();
    if (RCE == NULL)
        goto Return;

    //
    // Find the best source address for this destination.
    // (The NTE from our caller might not be the best.)
    //
    NTE = FindBestSourceAddress(IF, Dest);
    if (NTE == NULL) {
        //
        // We have no valid source address to use!
        //
        ExFreePool(RCE);
        goto Return;
    }

    AddRefNCE(NCE);
    RCE->NCE = NCE;
    RCE->NTE = NTE;     // Donate ref from FindBestSourceAddress.
    RCE->PathMTU = IF->LinkMTU;
    RCE->PMTULastSet = 0;  // Reduced PMTU timer not running.
    RCE->Destination = *Dest;
    RCE->Type = RCE_TYPE_REDIRECT;
    //
    // REVIEW - Should we allow redirects of link-local destinations?
    // Is zero always correct for non-link-local destinations?
    //
    RCE->Flags = 0;
    RCE->LastError = 0;
    RCE->CareOfRCE = FindBindingCacheRCE(Dest);
    RCE->Valid = RouteCacheValidationCounter;

    //
    // Copy state from a previous RCE for this destination,
    // if we have it and the state is relevant.
    //
    if (OldRCE != NULL) {
        //
        // PathMTU state is not relevant, since the new RCE
        // presumably specifies a new route.
        //
        // REVIEW: Take Flags?  Some flag may be relevant someday?
        //
        RCE->LastError = OldRCE->LastError;
    }

    //
    // Add the new route cache entry to the cache.
    //
    InsertRCE(RCE);

  Return:
    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
    if (OldRCE != NULL)
        ReleaseRCE(OldRCE);     // The cache's reference.
    return ReturnValue;
}


//* InitRouting - Initialize the routing module.
//
void
InitRouting(void)
{
    KeInitializeSpinLock(&RouteCacheLock);

    RouteCache.Limit = 32;
    RouteCache.First = RouteCache.Last = SentinelRCE;

    RouteTable.First = NULL;
    RouteTable.Last = &RouteTable.First;
}


//* UnloadRouting
//
//  Called when IPv6 stack is unloading.
//
void
UnloadRouting(void)
{
    //
    // With all the interfaces destroyed,
    // there should be no routes left.
    //
    ASSERT(RouteTable.First == NULL);
    ASSERT(RouteTable.Last == &RouteTable.First);
    ASSERT(RouteCache.First == SentinelRCE);
    ASSERT(RouteCache.Last == SentinelRCE);

    //
    // BUGBUG - Cleanup binding cache?
    //
}


//* UpdateCareOfRCE
//
//  Replaces the existing care-of RCE with the new care-of
//  RCE for all entries in the Route Cache for the given home address.
//  This function must be called with the RouteCache lock held.
//
void
UpdateCareOfRCE(
    IPv6Addr *HomeAddr,
    RouteCacheEntry *NewCareOfRCE)      // May be NULL.
{
    RouteCacheEntry *RCE;

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {
        if (IP6_ADDR_EQUAL(&RCE->Destination, HomeAddr)) {
            //
            // Found an entry for the home address.
            // Replace the care-of address.
            //
            if (RCE->CareOfRCE != NULL)
                ReleaseRCE(RCE->CareOfRCE);
            RCE->CareOfRCE = NewCareOfRCE;
            if (NewCareOfRCE != NULL)
                AddRefRCE(NewCareOfRCE);
        }
    }
}


//* CreateBindingCacheEntry - create new BCE.
//
//  Allocates a new Binding Cache entry,
//  and adds it to the front of the cache.
//
//  Must be called with the RouteCache lock held.
//
BindingCacheEntry *
CreateBindingCacheEntry()
{
    BindingCacheEntry *BCE;

    BCE = ExAllocatePool(NonPagedPool, sizeof *BCE); 
    if (BCE == NULL)
        return NULL;
    BCE->Next = BindingCache;
    BindingCache = BCE;

    return BCE;
}


//* DestroyBCE - remove an entry from the BindingCache.
//
//  Must be called with the RouteCache lock held.
//
void
DestroyBCE(
    BindingCacheEntry **PrevBCE,
    BindingCacheEntry *BCE)
{
    //
    // First update the RouteCache to reflect that there's no longer 
    // a binding present.
    //
    UpdateCareOfRCE(&BCE->HomeAddr, NULL);

    //
    // Unchain the given BCE and destroy it.
    //
    *PrevBCE = BCE->Next;
    ReleaseRCE(BCE->CareOfRCE);
    ExFreePool(BCE);
}


//* FindBindingCache - find BCE with specific care-of address.
//
//  Looks for a binding cache entry with the specified care-of address. 
//  Must be called with the route cache lock held.
//
BindingCacheEntry *
FindBindingCache(
    IPv6Addr *HomeAddr)
{
    BindingCacheEntry *BCE;

    BCE = BindingCache;
    while (BCE != NULL) {
        if (IP6_ADDR_EQUAL(&BCE->HomeAddr, HomeAddr)) {
            //
            // Found a matching entry.
            //
            break;
        }
        
        //
        // Keep looking.
        //
        BCE = BCE->Next;
    }

    return BCE;
}


//* FindBindingCacheRCE - find RCE for BCE with specific care-of address.
//
//  Looks for a binding cache entry with the specified care-of address.
//  If found the corresponding RCE is returned.
//  Must be called with the route cache lock held.
//
RouteCacheEntry *
FindBindingCacheRCE(
    IPv6Addr *HomeAddr)
{
    RouteCacheEntry *RCE = NULL;
    BindingCacheEntry *BCE;

    BCE = BindingCache;
    while (BCE != NULL) {
        if (IP6_ADDR_EQUAL(&BCE->HomeAddr, HomeAddr)) {
            //
            // Found a matching entry.
            //
            RCE = BCE->CareOfRCE;
            AddRefRCE(RCE);
            break;
        }
        
        //
        // Keep looking.
        //
        BCE = BCE->Next;
    }

    return RCE;
}


//* CacheBindingUpdate - update the binding cache entry for an address.
//
//  Find or Create (if necessary) an RCE to the CareOfAddress.  This routine
//  is called in response to a Binding Cache Update.  
//
//  Callable from DPC context, not from thread context.
//  Called with no locks held.
//
IP_STATUS  // Returns: IP_SUCCESS if update took, otherwise failure.
CacheBindingUpdate(
    IPv6BindingUpdateOption *BindingUpdate,  // The binding update option.
    IPv6Addr *CareOfAddr,                    // Address to use for mobile node.
    uint CareOfScopeId,                      // Scope id for CareOfAddr.
    IPv6Addr *HomeAddr)                      // Mobile node's home address.
{
    BindingCacheEntry *BCE, **PrevBCE;
    IP_STATUS ReturnValue = IP_SUCCESS;
    uchar DeleteRequest = FALSE;  // Request is to delete an existing binding?
    uchar BindingFound = FALSE;   // Existing binding for this home addr found?
    ushort SeqNo;

    //
    // Is this Binding Update a request to remove entries
    // from our binding cache?
    //
    if (BindingUpdate->Lifetime == 0 || IP6_ADDR_EQUAL(HomeAddr, CareOfAddr))
        DeleteRequest = TRUE;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the binding cache for the home address.
    //
    BCE = BindingCache;
    PrevBCE = &BindingCache;
    while (BCE != NULL) {

        if (!IP6_ADDR_EQUAL(&BCE->HomeAddr, HomeAddr)) {
            PrevBCE = &BCE->Next;
            BCE = BCE->Next;
            continue;
        }

        // 
        // We've found an existing entry for this home address.
        // Verify the sequence number is greater than the cached binding's
        // sequence number if there is one.
        //
        SeqNo = net_short(BindingUpdate->SeqNumber);
        if (BCE->BindingSeqNumber > 0 && SeqNo <= BCE->BindingSeqNumber) {
            KdPrint(("CacheBindingUpdate: New sequence number too small "
                     "(old seqnum = %d, new seqnum = %d)\n",
                     BCE->BindingSeqNumber, SeqNo));
            ReturnValue = IPV6_BINDING_SEQ_NO_TOO_SMALL;
            goto Return;
        }

        //
        // If the request is to delete the entry, do so and return.
        //
        if (DeleteRequest) {
            DestroyBCE(PrevBCE, BCE);
            goto Return;
        }       
        
        //
        // Update the binding.
        //
        BCE->BindingLifetime = 
            ConvertSecondsToTicks(net_long(BindingUpdate->Lifetime));
        BCE->BindingSeqNumber = SeqNo;
        BindingFound = TRUE;
        
        //
        // If the care-of address has changed, then we need to create a 
        // new care-of RCE to the new care-of address.
        //
        // BUGBUG: Doesn't check for the highly improbable(?) case that only
        // BUGBUG: the scope id of the care-of address changed.
        //
        if (!IP6_ADDR_EQUAL(&BCE->CareOfRCE->Destination, CareOfAddr)) {
            ReleaseRCE(BCE->CareOfRCE);
            BCE->CareOfRCE = NULL;
            ReturnValue = RouteToDestination(CareOfAddr, CareOfScopeId, NULL,
                                             RTD_FLAG_NORMAL, &BCE->CareOfRCE);

            //
            // Update all applicable RCEs.  Since RouteToDestination only
            // returns an RCE upon success, this has the added effect of
            // removing the binding from the route cache upon failure.
            //
            UpdateCareOfRCE(HomeAddr, BCE->CareOfRCE);
        }
        break;
    }
    
    if (DeleteRequest || BindingFound) {
        //
        // We're done.
        //
        goto Return;
    }


    //
    // We want to cache a binding and did not find an existing binding
    // for the home address above.  So we create a new binding cache entry.
    //
    BCE = CreateBindingCacheEntry();
    if (BCE == NULL) {
        ReturnValue = IP_NO_RESOURCES;
        goto Return;
    }
    BCE->HomeAddr = *HomeAddr;
    BCE->BindingLifetime = 
            ConvertSecondsToTicks(net_long(BindingUpdate->Lifetime));
    BCE->BindingSeqNumber = net_short(BindingUpdate->SeqNumber);

    // Now create a new RCE for the care-of address.
    ReturnValue = RouteToDestination(CareOfAddr, CareOfScopeId, NULL,
                                     RTD_FLAG_NORMAL, &BCE->CareOfRCE);
    if (ReturnValue != IP_SUCCESS) {
        //
        // Couldn't get a route.  Give up and delete the entry we were
        // creating; it will be at the front of the cache list.
        //
        DestroyBCE(&BindingCache, BCE);
    } else {
        //
        // Update all applicable RCEs.
        //
        UpdateCareOfRCE(HomeAddr, BCE->CareOfRCE);
    }

  Return:
    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
    return ReturnValue;
}


//* BindingCacheTimeout
//
//  Check for and handle binding cache lifetime expirations.
//
//  Callable from DPC context, not from thread context.
//  Called with no locks held.
//
void
BindingCacheTimeout(void)
{
    BindingCacheEntry *BCE, **PrevBCE;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the route cache for all binding cache entries. Update
    // their lifetimes, and remove if expired.
    //
    PrevBCE = &BindingCache;
    while ((BCE = *PrevBCE) != NULL) {

        //
        // REVIEW: The mobile IPv6 spec allows correspondent nodes to 
        // REVIEW: send a Binding Request when the current binding's
        // REVIEW: lifetime is "close to expiration" in order to prevent
        // REVIEW: the overhead of establishing a new binding after the
        // REVIEW: current one expires.  For now, we just let the binding
        // REVIEW: expire.
        //

        if (--BCE->BindingLifetime == 0) {
            //
            // This binding cache has expired.  Remove it from
            // the Binding Cache.
            //
            DestroyBCE(PrevBCE, BCE);
            continue;
        }

        PrevBCE = &BCE->Next;
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
}

//* RouterAdvertSend
//
//  Sends a Router Advertisement.
//  The advert is always sent to the all-nodes multicast address.
//  Chooses a valid source address for the interface.
//
//  Called with no locks held.
//
//  REVIEW - Should this function be in route.c or neighbor.c? Or split up?
//
void
RouterAdvertSend(Interface *IF)
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    uint SourceOptionLength;
    IPv6Header *IP;
    ICMPv6Header *ICMP;
    NDRouterAdvertisement *RA;
    void *SourceOption;
    NDOptionMTU *MTUOption;
    void *LLDest;
    IPv6Addr Source;
    KIRQL OldIrql;
    int Forwards;
    uint LinkMTU;
    uint NumPrefixes, i;
    uint RouterLifetime;
    NDOptionPrefixInformation *Prefix;
    RouteTableEntry *RTE;

    //
    // We can only send a router advertisement
    // if the interface has a valid link-local address.
    //
    if (!GetLinkLocalAddress(IF, &Source)) {
        KdPrint(("RouterAdvertSend - no link-local address?\n"));
        return;
    }

    //
    // For consistency, capture some volatile
    // information in locals.
    //
    Forwards = IF->Flags & IF_FLAG_FORWARDS;
    LinkMTU = IF->LinkMTU;

    //
    // Decide up front what will go in this advertisement,
    // so we know how big to make it. We want to know
    // how many prefixes/routes we will advertise.
    //
    NumPrefixes = 0;
    RouterLifetime = 0;
    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    for (RTE = RouteTable.First; RTE != NULL; RTE = RTE->Next) {
        //
        // We only pay attention to advertised routes.
        //
        if (RTE->Flags & RTE_FLAG_PUBLISH) {
            if (IsOnLinkRTE(RTE)) {
                if (RTE->IF == IF)
                    NumPrefixes++;
                else if (Forwards)
                    NumPrefixes++;
            }
            else {
                //
                // We only advertise routes if we are forwarding.
                // Note that we do NOT check IF != RTE->NCE->IF...
                // this means if such a route is published and used,
                // we will generate a Redirect.
                //
                if (Forwards) {
                    //
                    // We don't explicitly advertise zero-length prefixes.
                    // Instead we advertise a non-zero router lifetime.
                    //
                    if (RTE->PrefixLength == 0) {
                        if (RTE->ValidLifetime > RouterLifetime)
                            RouterLifetime = RTE->ValidLifetime;
                    }
                    else
                        NumPrefixes++;
                }
            }
        }
    }

    //
    // Calculate a payload length for the advertisement.
    // In addition to the 8 bytes for the advert proper, leave space
    // for source link-layer address option (round option length up to
    // 8-byte multiple), the MTU option, and the prefix-information options.
    // If we can't send everything we want, we'll just truncate.
    //
    SourceOptionLength = (IF->LinkAddressLength + 2 + 7) &~ 7;
    PayloadLength = sizeof *ICMP + sizeof *RA +
        SourceOptionLength + sizeof(NDOptionMTU);

    //
    // REVIEW - Restrict the number of prefixes that we send,
    // to keep the packet from being too large.
    // Really we should send multiple RAs in this situation.
    //
    if ((sizeof(IPv6Header) + PayloadLength +
         (sizeof(NDOptionPrefixInformation) * NumPrefixes)) > LinkMTU) {
        NumPrefixes = ((LinkMTU - sizeof(IPv6Header) - PayloadLength) /
                       sizeof(NDOptionPrefixInformation));
    }

    PayloadLength += sizeof(NDOptionPrefixInformation) * NumPrefixes;

    //
    // Allocate a packet for the advertisement.
    //
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
        return;
    }

    //
    // Prepare IP header of the advertisement.
    //
    IP = (IPv6Header *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = Source;
    IP->Dest = AllNodesOnLinkAddr;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header *)(IP + 1);
    ICMP->Type = ICMPv6_ROUTER_ADVERT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Prepare the Router Advertisement header.
    //
    RA = (NDRouterAdvertisement *)(ICMP + 1);
    RtlZeroMemory(RA, sizeof *RA);

    if (RouterLifetime != 0) {
        //
        // We will be a default router. Calculate the 16-bit lifetime.
        // Note that there is no infinite value on the wire.
        //
        RouterLifetime = ConvertTicksToSeconds(RouterLifetime);
        if (RouterLifetime > 0xffff)
            RouterLifetime = 0xffff;
        RA->RouterLifetime = net_short((ushort)RouterLifetime);
    }

    //
    // Always include source link-layer address option.
    //
    SourceOption = (void *)(RA + 1);
    ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
    ((uchar *)SourceOption)[1] = SourceOptionLength >> 3;
    (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);

    //
    // Always include MTU option.
    //
    MTUOption = (NDOptionMTU *)
        ((uchar *)SourceOption + SourceOptionLength);
    MTUOption->Type = ND_OPTION_MTU;
    MTUOption->Length = 1;
    MTUOption->Reserved = 0;
    MTUOption->MTU = net_long(LinkMTU);

    //
    // Scan the routing table again,
    // building the prefix options.
    //
    Prefix = (NDOptionPrefixInformation *)(MTUOption + 1);
    for (RTE = RouteTable.First, i = 0;
         (RTE != NULL) && (i < NumPrefixes);
         RTE = RTE->Next) {
        //
        // We only pay attention to advertised routes.
        //
        if (RTE->Flags & RTE_FLAG_PUBLISH) {
            uint Lifetime;

            if (IsOnLinkRTE(RTE)) {
                if (RTE->IF == IF) {
                    Prefix[i].Flags = ND_PREFIX_FLAG_ON_LINK;
                    if (RTE->PrefixLength == 64)
                        Prefix[i].Flags |= ND_PREFIX_FLAG_AUTONOMOUS;
                    goto InitializePrefix;
                }
                else if (Forwards) {
                    Prefix[i].Flags = ND_PREFIX_FLAG_ROUTE;
                    goto InitializePrefix;
                }
            }
            else {
                //
                // We only advertise routes if we are forwarding.
                // We don't explicitly advertise zero-length prefixes.
                // Instead we advertise a non-zero router lifetime.
                //
                if (Forwards && (RTE->PrefixLength != 0)) {
                    Prefix[i].Flags = ND_PREFIX_FLAG_ROUTE;
                  InitializePrefix:
                    Prefix[i].Type = ND_OPTION_PREFIX_INFORMATION;
                    Prefix[i].Length = 4;
                    Prefix[i].PrefixLength = RTE->PrefixLength;
                    Prefix[i].Reserved2 = 0;
                    Prefix[i].Prefix = RTE->Prefix;

                    //
                    // Is this also a site prefix?
                    // NB: The SitePrefixLength field overlaps Reserved2.
                    //
                    if (RTE->SitePrefixLength != 0) {
                        Prefix[i].Flags |= ND_PREFIX_FLAG_SITE_PREFIX;
                        Prefix[i].SitePrefixLength = RTE->SitePrefixLength;
                    }

                    //
                    // ConvertTicksToSeconds preserves the infinite value.
                    // REVIEW: Make the preferred lifetime smaller?
                    //
                    Lifetime = net_long(ConvertTicksToSeconds(RTE->ValidLifetime));
                    Prefix[i].ValidLifetime = Lifetime;
                    Prefix[i].PreferredLifetime = Lifetime;

                    i++;
                }
            }
        }
    }
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    //
    // Because this is an advertising interface,
    // it will ignore its own RA as well as other RAs.
    // However we would like to autoconfigure addresses
    // from the A-bit prefix options in our own RA.
    // So before sending the Router Advertisement,
    // we make another pass over the prefix options,
    // essentially pretending we are receiving those options.
    //
    for (i = 0; i < NumPrefixes; i++) {
        uint Lifetime;

        //
        // Because we just constructed the prefix-information options,
        // we know they are syntactically valid.
        //
        ASSERT(Prefix[i].ValidLifetime == Prefix[i].PreferredLifetime);
        Lifetime = net_long(Prefix[i].ValidLifetime);
        Lifetime = ConvertSecondsToTicks(Lifetime);

        if (Prefix[i].Flags & ND_PREFIX_FLAG_AUTONOMOUS) {
            IPv6Addr NewAddr;
            NetTableEntry *NTE;

            ASSERT(Prefix[i].PrefixLength == 64);

            //
            // Create the new address from the prefix & interface token.
            //
            NewAddr = Prefix[i].Prefix;
            (*IF->CreateToken)(IF->LinkContext, IF->LinkAddress, &NewAddr);

            //
            // REVIEW - Will the new address always be "proper"?
            // Depends on what prefixes we allow in the route table.
            //
            (void) AddrConfUpdate(IF, &NewAddr, TRUE,
                                  Lifetime, Lifetime, TRUE, &NTE);
            if (NTE != NULL) {
                //
                // Create the subnet anycast address for this prefix.
                //
                CopyPrefix(&NewAddr, &Prefix[i].Prefix, 64);
                (void) FindOrCreateAAE(IF, &NewAddr, CastFromNTE(NTE));
                ReleaseNTE(NTE);
            }
        }

        if (Prefix[i].Flags & ND_PREFIX_FLAG_SITE_PREFIX) {
            ASSERT(Prefix[i].SitePrefixLength <= Prefix[i].PrefixLength);

            SitePrefixUpdate(IF, &Prefix[i].Prefix,
                             Prefix[i].SitePrefixLength, Lifetime);
        }
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
