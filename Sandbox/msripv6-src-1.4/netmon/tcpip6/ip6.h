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
// Bloodhound parser Internet Protocol version 6
//

#pragma pack(1)
typedef struct {
    union {
        BYTE Byte[16];
        WORD Word[8];
    };
} IP6_ADDRESS;
#pragma pack()

typedef IP6_ADDRESS UNALIGNED * LPIP6_ADDRESS;

#pragma pack(1)
typedef struct {
    DWORD VersClassFlow; // 4 bits Version, 8 Traffic Class, 20 Flow Label.
    WORD PayloadLength;
    BYTE NextHeader;
    BYTE HopLimit;
    IP6_ADDRESS SrcAddr;
    IP6_ADDRESS DstAddr;
} IP6_HEADER;
#pragma pack()

typedef IP6_HEADER UNALIGNED * LPIP6_HEADER;

INLINE BYTE IP6Version(LPIP6_HEADER pIP6)
{
    return (BYTE) ((pIP6->VersClassFlow & 0xf0) >> 4);
}

INLINE BYTE IP6Class(LPIP6_HEADER pIP6)
{
    return (BYTE) (((pIP6->VersClassFlow & 0xf) << 4) |
                   ((pIP6->VersClassFlow & 0xf000) >>  12));
}

INLINE DWORD IP6FlowLabel(LPIP6_HEADER pIP6)
{
    // Treat flow-label as big-endian 20-bit number
    return (((pIP6->VersClassFlow & 0xf00) << 8) |
            ((pIP6->VersClassFlow & 0xff0000) >> 8) |
            ((pIP6->VersClassFlow & 0xff000000) >> 24));
}

INLINE DWORD IP6PayloadLength(LPIP6_HEADER pIP6)
{
    // BUGBUG Should implement jumbo-grams
    return XCHG(pIP6->PayloadLength);
}

#pragma pack(1)
typedef struct {
    IP6_ADDRESS SrcAddr;
    IP6_ADDRESS DstAddr;
    DWORD PayloadLength;
    DWORD NextHeader;
} IP6_PSEUDO_HEADER;
#pragma pack()

typedef IP6_PSEUDO_HEADER * LPIP6_PSEUDO_HEADER; // NB: It's aligned

#pragma pack(1)
typedef struct {
    BYTE NextHeader;
    BYTE Length;
} IP6_EXTENSION_HEADER;
#pragma pack()

typedef IP6_EXTENSION_HEADER UNALIGNED * LPIP6_EXTENSION_HEADER;

INLINE DWORD IP6ExtensionHeaderLength(LPIP6_EXTENSION_HEADER pExt)
{
    return (pExt->Length + 1) * 8;
}

#pragma pack(1)
typedef struct {
    IP6_EXTENSION_HEADER Ext;
    BYTE Type;
    BYTE SegmentsLeft;
} IP6_ROUTING_HEADER;
#pragma pack()

typedef IP6_ROUTING_HEADER UNALIGNED * LPIP6_ROUTING_HEADER;

#pragma pack(1)
typedef struct {
    IP6_ROUTING_HEADER Rte;
    DWORD Reserved;
    IP6_ADDRESS Address[0];
} IP6_ROUTING0_HEADER;
#pragma pack()

typedef IP6_ROUTING0_HEADER UNALIGNED * LPIP6_ROUTING0_HEADER;

#pragma pack(1)
typedef struct {
    BYTE NextHeader;
    BYTE Reserved;
    WORD FragmentOffset;
    DWORD Identifier;
} IP6_FRAGMENT_HEADER;
#pragma pack()

typedef IP6_FRAGMENT_HEADER UNALIGNED * LPIP6_FRAGMENT_HEADER;

INLINE WORD IP6FragmentOffset(LPIP6_FRAGMENT_HEADER pFrag)
{
    return XCHG(pFrag->FragmentOffset) & ~7;
}

INLINE DWORD IP6MoreFragments(LPIP6_FRAGMENT_HEADER pFrag)
{
    return pFrag->FragmentOffset & 0x100;
}

