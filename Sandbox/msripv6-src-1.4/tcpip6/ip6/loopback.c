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
// IPv6 loopback interface psuedo-driver.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "route.h"
#include "icmp.h"
#include "neighbor.h"
#include "security.h"

#define LOOPBACK_MTU    1500    // Same as ethernet.

KSPIN_LOCK LoopLock;
PNDIS_PACKET LoopTransmitHead = (PNDIS_PACKET)NULL;
PNDIS_PACKET LoopTransmitTail = (PNDIS_PACKET)NULL;
WORK_QUEUE_ITEM LoopWorkItem;
uint LoopTransmitRunning = 0;

Interface LoopInterface;      // Loopback interface.
NeighborCacheEntry *LoopNCE;  // Loopback NCE.
NetTableEntry *LoopNTE;       // Loopback NTE.

#ifdef ALLOC_PRAGMA
//
// Make init code disposable.
//
NetTableEntry *LoopbackInit(void);

#pragma alloc_text(INIT, LoopbackInit)

#endif // ALLOC_PRAGMA

//* LoopTransmit - Loopback transmit event routine.
//
//  This is the work item callback routine called for a loopback transmit.
//  Pull packets off the transmit queue and "send" them to ourselves
//  by the expedient of receiving them locally.
//
void                  // Returns: Nothing.
LoopTransmit(
    void *Context)    // Pointer to loopback IF.
{
    KIRQL OriginalIrql, OldIrql;
    PNDIS_PACKET Packet;
    IPv6Packet IPPacket;
    int Rcvd = FALSE;
    int PktRefs;  // Packet references

    ASSERT(LoopTransmitRunning);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // All receive processing normally happens at DPC level,
    // so we must pretend to be a DPC, so we raise IRQL.
    // (System worker threads typically run at PASSIVE_LEVEL).
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OriginalIrql);

    KeAcquireSpinLock(&LoopLock, &OldIrql);

    for (;;) {
        //
        // Get the next packet from the queue.
        //
        Packet = LoopTransmitHead;
        if (Packet == NULL)
            break;

        LoopTransmitHead = *(PNDIS_PACKET *)Packet->MacReserved;
        KeReleaseSpinLock(&LoopLock, OldIrql);

        Rcvd = TRUE;

        //
        // Prepare IPv6Packet notificaton info from NDIS packet.
        //

        InitializePacketFromNdis(&IPPacket, Packet, PC(Packet)->pc_offset);

        PktRefs = IPv6Receive(Context, &IPPacket);

        ASSERT(PktRefs == 0);

        IPv6SendComplete(Context, Packet, NDIS_STATUS_SUCCESS);

        //
        // Give other threads a chance to run.
        //
        KeLowerIrql(OriginalIrql);
        KeRaiseIrql(DISPATCH_LEVEL, &OriginalIrql);

        KeAcquireSpinLock(&LoopLock, &OldIrql);
    }

    LoopTransmitRunning = 0;
    KeReleaseSpinLock(&LoopLock, OldIrql);

    if (Rcvd)
        IPv6ReceiveComplete();

    KeLowerIrql(OriginalIrql);
}


//* LoopQueueTransmit - Queue a packet on loopback interface.
//
//  This is the routine called when we need to transmit a packet to ourselves.
//  We put the packet on our loopback queue, and schedule an event to deal
//  with it.  All the real work is done in LoopTransmit.
//
void
LoopQueueTransmit(
    void *Context,        // Pointer to loopback IF.
    PNDIS_PACKET Packet,  // Pointer to packet to be transmitted.
    uint Offset,          // Offset from start of packet to IP header.
    void *LinkAddress)    // (bogus) Link-level address.
{
    PNDIS_PACKET *PacketPtr;
    KIRQL OldIrql;

    //
    // Save the offset for LoopTransmit to use.
    //
    PC(Packet)->pc_offset = Offset;

    PacketPtr = (PNDIS_PACKET *)Packet->MacReserved;
    *PacketPtr = (PNDIS_PACKET)NULL;

    KeAcquireSpinLock(&LoopLock, &OldIrql);

    //
    // Add the packet to the end of the transmit queue.
    //
    if (LoopTransmitHead == (PNDIS_PACKET)NULL) {
        // Transmit queue is empty.
        LoopTransmitHead = Packet;
    } else {
        // Transmit queue is not empty.
        PacketPtr = (PNDIS_PACKET *)LoopTransmitTail->MacReserved;
        *PacketPtr = Packet;
    }
    LoopTransmitTail = Packet;

    //
    // If LoopTransmit is not already running, schedule it.
    //
    if (!LoopTransmitRunning) {
        ExQueueWorkItem(&LoopWorkItem, CriticalWorkQueue);
        LoopTransmitRunning = 1;
    }
    KeReleaseSpinLock(&LoopLock, OldIrql);
}


