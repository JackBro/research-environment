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
// IPv6 fragmentation/reassembly definitions.
//


#ifndef FRAGMENT_H_INCLUDED
#define FRAGMENT_H_INCLUDED 1

//
// Structure used to link fragments together.
//
// The fragment data follows the shim structure in memory.
//
typedef struct PacketShim {
    struct PacketShim *Next;    // Next packet on list.
    ushort Len;
    ushort Offset;
} PacketShim;

__inline uchar *
PacketShimData(PacketShim *shim)
{
    return (uchar *)(shim + 1);
}


//
// Structure used to keep track of the fragments
// being reassembled into a single IPv6 datagram.
//
// REVIEW: Some of these fields are bigger than they need to be.
//
struct Reassembly {
    struct Reassembly *Next, *Prev;
    IPv6Header IPHdr;         // Copy of original IP header.
    NetTableEntryOrInterface *NTEorIF;  // From the offset-zero fragment.
    ulong Id;                 // Unique (along w/ addrs) datagram identifier.
    uchar *UnfragData;        // Pointer to unfragmentable data.
    ushort UnfragmentLength;  // Length of the unfragmentable part.
    ushort Timer;             // Time to expiration in ticks (see IPv6Timeout).
    uint DataLength;          // Length of the fragmentable part.
    PacketShim *ContigList;   // List of contiguous fragments (from start).
    PacketShim *ContigEnd;    // Last shim on ContigList (for quick access).
    PacketShim *GapList;      // List of non-contiguous fragments.
    uint Flags;               // Packet flags.
    uint Size;                // Amount of memory consumed in this reassembly.
    ushort Marker;            // The current marker for contiguous data.
    uchar NextHeader;         // Header type following the fragment header.
};

//
// There are denial-of-service issues with reassembly.
// We limit the total amount of memory in the reassembly list.
// If we get fragments that cause us to exceed the limit,
// we remove old reassemblies.
//
extern struct ReassemblyList {
    KSPIN_LOCK Lock;            // Protects Reassembly List.
    Reassembly *First;          // List of packets being reassembled.
    Reassembly *Last;
    uint Size;                  // Total size of the waiting fragments.
} ReassemblyList;

#define SentinelReassembly      ((Reassembly *)&ReassemblyList.First)

//
// With the below overhead sizes, this limit allows
// up to three 64K packets to be in reassembly simultaneously.
//
#define MAX_REASSEMBLY_SIZE    (256 * 1024)

//
// Per-packet and per-fragment overhead sizes.
// These are in addition to the actual size of the buffered data.
// They should be at least as large as the Reassembly
// and PacketShim struct sizes.
//
#define REASSEMBLY_SIZE_PACKET  1024
#define REASSEMBLY_SIZE_FRAG    256

#define DEFAULT_REASSEMBLY_TIMEOUT IPv6TimerTicks(60)  // 60 seconds.


extern Reassembly *
FragmentLookup(IPv6Addr *Source, IPv6Addr *Dest, ulong Id);

extern void
AddToReassemblyList(Reassembly *Reass);

extern void
DeleteFromReassemblyList(Reassembly *Reass);

extern void
IncreaseReassemblySize(Reassembly *Reass, uint Size);

extern void
PruneReassemblyList(void);

extern void
ReassemblyTimeout(void);

extern void
ReassembleDatagram(IPv6Packet *Packet, Reassembly *Reass);

extern IPv6Packet *
CreateFragmentPacket(Reassembly *Reass);

#endif // FRAGMENT_H_INCLUDED