#pragma pack(1)
typedef struct {
    BYTE NextHeader;
    BYTE Length;
    WORD Reserved;
    DWORD SPI;
    DWORD SeqNum;
} IP6_AUTH_HEADER;
#pragma pack()

typedef IP6_AUTH_HEADER UNALIGNED * LPIP6_AUTH_HEADER;

INLINE DWORD IP6AuthHeaderLength(LPIP6_AUTH_HEADER pAuth)
{
    return ((pAuth->Length + 2) * 4);
}

INLINE DWORD IP6AuthSPI(LPIP6_AUTH_HEADER pAuth)
{
    return DXCHG(pAuth->SPI);
}

#pragma pack(1)
typedef struct {
    DWORD SPI;
    DWORD SeqNum;
} IP6_SECURITY_HEADER;
#pragma pack()

typedef IP6_SECURITY_HEADER UNALIGNED * LPIP6_SECURITY_HEADER;

INLINE DWORD IP6SecurityHeaderLength(LPIP6_SECURITY_HEADER pSec)
{
    return (sizeof *pSec);
}

INLINE DWORD IP6SecuritySPI(LPIP6_SECURITY_HEADER pSec)
{
    return DXCHG(pSec->SPI);
}

#pragma pack(1)
typedef struct {
    BYTE PadLen;
    BYTE NextHeader;
    BYTE AuthData[16];
} IP6_SECURITY_TRAILER;
#pragma pack()

typedef IP6_SECURITY_TRAILER UNALIGNED * LPIP6_SECURITY_TRAILER;


//
// Mobile IPv6 destination option formats.
//
#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
    BYTE    Flags;           // See mask values below.
    BYTE    PrefixLength;    // Only used for "home registration" updates.
    WORD    SeqNumber;
    DWORD   Lifetime;        // Number of seconds before binding expires.
    IP6_ADDRESS CareOfAddr;  // Only present when specified in the Flags field.
} IP6_BINDING_UPDATE_OPT; 
#pragma pack()

typedef IP6_BINDING_UPDATE_OPT UNALIGNED * LPIP6_BINDING_UPDATE_OPT;

#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
    BYTE    Status;       // Disposition of the mobile node's binding update.
    WORD    SeqNumber;
    DWORD   Lifetime;     // Granted lifetime if binding accepted.
    DWORD   Refresh;      // Interval recommended to send new binging update.
} IP6_BINDING_ACK_OPT;
#pragma pack() 

typedef IP6_BINDING_ACK_OPT UNALIGNED * LPIP6_BINDING_ACK_OPT;

#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
    IP6_ADDRESS HomeAddress;
} IP6_HOME_ADDR_OPT;
#pragma pack()

typedef IP6_HOME_ADDR_OPT UNALIGNED * LPIP6_HOME_ADDR_OPT;

#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
} IP6_BINDING_REQ_OPT;
#pragma pack()

typedef IP6_BINDING_REQ_OPT UNALIGNED * LPIP6_BINDING_REQ_OPT;

//
// Mobility sub-options
//
#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
} IP6_SUB_OPT_HDR;
#pragma pack()

typedef IP6_SUB_OPT_HDR UNALIGNED * LPIP6_SUB_OPT_HDR;

#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
    WORD    UniqueId;
} IP6_UNIQUE_ID_SUB_OPT;
#pragma pack()

typedef IP6_UNIQUE_ID_SUB_OPT UNALIGNED * LPIP6_UNIQUE_ID_SUB_OPT;

#pragma pack(1)
typedef struct {
    BYTE    Type;
    BYTE    Length;
    IP6_ADDRESS Address[0];
} IP6_HOME_AGENTS_SUB_OPT;
#pragma pack()

typedef IP6_HOME_AGENTS_SUB_OPT UNALIGNED * LPIP6_HOME_AGENTS_SUB_OPT;


typedef struct {
    BYTE    Type;
    BYTE    Length;
    WORD    Value;
} IP6_ROUTER_ALERT_OPT;

typedef IP6_ROUTER_ALERT_OPT UNALIGNED * LPIP6_ROUTER_ALERT_OPT;



//=============================================================================
//  Variables.
//=============================================================================