#pragma BEGIN_INIT

//* LoopbackInit
//
//  This function initializes the loopback Interface and NTE.
//
NetTableEntry *
LoopbackInit(void)
{
    KIRQL Irql;
    NDIS_STATUS status;
    int rc;

    KeInitializeSpinLock(&LoopInterface.Lock);
    LoopInterface.IF = &LoopInterface;
    LoopInterface.RefCnt = 1;
    LoopInterface.LinkContext = (void *)&LoopInterface;
    LoopInterface.Transmit = LoopQueueTransmit;
    LoopInterface.TrueLinkMTU = LoopInterface.LinkMTU = LOOPBACK_MTU;
    LoopInterface.LinkAddressLength = 0;
    LoopInterface.LinkAddress = NULL;
    LoopInterface.CurHopLimit = 1;    
    // Disable Neighbor Discovery for this interface.
    LoopInterface.Flags = 0;

    NeighborCacheInit(&LoopInterface);

    LoopNTE = ExAllocatePool(NonPagedPool, sizeof(NetTableEntry));
    if (LoopNTE == NULL)
        return LoopNTE;

    LoopNTE->Address = LoopbackAddr;
    LoopNTE->Type = ADE_UNICAST;
    LoopNTE->Scope = ADE_NODE_LOCAL;
    AddNTEToInterface(&LoopInterface, LoopNTE);
    LoopNTE->RefCnt = 1;
    LoopNTE->AutoConfigured = FALSE;

    LoopNTE->DADState = DAD_STATE_PREFERRED;
    LoopNTE->DADCount = LoopNTE->DADCountLB = 0;
    LoopNTE->DADTimer = 0;
    LoopNTE->ValidLifetime = INFINITE_LIFETIME;
    LoopNTE->PreferredLifetime = INFINITE_LIFETIME;

    LoopNTE->NextOnNTL = NULL;

    //
    // Join the all-nodes group with node-local scope.
    // Note that we create an AAE instead of an MAE
    // because we don't need the MLD machinery.
    // REVIEW - This just makes the all-nodes group work.
    // What about other node-local multicast groups?
    //
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    rc = FindOrCreateAAE(&LoopInterface, &AllNodesOnNodeAddr,
                         CastFromNTE(LoopNTE));
    KeLowerIrql(Irql);
    ASSERT(rc);

    //
    // Prepare a work-queue item which we will later enqueue for a system
    // worker thread to handle.  Associate our callback routine (LoopTransmit)
    // with the work item.  We keep a single static copy of the same work item
    // and just resubmit it whenever we have packets to send and no worker
    // routine already running -- this limits us to a constant argument, but
    // the LoopInterface isn't changing.
    // 
    ExInitializeWorkItem(&LoopWorkItem, LoopTransmit, &LoopInterface);

    KeInitializeSpinLock(&LoopLock);

    //
    // Add the loopback interface to the global interface list.
    // It is the first interface on the list.
    //
    AddInterface(&LoopInterface);

    //
    // Initialize default security policy
    //
    LoopInterface.FirstSP = DefaultSP;
    LoopInterface.FirstSP->RefCount++;

    //
    // Create a permanent neighbor cache entry
    // for use in loopback routing table entries.
    //
    LoopNCE = CreatePermanentNeighbor(&LoopInterface, &LoopbackAddr, NULL);
    if (LoopNCE == NULL)
        return NULL;

#if 0
    //
    // We do not need an actual loopback route for ::1.
    // It's hardcoded into FindNextHop.
    //
    RouteTableUpdate(&LoopInterface, LoopNCE, &LoopbackAddr, 128,
                     INFINITE_LIFETIME, DEFAULT_LOOPBACK_PREF,
                     FALSE, FALSE);
#endif

    return LoopNTE;
}

#pragma END_INIT
