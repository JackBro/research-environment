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
// Implementation specific definitions for Internet Protocol Version 6.
//
// Things we want visible to other kernel modules, yet aren't part of the
// official specifications (i.e. are implementation specific) go here.
// 


#ifndef IP6IMP_INCLUDED
#define IP6IMP_INCLUDED 1

#include <ip6.h>
#include <ip6exp.h>

#define IP_NET_STATUS 0
#define IP_HW_STATUS 1

//
// The actual definition of a NetTableEntryOrInterface
// can be found in ip6def.h.
//
typedef struct NetTableEntryOrInterface NetTableEntryOrInterface;

//
// The actual definition of a Security Association Linkage
// can be found in security.h.
//
typedef struct SALinkage SALinkage;

typedef struct IPv6Packet IPv6Packet;
typedef struct IPv6PacketAuxiliary IPv6PacketAuxiliary;

//
// Structure of packet data we pass around the stack.
//
struct IPv6Packet {
    IPv6Packet *Next;                   // Next entry on a list of packets.
    uint Position;                      // Current logical offset into packet.
    void *Data;                         // Current pointer into packet data.
    uint ContigSize;                    // Amount of contiguous data remaining.
    uint TotalSize;                     // Total amount of data remaining.
    NetTableEntryOrInterface *NTEorIF;  // NTE or IF we received packet on.
    PNDIS_PACKET NdisPacket;            // Original NDIS Packet (if any).
    long RefCnt;                        // References held to NdisPacket.
    IPv6PacketAuxiliary *AuxList;       // Extra memory allocated by our stack.
    uint Flags;                         // Various, see below.
    IPv6Header *IP;                     // IP header for this packet.
    uint IPPosition;                    // Offset at which IP header resides.
    IPv6Addr *SrcAddr;                  // Source/home addr for mobile IP.   
    SALinkage *SAPerformed;             // Security Associations performed.
};

// Flags for above.
#define PACKET_OURS             0x01  // We alloc'd IPv6Packet struct off heap.
#define PACKET_NOT_LINK_UNICAST 0x02  // Link-level broadcast or multicast.
#define PACKET_REASSEMBLED      0x04  // Arrived as a bunch of fragments.
#define PACKET_HOLDS_NTE_REF    0x08  // Packet holds an NTE reference.
#define PACKET_JUMBO_OPTION     0x10  // Packet has a jumbo payload option.
#define PACKET_ICMP_ERROR       0x20  // Packet is an ICMP error message.

struct IPv6PacketAuxiliary {
    IPv6PacketAuxiliary *Next;  // Next entry on packet's aux list.
    uint Position;              // Packet position corresponding to region.
    uint Length;                // Length of region in bytes.
    uchar Data[];               // Data comprising region.
};

//
// Macro for comparing IPv6 addresseses.
//
#define IP6_ADDR_EQUAL(x,y) \
        (RtlCompareMemory(x, y, sizeof(IPv6Addr)) == sizeof(IPv6Addr))


//
// The actual definition of a route cache entry
// can be found in route.h.
//
typedef struct RouteCacheEntry RouteCacheEntry;


//
// Structure of a packet context.
//
typedef struct Packet6Context {
    PNDIS_PACKET pc_link;                     // For lists of packets.
    union {
        uint pc_offset;                       // Offset where real data starts.
        uint pc_nucast;                       // Used only in lan.c.
    };
    void (*CompletionHandler)(PNDIS_PACKET);  // Called on event completion.
    void *CompletionData;                     // Data for completion handler.
    IPv6Addr DiscoveryAddress;                // Source address for ND.
} Packet6Context;


//
// The ProtocolReserved field (extra bytes after normal NDIS Packet fields)
// is structured as a Packet6Context.
//
// NB: Only packets created by IPv6 have an IPv6 Packet6Context.
// Packets that NDIS hands up to us do NOT have a Packet6Context.
//
#define PC(Packet) ((Packet6Context *)(Packet)->ProtocolReserved)


//
// Global variables exported by the IPv6 driver for use by other
// NT kernel modules.
//
extern NDIS_HANDLE IPv6PacketPool, IPv6BufferPool;


//
// Functions exported by the IPv6 driver for use by other
// NT kernel modules.
//

void
IPv6RegisterULProtocol(uchar Protocol, void *RecvHandler, void *CtrlHandler);

extern void
IPv6SendComplete(void *, PNDIS_PACKET, NDIS_STATUS);

extern int
IPv6Receive(void *, IPv6Packet *);

extern void
IPv6ReceiveComplete(void);

extern void
InitializePacketFromNdis(IPv6Packet *Packet,
                         PNDIS_PACKET NdisPacket, uint Offset);

extern void
GetDataAtPacketOffset(IPv6Packet *Packet, int Offset, void **Data,
                      uint *ContigSize);

extern uint
GetPacketPositionFromPointer(IPv6Packet *Packet, uchar *Pointer);

extern uint
PacketPullup(IPv6Packet *Packet, uint Needed);

extern void
PacketPullupCleanup(IPv6Packet *Packet);

extern void
AdjustPacketParams(IPv6Packet *Packet, uint BytesToSkip);

extern void
PositionPacketAt(IPv6Packet *Packet, uint NewPosition);

extern uint
CopyToBufferChain(PNDIS_BUFFER DstBuffer, uint DstOffset,
                  PNDIS_PACKET SrcPacket, uint SrcOffset, uchar *SrcData,
                  uint Length);

extern uint
CopyPacketToNdis(PNDIS_BUFFER DestBuf, IPv6Packet *Packet, uint Size,
                 uint DestOffset, uint RcvOffset);

extern void
CopyPacketToBuffer(uchar *DestBuf, IPv6Packet *SrcPkt, uint Size, uint Offset);

extern PNDIS_BUFFER
CopyFlatToNdis(PNDIS_BUFFER DestBuf, uchar *SrcBuf, uint Size, uint *Offset,
               uint *BytesCopied);

extern void
CopyNdisToFlat(void *DstData, PNDIS_BUFFER SrcBuffer, uint SrcOffset,
               uint Length, PNDIS_BUFFER *NextBuffer, uint *NextOffset);

extern NDIS_STATUS
IPv6AllocatePacket(uint Length, PNDIS_PACKET *pPacket, void **pMemory);

extern void
IPv6FreePacket(PNDIS_PACKET Packet);

#endif // IP6IMP_INCLUDED