extern HPROTOCOL hIP6;
extern SET IP6ProtocolsSet;
extern SET IP6AbbrevProtocolsSet;
extern SET IP6FragmentFlagsSet;
extern SET IP6OptionsSet;
extern LPHANDOFFTABLE lpIP6HandoffTable;

//=============================================================================
//  Helper functions.
//=============================================================================

VOID WINAPIV IP6AttachSummary(HFRAME hFrame, LPIP6_HEADER pIP6, DWORD BytesLeft);
VOID WINAPIV IP6AttachMalformedExtension(HFRAME hFrame, BYTE NextHeader, LPBYTE pNextHeader, DWORD ExtHdrLen);
VOID WINAPIV IP6AttachIncompleteExtension(HFRAME hFrame, BYTE NextHeader, LPBYTE pNextHeader, DWORD ExtHdrLen);
VOID WINAPIV IP6AttachOptions(HFRAME hFrame, LPIP6_EXTENSION_HEADER pExt, DWORD Prop);
VOID WINAPIV IP6AttachRouting(HFRAME hFrame, LPIP6_ROUTING_HEADER pRoute);
VOID WINAPIV IP6AttachFragment(HFRAME hFrame, LPIP6_FRAGMENT_HEADER pFrag);
VOID WINAPIV IP6AttachSecurity(HFRAME hFrame, LPIP6_SECURITY_HEADER pSec);
VOID WINAPIV IP6AttachSecurityTrailer(HFRAME hFrame, LPIP6_SECURITY_TRAILER pSecTrailer);
VOID WINAPIV IP6AttachAuth(HFRAME hFrame, LPIP6_AUTH_HEADER pAuth);

