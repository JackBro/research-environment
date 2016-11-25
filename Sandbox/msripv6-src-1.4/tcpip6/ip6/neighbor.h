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
// Exported Neighbor Discovery definitions and declarations.
//


#ifndef NEIGHBOR_INCLUDED
#define NEIGHBOR_INCLUDED

extern uint
CalcReachableTime(uint BaseReachableTime);

extern void
NeighborCacheInit(Interface *IF);

extern void
NeighborCacheDestroy(Interface *IF);

extern void
NeighborCacheTimeout(Interface *IF);

extern void
NeighborCacheStale(Interface *IF);

extern void
FlushNeighborCache(Interface *IF, IPv6Addr *Addr);

extern NeighborCacheEntry *
FindOrCreateNeighbor(Interface *IF, IPv6Addr *Addr);

extern NeighborCacheEntry *
CreatePermanentNeighbor(Interface *IF, IPv6Addr *Addr, void *LinkAddress);

extern int
GetReachability(NeighborCacheEntry *NCE);

extern void
DADTimeout(NetTableEntry *NTE);

extern void
RouterSolicitSend(Interface *IF);

extern void
RouterSolicitTimeout(Interface *IF);

extern void
RouterAdvertTimeout(Interface *IF, int Force);

extern char *
RouterSolicitReceive(IPv6Packet *Packet, ICMPv6Header *ICMP);

extern char *
RouterAdvertReceive(IPv6Packet *Packet, ICMPv6Header *ICMP);

extern char *
NeighborSolicitReceive(IPv6Packet *Packet, ICMPv6Header *ICMP);

extern char *
NeighborAdvertReceive(IPv6Packet *Packet, ICMPv6Header *ICMP);

extern char *
RedirectReceive(IPv6Packet *Packet, ICMPv6Header *ICMP);

extern void
RedirectSend(
    NeighborCacheEntry *NCE,               // Neighbor getting the Redirect.
    NeighborCacheEntry *TargetNCE,         // Better first-hop to use
    IPv6Addr *Destination,                 // for this Destination address.
    NetTableEntryOrInterface *NTEorIF,     // Source of the Redirect.
    PNDIS_PACKET FwdPacket,                // Packet triggering the redirect.
    uint FwdOffset,
    uint FwdPayloadLength);

#endif  // NEIGHBOR_INCLUDED
