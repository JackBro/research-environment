// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1993-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Bloodhound parser Internet Control Message Protocol version 6
//

#pragma pack(1)
typedef struct {
    BYTE Type;
    BYTE Code;
    WORD Checksum;
} ICMP6_HEADER;
#pragma pack()

typedef ICMP6_HEADER UNALIGNED * LPICMP6_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    DWORD Parameter;
} ICMP6_ERROR_HEADER;
#pragma pack()

typedef ICMP6_ERROR_HEADER UNALIGNED * LPICMP6_ERROR_HEADER;

INLINE DWORD ICMP6PacketTooBigMTU(LPICMP6_ERROR_HEADER pHdr)
{
    return DXCHG(pHdr->Parameter);
}

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    // Identifier and SequenceNumber are "opaque" data. They appear
    // to be little-endian in the NRL/NetBSD implementation.
    WORD Identifier;
    WORD SequenceNumber;
} ICMP6_ECHO_HEADER;
#pragma pack()

typedef ICMP6_ECHO_HEADER UNALIGNED * LPICMP6_ECHO_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    WORD MaximumResponseDelay;
    WORD Unused;
    IP6_ADDRESS MulticastAddress;
} ICMP6_GROUP_MEMBERSHIP_HEADER;
#pragma pack()

typedef ICMP6_GROUP_MEMBERSHIP_HEADER UNALIGNED * LPICMP6_GROUP_MEMBERSHIP_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    DWORD Reserved;
} ICMP6_ROUTER_SOLICITATION_HEADER;
#pragma pack()

typedef ICMP6_ROUTER_SOLICITATION_HEADER UNALIGNED * LPICMP6_ROUTER_SOLICITATION_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    BYTE CurHopLimit;
    BYTE Flags;
    WORD RouterLifetime;
    DWORD ReachableTime;
    DWORD RetransTimer;
} ICMP6_ROUTER_ADVERTISEMENT_HEADER;
#pragma pack()

typedef ICMP6_ROUTER_ADVERTISEMENT_HEADER UNALIGNED * LPICMP6_ROUTER_ADVERTISEMENT_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    DWORD Flags;
    IP6_ADDRESS TargetAddress;
} ICMP6_NEIGHBOR_HEADER;
#pragma pack()

typedef ICMP6_NEIGHBOR_HEADER UNALIGNED * LPICMP6_NEIGHBOR_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_HEADER ICMP6;
    DWORD Reserved;
    IP6_ADDRESS TargetAddress;
    IP6_ADDRESS DestAddress;
} ICMP6_REDIRECT_HEADER;
#pragma pack()

typedef ICMP6_REDIRECT_HEADER UNALIGNED * LPICMP6_REDIRECT_HEADER;

#pragma pack(1)
typedef struct {
    BYTE Type;
    BYTE Length;
} ICMP6_ND_OPTION_HEADER;
#pragma pack()

typedef ICMP6_ND_OPTION_HEADER UNALIGNED * LPICMP6_ND_OPTION_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_ND_OPTION_HEADER Option;
    BYTE PrefixLength;
    BYTE Flags;
    DWORD ValidLifetime;
    DWORD PreferredLifetime;
    BYTE Reserved[3];
    BYTE SitePrefixLength;
    IP6_ADDRESS Prefix;
} ICMP6_ND_OPTION_PREFIX_HEADER;
#pragma pack()

typedef ICMP6_ND_OPTION_PREFIX_HEADER UNALIGNED * LPICMP6_ND_OPTION_PREFIX_HEADER;

#pragma pack(1)
typedef struct {
    ICMP6_ND_OPTION_HEADER Option;
    WORD Reserved;
    DWORD MTU;
} ICMP6_ND_OPTION_MTU_HEADER;
#pragma pack()

typedef ICMP6_ND_OPTION_MTU_HEADER UNALIGNED * LPICMP6_ND_OPTION_MTU_HEADER;

//=============================================================================
//  Variables.
//=============================================================================

extern HPROTOCOL hICMP6;
extern SET ICMP6TypesSet;
extern SET ICMP6ErrorCodeSets[];
extern SET ICMP6NeighborFlagsSet;
extern SET ICMP6RouterAdvertisementFlagsSet;
extern SET ICMP6PrefixFlagsSet;

//=============================================================================
//  Helper functions.
//=============================================================================

VOID WINAPIV ICMP6AttachSummary(HFRAME hFrame, LPICMP6_HEADER pICMP6, DWORD BytesLeft);
VOID WINAPIV ICMP6AttachChecksum(HFRAME hFrame, LPICMP6_HEADER pICMP6, LPIP6_HEADER pIP6, DWORD InstData, DWORD BytesLeft);
VOID WINAPIV ICMP6AttachNDOption(HFRAME hFrame, LPICMP6_ND_OPTION_HEADER pOption, DWORD Level);
DWORD WINAPIV ICMP6FindPreviousFrameNumber(HFRAME hStart, LPBYTE Data, DWORD Length);

//=============================================================================
//  Protocol entry points.
//=============================================================================

VOID   WINAPI ICMP6Register(HPROTOCOL);
VOID   WINAPI ICMP6Deregister(HPROTOCOL);
LPBYTE WINAPI ICMP6RecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI ICMP6AttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI ICMP6FormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);

//
// Message type definitions (from RFC 1885)
//
#define ICMP6_DESTINATION_UNREACHABLE           1
#define ICMP6_PACKET_TOO_BIG                    2
#define ICMP6_TIME_EXCEEDED                     3
#define ICMP6_PARAMETER_PROBLEM                 4

#define ICMP6_ECHO_REQUEST                      128
#define ICMP6_ECHO_REPLY                        129
#define ICMP6_GROUP_MEMBERSHIP_QUERY            130
#define ICMP6_GROUP_MEMBERSHIP_REPORT           131
#define ICMP6_GROUP_MEMBERSHIP_REDUCTION        132

//
// Message type definitions (from RFC 1970)
//
#define ICMP6_ROUTER_SOLICITATION               133
#define ICMP6_ROUTER_ADVERTISEMENT              134
#define ICMP6_NEIGHBOR_SOLICITATION             135
#define ICMP6_NEIGHBOR_ADVERTISEMENT            136
#define ICMP6_REDIRECT                          137

//
// Neighbor discovery option types (from RFC 1970)
//
#define ICMP6_ND_OPTION_SOURCE_LL_ADDR          1
#define ICMP6_ND_OPTION_TARGET_LL_ADDR          2
#define ICMP6_ND_OPTION_PREFIX                  3
#define ICMP6_ND_OPTION_REDIRECT                4
#define ICMP6_ND_OPTION_MTU                     5