VOID WINAPIV IP6AttachOptionMalformed(HFRAME hFrame, LPBYTE pOption, LPBYTE pOptionEnd);
VOID WINAPIV IP6AttachOptionUnrecognized(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionPad1(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionPadN(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionJumbo(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionHomeAddr(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionBinding(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionBindAck(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionBindRequest(HFRAME hFrame, LPBYTE pOption);
VOID WINAPIV IP6AttachOptionRouterAlter(HFRAME hFrame, LPBYTE pOption);

VOID WINAPIV IP6FormatString(LPPROPERTYINST lpPropertyInst);
VOID WINAPIV IP6FormatNamedByte(LPPROPERTYINST lpPropertyInst);
VOID WINAPIV IP6FormatAddress(LPPROPERTYINST lpPropertyInst);
LPSTR WINAPIV IP6FormatAddressWorker(LPSTR sz, LPIP6_ADDRESS pAddr);
VOID WINAPIV IP6FormatChecksum(LPPROPERTYINST lpPropertyInst);


WORD WINAPIV CalcChecksum(LPIP6_PSEUDO_HEADER pPsuHdr, LPBYTE Packet, DWORD Length, DWORD ChecksumOffset);

//=============================================================================
//  Protocol entry points.
//=============================================================================

VOID   WINAPI IP6Register(HPROTOCOL);
VOID   WINAPI IP6Deregister(HPROTOCOL);
LPBYTE WINAPI IP6RecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI IP6AttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI IP6FormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);

//
// Protocol Definitions (from RFC 1060)
//

#define PROTO_HOPOPTS           0    // IPv6 - Hop-by-Hop Options Header
#define PROTO_ICMP              1    // Internet Control Messages
#define PROTO_IGMP              2    // Internet Group Management
#define PROTO_GGP               3    // Gateway to Gateway
#define PROTO_ST                5    // Stream
#define PROTO_TCP               6    // Transmission Control
#define PROTO_UCL               7    // UCL
#define PROTO_EGP               8    // Exterior Gateway Protocol
#define PROTO_IGP               9    // any private interior gateway
#define PROTO_BBNRCC            10   // BBN RCC Monitoring
#define PROTO_NVP               11   // Network Voice Protocol
#define PROTO_PUP               12   // PUP
#define PROTO_ARGUS             13   // ARGUS
#define PROTO_EMCON             14   // EMCON
#define PROTO_XNET              15   // Cross Net Debugger
#define PROTO_CHAOS             16   // Chaos
#define PROTO_USRDGRAM          17   // User Datagram
#define PROTO_MUX               18   // Multiplexing
#define PROTO_DCN               19   // DCN Measurement Subsystems
#define PROTO_HMP               20   // Host Monitoring
#define PROTO_PRM               21   // Packet Radio Measurement
#define PROTO_XNS_IDP           22   // Xerox NS IDP
#define PROTO_TRUNK_1           23   // Trunk-1
#define PROTO_TRUNK_2           24   // Trunk-2
#define PROTO_LEAF_1            25   // Leaf-1
#define PROTO_LEAF_2            26   // Leaf-2
#define PROTO_RDP               27   // Reliable Data Protocol
#define PROTO_IRTP              28   // Internet Reliable Transaction
#define PROTO_ISO_TP4           29   // ISO Transport Protocol Class 4
#define PROTO_NETBLT            30   // Bulk Data Transfer Protocol
#define PROTO_MFE_NSP           31   // MFE Network Services Protocol
#define PROTO_MERIT_INP         32   // MERIT Internodal Protocol
#define PROTO_SEP               33   // Sequential Exchange Protocol
#define PROTO_3PC               34   // Third Party Connect Protocol
#define PROTO_ROUTING           43   // IPv6 Routing Header
#define PROTO_FRAGMENT          44   // IPv6 Fragment Header
#define PROTO_GRE               47   // Generic Routing Encapsulation
#define PROTO_ESP               50   // Encapsulating Security Payload Header
#define PROTO_AUTH              51   // Authentication Header
#define PROTO_ICMP6             58   // ICMPv6
#define PROTO_NONE              59   // IPv6 No Next Header
#define PROTO_DSTOPTS           60   // IPv6 Destination Options Header
#define PROTO_ANYHOST           61   // any host internal protocol
#define PROTO_CFTP              62   // CFTP
#define PROTO_LOCAL             63   // any local network
#define PROTO_SATEXPAK          64   // SATNET and Backroom EXPAK
#define PROTO_RVD               66   // MIT Remote Virtual Disk Protocol
#define PROTO_IPPC              67   // Internet Pluribus Packet Core
#define PROTO_DFILESYS          68   // any distributed file system
#define PROTO_SATMON            69   // SATNET Monitoring
#define PROTO_VISA              70   // VISA Protocol
#define PROTO_IPCV              71   // Internet Packet Core Utility
#define PROTO_BACKSAT           76   // Backroom SATNET Monitoring
#define PROTO_SUN_ND            77   // SUN ND PROTOCOL **TEMPORARY**
#define PROTO_WIDEMON           78   // WIDEBAND Monitoring
#define PROTO_WIDEPAK           79   // WIDEBAND EXPAK
#define PROTO_ISO_IP            80   // ISO Internet Protocol
#define PROTO_VMTP              81   // VMTP
#define PROTO_SECUREVMTP        82   // Secure VMTP
#define PROTO_VINES             83   // VINES
#define PROTO_TTP               84   // TTP
#define PROTO_NSFNETIGP         85   // NSFNET - IGP
#define PROTO_DGP               86   // Dissimilar Gateway Protocol
#define PROTO_TCF               87   // TCF
#define PROTO_IGRP              88   // IGRP
#define PROTO_OSPF              89   // Open Shortest Path First IGP
#define PROTO_SPRITE_RPC        90   // Sprite RPC Protocol
#define PROTO_LARP              91   // Locus Address Resolution Protocol

//
// Option Types
//
#define IP6_OPTION_PAD1         0
#define IP6_OPTION_PADN         1
#define IP6_OPTION_JUMBO        194
#define IP6_OPTION_BIND_UPDATE  198
#define IP6_OPTION_BIND_ACK     7
#define IP6_OPTION_BIND_REQUEST 8
#define IP6_OPTION_HOME_ADDR    201
#define IP6_OPTION_ROUTER_ALERT 5    // REVIEW: Tentative, waiting for IANA.

//
// Mobility Sub-options Types
//
#define IP6_SUB_OPTION_UNIQUE_ID   1
#define IP6_SUB_OPTION_HOME_AGENTS 2
