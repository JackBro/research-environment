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
// Definitions for using IPv4 as a link-layer for IPv6.
//


#ifndef TUNNEL_INCLUDED
#define TUNNEL_INCLUDED 1

//
// A few IPv4 definitions that we need.
// Including the v4 header files causes errors.
// REVIEW: can we fix this?
//

#define INADDR_ANY 0

typedef unsigned long IPAddr;  // An IP address.

__inline int
IsV4Broadcast(IPAddr Addr)
{
    //
    // BUGBUG - Can we check for subnet broadcasts?
    //
    return Addr == (IPAddr)-1;
}

__inline int
IsV4Multicast(IPAddr Addr)
{
    return (Addr & 0xf0) == 0xe0;
}

//
// IP Header format.
//
typedef struct IPHeader {
    uchar iph_verlen;    // Version and length.
    uchar iph_tos;       // Type of service.
    ushort iph_length;   // Total length of datagram.
    ushort iph_id;       // Identification.
    ushort iph_offset;   // Flags and fragment offset.
    uchar iph_ttl;       // Time to live.
    uchar iph_protocol;  // Protocol.
    ushort iph_xsum;     // Header checksum.
    IPAddr iph_src;      // Source address.
    IPAddr iph_dest;     // Destination address.
} IPHeader;

//
// Tunnel protocol constants.
//
#define TUNNEL_PROTOCOL    41
#define TUNNEL_DEVICE_NAME (DD_RAW_IP_DEVICE_NAME L"\\41")
#define TUNNEL_6OVER4_TTL  16

//
// We don't yet support Path MTU Discovery inside a tunnel,
// so we use a single MTU for the tunnel.
// Our buffer size for receiving IPv4 packets must be larger
// then that MTU, because other implementations might use
// a different value for their tunnel MTU.
//
#define TUNNEL_DEFAULT_MTU      IPv6_MINIMUM_MTU
#define TUNNEL_MAX_MTU          (64 * 1024 - sizeof(IPHeader) - 1)
#define TUNNEL_RECEIVE_BUFFER   (64 * 1024)

//
// Each tunnel interface (including the pseudo-interface)
// provides a TunnelContext to the IPv6 code as its link-level context.
//
// Each tunnel interface uses a separate v4 TDI address object
// for sending packets. This allows v4 attributes of the packets
// (like source address and TTL) to be controlled individually
// for each tunnel interface.
//
// The pseudo-tunnel interface does not control the v4 source address
// of the packets that it sends; the v4 stack is free to chose
// any v4 address. Note that this means a packet with a v4-compatible
// v6 source address might be sent using a v4 source address
// that is not derived from the v6 source address.
//
// The 6-over-4 and point-to-point virtual interfaces, however,
// do strictly control the v4 source addresses of their packets.
// As "real" interfaces, they participate in Neighbor Discovery
// and their link-level (v4) address is important.
//
// In contract to sending, the receive path uses a single address object
// for all tunnel interfaces. We use the TDI address object
// associated with the pseudo-interface; because it's bound to INADDR_ANY
// it receives all encapsulated v6 packets sent to the machine.
//

typedef struct TunnelContext {
    IPAddr SrcAddr;     // Our v4 address; the link-layer address.
    IPAddr DstAddr;     // Address of the other tunnel endpoint.
                        // (Zero for 6-over-4 tunnels.)
    Interface *IF;
    //
    // Although we only use AOFile (the pointer version of AOHandle),
    // we must keep AOHandle open so AOFile will work. AOHandle
    // is in the kernel process context, so any open/close operations
    // must be done in the kernel process context.
    //
    PFILE_OBJECT AOFile;
    HANDLE AOHandle;
} TunnelContext;

// Information we keep globally.
typedef struct TunnelGlobals {
    KSPIN_LOCK Lock;

    WCHAR *RegKeyNameParam;             // Where we get parameters.

    PDEVICE_OBJECT V4Device;            // Does not hold a reference.

    TunnelContext Pseudo;               // For automatic/configured tunnels.

    // Used by TunnelReceive/TunnelReceiveComplete.
    PIRP ReceiveIrp;                    // Protected by Tunnel.Lock.
    TDI_CONNECTION_INFORMATION ReceiveInputInfo;
    TDI_CONNECTION_INFORMATION ReceiveOutputInfo;

    // For 6over4. Protected by Tunnel.Lock.
    struct TunnelAddress *Addresses;

    // List of point-to-point tunnels. Protected by Tunnel.Lock.
    struct TunnelAddress *P2PTunnels;

    struct Interface Interface;

    HANDLE TdiAddressChangeHandle;

    PEPROCESS KernelProcess;            // For later comparisons.
} TunnelGlobals;

extern TunnelGlobals Tunnel;

//
// There is a TunnelAddress for each v4 address,
// in the Tunnel.Addresses list. This TunnelAddress
// has link-layer context for the corresponding 6-over-4 tunnel.
//
// There is also a TunnelAddress for each point-to-point tunnel.
//
typedef struct TunnelAddress {
    struct TunnelAddress *Next;

    TunnelContext Link;      // Link-layer context.

    NetTableEntry *AutoNTE;  // v4-compatible address for automatic tunnels.
} TunnelAddress;


//
// Context information that we pass to the IPv4 stack
// when transmitting.
//
typedef struct TunnelTransmitContext {
    Interface *IF;
    PNDIS_PACKET packet;
    TA_IP_ADDRESS taIPAddress;
    TDI_CONNECTION_INFORMATION tdiConnInfo;
} TunnelTransmitContext;

//
// Generic tunneling support routines,
// exported from tunnel.c.
//

extern NDIS_STATUS
TunnelCreateInterface(TunnelAddress *ta);

void
TunnelTransmit(
    void *Context,        // Pointer to tunnel interface.
    PNDIS_PACKET Packet,  // Pointer to packet to be transmitted.
    uint Offset,          // Offset from start of packet to IPv6 header.
    void *LinkAddress);   // Link-level address.

extern NTSTATUS
TunnelAddMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress);

extern NTSTATUS
TunnelDelMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress);

#endif  // TUNNEL_INCLUDED
