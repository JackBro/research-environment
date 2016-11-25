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
// IPv6 private definitions.
//
// This file contains all of the definitions for IPv6 that
// are not visible to outside layers.
//


#ifndef IPv6DEF_INCLUDED
#define IPv6DEF_INCLUDED 1

typedef struct NeighborCacheEntry NeighborCacheEntry;
typedef struct AddressEntry AddressEntry;
typedef struct MulticastAddressEntry MulticastAddressEntry;
typedef struct AnycastAddressEntry AnycastAddressEntry;
typedef struct NetTableEntryOrInterface NetTableEntryOrInterface;
typedef struct NetTableEntry NetTableEntry;
typedef struct Interface Interface;

typedef struct IPSecProc IPSecProc;

// REVIEW: Added so the build will work.
typedef unsigned long   IPAddr;  

//
// Override the default DDK definitions of ASSERT/ASSERTMSG.
// The default definitions use RtlAssert, which does nothing
// unless you are using a checked kernel.
//
#undef ASSERT
#undef ASSERTMSG

#if DBG
#define ASSERT(exp) \
    if (!(exp)) { \
        DbgPrint("assert failed (%s, %d): %s\n", __FILE__, __LINE__, #exp); \
        DbgBreakPoint(); \
    } else

#define ASSERTMSG(msg, exp) \
    if (!(exp)) { \
        DbgPrint("assert failed (%s, %d): %s\n", __FILE__, __LINE__, (msg)); \
        DbgBreakPoint(); \
    } else

#else
#define ASSERT(exp)
#define ASSERTMSG(msg, exp)
#endif // DBG

//
// Per-neighbor information.  We keep address translation and unreachability
// detection info for each of our neighbors that we're in communication with.
//
// A non-zero reference count prevents the NCE from being reclaimed.
// An NCE with zero references may be kept cached.
// A per-interface lock protects all NCEs for that interface.
//
// NCEs with a non-zero reference count hold a reference for their interface.
// NCEs with a zero reference count do not hold a reference.
// This means if you hold a reference for an NCE,
// you can always safely access and dereference NCE->IF.
//
// The Next/Prev fields link NCEs into a circular doubly-linked list.
// They must be first and must match the IF->FirstNCE/LastNCE fields
// to make the casting work out.
//
// The list of NCEs is kept sorted, from most-recently-used to least.
//
struct NeighborCacheEntry {           // a.k.a. NCE
    NeighborCacheEntry *Next;         // Next entry on I/F neighbor list.
    NeighborCacheEntry *Prev;         // Previous entry on I/F neighbor list.
    IPv6Addr NeighborAddress;         // Address of I/F on neighboring node.
    void *LinkAddress;                // Media address corresponding to above.
    // NB: LinkAddressLength field not needed - use IF->LinkAddressLength.
    ushort IsRouter:1,                // Is the neighbor a router?
           IsUnreachable:1,           // Does ND indicate unreachability?
           // DoRoundRobin is only meaningful if IsUnreachable is TRUE.
           DoRoundRobin:1;            // Should FindRoute do round-robin?
    ushort NDState;                   // Neighbor Discovery Protocol state.
    uint LastReachable;               // Timestamp (IPv6Timer ticks).
    ushort EventTimer;                // In IPv6Timer ticks (see IPv6Timeout).
    ushort EventCount;                // Count number of various timeouts.
    Interface *IF;                    // Interface on media with neighbor.
    NDIS_PACKET *WaitQueue;           // Queue of packets waiting on ND.
    long RefCnt;                      // Reference count - interlocked.
};

//
// The caller must already have a reference for the NCE.
// The interface need not be locked.
//
__inline void
AddRefNCE(NeighborCacheEntry *NCE)
{
    long RefCnt = InterlockedIncrement(&NCE->RefCnt);
    ASSERT(RefCnt != 1);
}

extern void
AddRefNCEInCache(NeighborCacheEntry *NCE);

extern void
ReleaseNCE(NeighborCacheEntry *NCE);

//
// Values for "NDState" above.  See RFC 1970, section 7.3.2 for details.
// Note: only state names are documented, we chose the values used here.
//
// In the INCOMPLETE state, the LinkAddress is not valid.
// In all other states, LinkAddress may be used to send packets.
// WaitQueue can only be non-NULL in the INCOMPLETE state,
// but it is always initialized to NULL in the other states.
// EventTimer is also initialized in all states,
// but EventCount may not be.
//
// The INCOMPLETE state has two flavors, dormant and active.  If
// EventTimer and EventCount are both zero, then we are not actively
// trying to solicit the link address.  If someone tries to send to
// this neighbor, then we start soliciting the link address.  If the
// solicitation fails (or if we enter the PROBE state and then fail to
// confirm reachability), then any waiting packets are discarded and
// we reset to INCOMPLETE with zero EventTimer/EventCount.  (So with
// the next use of this neighbor, we start soliciting again from scratch.)
//
// The IsUnreachable flag tracks separately whether the neighbor is
// *known* to be unreachable.  For example, a new NCE will be in in the
// INCOMPLETE state, but IsUnreachable is FALSE because we don't know
// yet whether the neighbor is reachable.  Because FindRoute uses
// IsUnreachable, code paths that change this flag must call
// InvalidateRouteCache.
//
#define ND_STATE_INCOMPLETE 0
#define ND_STATE_PROBE      1
#define ND_STATE_DELAY      2
#define ND_STATE_STALE      3
#define ND_STATE_REACHABLE  4
#define ND_STATE_PERMANENT  5


//
// There are a few places in the implementation where we need
// to pass a pointer which is either a NetTableEntry or an Interface.
// NetTableEntries and Interfaces share this structure as their
// first element.  With Interfaces, the IF field points back
// at the Interface itself.
//
struct NetTableEntryOrInterface {    // a.k.a. NTEorIF
    Interface *IF;
};

__inline int
IsNTE(NetTableEntryOrInterface *NTEorIF)
{
    return (NetTableEntryOrInterface *)NTEorIF->IF != NTEorIF;
}

__inline NetTableEntry *
CastToNTE(NetTableEntryOrInterface *NTEorIF)
{
    ASSERT(IsNTE(NTEorIF));
    return (NetTableEntry *) NTEorIF;
}

__inline NetTableEntryOrInterface *
CastFromNTE(NetTableEntry *NTE)
{
    return (NetTableEntryOrInterface *) NTE;
}

__inline int
IsIF(NetTableEntryOrInterface *NTEorIF)
{
    return (NetTableEntryOrInterface *)NTEorIF->IF == NTEorIF;
}

__inline Interface *
CastToIF(NetTableEntryOrInterface *NTEorIF)
{
    ASSERT(IsIF(NTEorIF));
    return (Interface *) NTEorIF;
}

__inline NetTableEntryOrInterface *
CastFromIF(Interface *IF)
{
    return (NetTableEntryOrInterface *) IF;
}


//
// Local address information. Each interface keeps track of the addresses
// assigned to the interface. Depending on the type of address, each
// ADE structure is the first element of a larger NTE, MAE, or AAE structure.
// The address information is protected by the interface's lock.
//
// The NTEorIF field must be first. In NTEs, it points to the interface
// and holds a reference for the interface.  In MAEs and AAEs, it can
// point to the interface or to one of the NTEs on the interface but in
// either case it does NOT hold a reference.
//
struct AddressEntry {            // a.k.a. ADE
    union {
        Interface *IF;
        NetTableEntry *NTE;
        NetTableEntryOrInterface *NTEorIF;
    };
    AddressEntry *Next;          // Linkage on chain.
    IPv6Addr Address;            // Address identifying this entry.
    ushort Type;                 // Address type (unicast, multicast, etc).
    ushort Scope;                // Address scope (link, site, global, etc).
};

//
// Values for address Type.
//
#define ADE_UNICAST   0x00
#define ADE_ANYCAST   0x01
#define ADE_MULTICAST 0x02

//
// Values for address Scope.
//
#define ADE_RESERVED   0x00
#define ADE_NODE_LOCAL 0x01
#define ADE_LINK_LOCAL 0x02
#define ADE_SITE_LOCAL 0x05
#define ADE_ORG_LOCAL  0x08
#define ADE_GLOBAL     0x0e


//
// Multicast ADEs are really MAEs.
//
// MAEs can be a separate global QueryList.
// If an MAE on an Interface has a non-zero MCastTimer value,
// then it is on the QueryList.
// An MAE can be on the QueryList with a zero MCastTimer value
// only when it is not on any interface and it just needs
// a Done message sent before it can be deleted.
//
struct MulticastAddressEntry {   // a.k.a. MAE
    AddressEntry;                // Inherit the ADE fields.
    uint MCastRefCount;          // Sockets/etc receiving from this group.
    //
    // The fields below are protected by the QueryList lock.
    //
    ushort MCastFlags:4,         // Necessary info about a group.
           MCastCount:4,         // Count of initial reports left to send.
           MCastSuppressLB:8;    // Packet count for loopback suppression.
    ushort MCastTimer;           // Ticks until a membership report is sent.
    MulticastAddressEntry *NextQL;      // For the QueryList.
};

//
// Bit values for MCastFlags.
//
#define MAE_REPORTABLE          0x01    // We should send Reports.
#define MAE_LAST_REPORTER       0x02    // We should send Done.


//
// Anycast ADEs are really AAEs.
// Currently an AAE has no additional fields.
//
struct AnycastAddressEntry {     // a.k.a. AAE
    AddressEntry;                // Inherit the ADE fields.
};


//
// Unicast ADEs are really NTEs.
// There is one NTE for each source (unicast) address
// assigned to an interface.
//
// NTEs hold a reference for their interface,
// so if you have a reference for an NTE
// you can always safely access and dereference NTE->IF.
//
struct NetTableEntry {                // a.k.a. NTE
    AddressEntry;                     // Inherit the ADE fields.
    NetTableEntry *NextOnNTL;         // Next NTE on NetTableList.
    long RefCnt;                      // Reference count - interlocked.
    uchar AutoConfigured;             // Was it auto-configured?
    uchar DADState;                   // Address configuration state.
    ushort DADCount;                  // How many DAD solicits left to send.
    ushort DADCountLB;                // Loopback prevention count.
    ushort DADTimer;                  // In IPv6Timer ticks (see IPv6Timeout).
    uint ValidLifetime;               // In IPv6Timer ticks (see IPv6Timeout).
    uint PreferredLifetime;           // In IPv6Timer ticks (see IPv6Timeout).
};

__inline void
AddRefNTE(NetTableEntry *NTE)
{
    InterlockedIncrement(&NTE->RefCnt);
}

__inline void
ReleaseNTE(NetTableEntry *NTE)
{
    InterlockedDecrement(&NTE->RefCnt);
}

//
// Values for DADState above.
//
#define DAD_STATE_INVALID    0
#define DAD_STATE_DUPLICATE  1
#define DAD_STATE_TENTATIVE  2
#define DAD_STATE_DEPRECATED 3
#define DAD_STATE_PREFERRED  4

__inline int
IsValidNTE(NetTableEntry *NTE)
{
    return ((NTE->DADState == DAD_STATE_PREFERRED) ||
            (NTE->DADState == DAD_STATE_DEPRECATED));
}


//
// We use this infinite lifetime value for prefix lifetimes,
// router lifetimes, address lifetimes, etc.
//
#define INFINITE_LIFETIME 0xffffffff


//
// Each interface keeps track of which link-layer multicast addresses
// are currently enabled for receive. The RefCnt is required because
// multiple IPv6 multicast addresses can map to a single link-layer
// multicast address.
//
typedef struct LinkLayerMulticastAddress {
    uint RefCnt;
    uchar LinkAddress[];        // The link-layer address follows in memory.
} LinkLayerMulticastAddress;

// 
// SecurityPolicy defined in security.h
//
typedef struct SecurityPolicy SecurityPolicy;

//
// Information about IPv6 interfaces.  There can be multiple NTEs for each
// interface, but there is exactly one interface per NTE.
//
struct Interface {                 // a.k.a. IF
    NetTableEntryOrInterface;      // For NTEorIF. Points to self.

    Interface *Next;               // Next interface on chain.

    long RefCnt;                   // Reference count - interlocked.

    //
    // Interface to the link layer. The functions all take
    // the LinkContext as their first argument. See comments
    // in llip6if.h.
    //
    void *LinkContext;             // Link layer context.
    void (*CreateToken)(void *Context, void *LinkAddress, IPv6Addr *Address);
    void *(*ReadLLOpt)(void *Context, uchar *OptionData);
    void (*WriteLLOpt)(void *Context, uchar *OptionData, void *LinkAddress);
    void (*ConvertMCastAddr)(void *Context,
                             IPv6Addr *Address, void *LinkAddress);
    void (*Transmit)(void *Context, PNDIS_PACKET Packet,
                     uint Offset, void *LinkAddress);
    NDIS_STATUS (*SetMCastAddrList)(void *Context, void *LinkAddresses,
                                    uint NumKeep, uint NumAdd, uint NumDel);
    void (*Close)(void *Context);

    uint Index;                    // Node unique index of this I/F.
    uint Site;                     // Node unique id for this I/F's site.
    uint Flags;                    // Changes require lock, reads don't.
    AddressEntry *ADE;             // List of ADEs on this I/F.
    NetTableEntry *LinkLocalNTE;   // Primary link-local address. BUGBUG.
    NeighborCacheEntry *FirstNCE;  // List of active neighbors on I/F.
    NeighborCacheEntry *LastNCE;   // Last NCE in the list.
    uint NCECount;                 // Number of NCEs in the list.
    uint TrueLinkMTU;              // True maximum MTU for this I/F.
    uint LinkMTU;                  // May be smaller than TrueLinkMTU.
    uint CurHopLimit;              // Default Hop Limit for unicast.
    uint BaseReachableTime;        // Base for random ReachableTime (in ms).
    uint ReachableTime;            // Reachable timeout (in IPv6Timer ticks).
    uint RetransTimer;             // NS timeout (in IPv6Timer ticks).
    uint DupAddrDetectTransmits;   // Number of solicits during DAD.
    uint RSCount;                  // Number of Router Solicits sent.
    uint RSTimer;                  // RS timeout (in IPv6Timer ticks).
    uint RACount;                  // Number of "fast" RAs left to send.
    uint RATimer;                  // RA timeout (in IPv6Timer ticks).
    uint LinkAddressLength;        // Length of I/F link-level address.
    uchar *LinkAddress;            // Pointer to link-level address.
    uint LinkHeaderSize;           // Length of link-level header.
    KSPIN_LOCK Lock;               // Interface lock.

    KMUTEX MCastLock;              // Serializes SetMCastAddrList operations.
    LinkLayerMulticastAddress *MCastAddresses;  // Current addresses.
    uint MCastAddrNum;             // Number of link-layer mcast addresses.
    uint MCastAddrNew;             // Number added since SetMCastAddrList.
    int LoopbackCapable;           // Can we disable loopback of this IF?
    SecurityPolicy *FirstSP;
    ulong SPNum;
};

#define SentinelNCE(IF) ((NeighborCacheEntry *)&(IF)->FirstNCE)

#define IF_FLAG_DISABLED                0x01    // Interface is disabled.
#define IF_FLAG_DISCOVERS               0x02    // Uses Neighbor Discovery.
#define IF_FLAG_ADVERTISES              0x04    // Sends Router Advertisements.
#define IF_FLAG_FORWARDS                0x08    // Forwards packets.
#define IF_FLAG_MCAST_SYNC              0x10    // Need sync with link-layer.
//
// The DISCONNECTED and RECONNECTED flags should not both be set.
// RECONNECTED indicates that the host interface was recently reconnected;
// it is cleared upon receiving a Router Advertisement.
//
#define IF_FLAG_MEDIA_DISCONNECTED      0x20    // Cable is unplugged.
#define IF_FLAG_MEDIA_RECONNECTED       0x40    // Cable was reconnected.

//
// This function should be used after taking the interface lock,
// to check if the interface is disabled.
//
__inline int
IsDisabledIF(Interface *IF)
{
    return IF->Flags & IF_FLAG_DISABLED;
}

//
// Called with the interface lock held.
//
__inline int
IsMCastSyncNeeded(Interface *IF)
{
    return IF->Flags & IF_FLAG_MCAST_SYNC;
}

//
// Active interfaces hold a reference to themselves.
// NTEs hold a reference to their interface.
// NCEs that have a non-zero ref count hold a reference.
// MAEs and AAEs do not hold a reference for their NTE or IF.
//

__inline void
AddRefIF(Interface *IF)
{
    InterlockedIncrement(&IF->RefCnt);
}

__inline void
ReleaseIF(Interface *IF)
{
    InterlockedDecrement(&IF->RefCnt);
}

//
// We have a periodic timer (IPv6Timer) that causes our IPv6Timeout
// routine to be called IPv6_TICKS_SECOND times per second.  Most of the
// timers and timeouts in this implementation are driven off this routine.
//
// There is a trade-off here between timer granularity/resolution
// and overhead.  The resolution should be subsecond because
// RETRANS_TIMER is only one second.
//
extern uint IPv6TickCount;

#define IPv6_TICKS_SECOND 2  // Two ticks per second.

#define IPv6_TIMEOUT (1000 / IPv6_TICKS_SECOND)  // In milliseconds.

#define IPv6TimerTicks(seconds) ((seconds) * IPv6_TICKS_SECOND)

//
// ConvertSecondsToTicks and ConvertTicksToSeconds
// both leave the value INFINITE_LIFETIME unchanged.
//

extern uint
ConvertSecondsToTicks(uint Seconds);

extern uint
ConvertTicksToSeconds(uint Ticks);

//
// ConvertMillisToTicks and ConvertTicksToMillis
// do not have an infinite value.
//

extern uint
ConvertMillisToTicks(uint Millis);

__inline uint
ConvertTicksToMillis(uint Ticks)
{
    return Ticks * IPv6_TIMEOUT;
}


//
// REVIEW: Hack to handle those few remaining places where we still need
// REVIEW: to allocate space for a link-level header before we know the
// REVIEW: outgoing inteface (and thus know how big said header will be).
// REVIEW: When these places have all been fixed, we won't need this.
//
#define MAX_LINK_HEADER_SIZE 32


//
// Various constants from the IPv6 RFCs...
//
// REVIEW: Some of these should be per link-layer type.
// REVIEW: Put them in the Interface structure?
//
#define MAX_INITIAL_RTR_ADVERT_INTERVAL IPv6TimerTicks(16)
#define MAX_INITIAL_RTR_ADVERTISEMENTS  3 // Produces 4 quick RAs.
#define MAX_FINAL_RTR_ADVERTISEMENTS    3
#define MIN_DELAY_BETWEEN_RAS           BUGBUG:Unused
#define MAX_RA_DELAY_TIME               1 // 0.5 seconds
#define MaxRtrAdvInterval               IPv6TimerTicks(600)
#define MinRtrAdvInterval               IPv6TimerTicks(200)
#define MAX_RTR_SOLICITATION_DELAY BUGBUG:Unused
#define RTR_SOLICITATION_INTERVAL  IPv6TimerTicks(4)  // 4 seconds.
#define MAX_RTR_SOLICITATIONS      3
#define MAX_MULTICAST_SOLICIT      3  // Total transmissions before giving up.
#define MAX_UNICAST_SOLICIT        3  // Total transmissions before giving up.
#define MAX_ANYCAST_DELAY_TIME     BUGBUG:Unused
#define REACHABLE_TIME             (30 * 1000)  // 30 seconds in milliseconds.
#define ICMP_MIN_ERROR_INTERVAL    500  // milliseconds.
#define RETRANS_TIMER              IPv6TimerTicks(1)  // 1 second.
#define DELAY_FIRST_PROBE_TIME     IPv6TimerTicks(5)  // 5 seconds.
#define MIN_RANDOM_FACTOR          50   // Percentage of base value.
#define MAX_RANDOM_FACTOR          150  // Percentage of base value.
#define PREFIX_LIFETIME_SAFETY     IPv6TimerTicks(2 * 60 * 60)  // 2 hours.
#define RECALC_REACHABLE_INTERVAL  IPv6TimerTicks(3 * 60 * 60)  // 3 hours.
#define PATH_MTU_RETRY_TIME        IPv6TimerTicks(10 * 60)  // 10 minutes.
#define MLD_UNSOLICITED_REPORT_INTERVAL IPv6TimerTicks(10)  // 10 seconds.
#define MLD_NUM_INITIAL_REPORTS         2

//
// Support for tagging memory allocations.
//
#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, '6vPI')

#endif // POOL_TAGGING


//
// Under NT, we use the assembly language version of the common core checksum
// routine instead of the C language version.
//
ULONG
tcpxsum(IN ULONG Checksum, IN PUCHAR Source, IN ULONG Length);

#define Cksum(Buffer, Length) ((ushort)tcpxsum(0, (PUCHAR)(Buffer), (Length)))


//
// Protocol Receive Procedures ("Next Header" handlers) have this prototype.
//
typedef uchar * ProtoRecvProc(IPv6Packet *Packet);

typedef struct StatusArg {
    IP_STATUS Status;
    unsigned long Arg;
    IPv6Header *IP;
} StatusArg;

//
// Protocol Control Receive Procedures have this prototype.
// These receive handlers are called for ICMP errors.
//
typedef uchar * ProtoControlRecvProc(IPv6Packet *Packet, StatusArg *Arg);

typedef struct ProtocolSwitch {
  ProtoRecvProc *DataReceive;
  ProtoControlRecvProc *ControlReceive;
} ProtocolSwitch;

extern ProtoRecvProc IPv6HeaderReceive;
extern ProtoRecvProc ICMPv6Receive;
extern ProtoRecvProc FragmentReceive;
extern ProtoRecvProc DestinationOptionsReceive;
extern ProtoRecvProc DoNothingReceive;
extern ProtoRecvProc RoutingReceive;
extern ProtoRecvProc EncapsulatingSecurityPayloadReceive;
extern ProtoRecvProc AuthenticationHeaderReceive;

extern ProtoControlRecvProc ICMPv6ControlReceive;
extern ProtoControlRecvProc ExtHdrControlReceive;

//
// Hop-by-Hop Options use a special receive handler.
// This is because they are processed even when a
// a packet is being forwarded instead of received.
// Note that they are only processed when immediately
// following an IPv6 header.
//
extern uchar *
HopByHopOptionsReceive(IPv6Packet *Packet);


//
// Upper Layer Status Procedures have this prototype.
//
typedef uint (*ULStatusProc)(uchar, IP_STATUS, IPv6Addr, IPv6Addr, IPv6Addr,
                             ulong, void *);


//
// The actual definition of a reassembly structure
// can be found in fragment.h.
//
typedef struct Reassembly Reassembly;


//
// Prototypes for global variables.
//
extern ULONG DefaultCurHopLimit;
extern ProtocolSwitch ProtocolSwitchTable[];
extern KSPIN_LOCK NetTableListLock;
extern NetTableEntry *NetTableList;  // Pointer to the net table.
extern KSPIN_LOCK IFListLock;
extern Interface *IFList;  // List of all interfaces on the system.
extern struct EchoControl *ICMPv6OutstandingEchos;
extern LIST_ENTRY PendingEchoList;  // def needed for initialization.
extern Interface LoopInterface;
extern NeighborCacheEntry *LoopNCE;
extern NetTableEntry *LoopNTE;
extern IPv6Addr UnspecifiedAddr;
extern IPv6Addr LoopbackAddr;
extern IPv6Addr AllNodesOnNodeAddr;
extern IPv6Addr AllNodesOnLinkAddr;
extern IPv6Addr AllRoutersOnLinkAddr;
extern IPv6Addr LinkLocalPrefix;


//
// Some handy functions for working with IPv6 addresses.
//

__inline int
IsUnspecified(IPv6Addr *Addr)
{
    return IP6_ADDR_EQUAL(Addr, &UnspecifiedAddr);
}

__inline int
IsLoopback(IPv6Addr *Addr)
{
    return IP6_ADDR_EQUAL(Addr, &LoopbackAddr);
}

__inline int
IsMulticast(IPv6Addr *Addr)
{
    return Addr->u.Byte[0] == 0xff;
}

__inline int
IsLinkLocal(IPv6Addr *Addr)
{
    return ((Addr->u.Byte[0] == 0xfe) &&
            ((Addr->u.Byte[1] & 0xc0) == 0x80));
}

__inline int
IsLinkLocalMulticast(IPv6Addr *Addr)
{
    return IsMulticast(Addr) && ((Addr->u.Byte[1] & 0xf) == ADE_LINK_LOCAL);
}

__inline int
IsNodeLocalMulticast(IPv6Addr *Addr)
{
    return IsMulticast(Addr) && ((Addr->u.Byte[1] & 0xf) == ADE_NODE_LOCAL);
}

extern int
IsSolicitedNodeMulticast(IPv6Addr *Addr);

__inline int
IsSiteLocal(IPv6Addr *Addr)
{
    return ((Addr->u.Byte[0] == 0xfe) &&
            ((Addr->u.Byte[1] & 0xc0) == 0xc0));
}

__inline int
IsSiteLocalMulticast(IPv6Addr *Addr)
{
    return IsMulticast(Addr) && ((Addr->u.Byte[1] & 0xf) == ADE_SITE_LOCAL);
}

__inline int
IsSubnetRouterAnycast(IPv6Addr *Addr)
{
    return ((Addr->u.DWord[2] == 0) &&
            (Addr->u.DWord[3] == 0));
}

__inline int
IsV4Mapped(IPv6Addr *Addr)
{
    return ((Addr->u.DWord[0] == 0) &&
            (Addr->u.DWord[1] == 0) &&
            (Addr->u.Word[4] == 0) &&
            (Addr->u.Word[5] == 0xffff));
}

__inline int
IsV4Translated(IPv6Addr *Addr)
{
    return ((Addr->u.DWord[0] == 0) &&
            (Addr->u.DWord[1] == 0) &&
            (Addr->u.Word[4] == 0xffff) &&
            (Addr->u.Word[5] == 0));
}

extern int
IsUniqueAddress(IPv6Addr *Addr);

extern int
IsV4Compatible(IPv6Addr *Addr);

extern uint
DetermineScopeId(IPv6Addr *Addr, Interface *IF);

extern void
CreateSolicitedNodeMulticastAddress(IPv6Addr *Addr, IPv6Addr *MCastAddr);

extern void
CreateV4CompatibleAddress(IPv6Addr *Addr, IPAddr V4Addr);

extern int
HasPrefix(IPv6Addr *Addr, IPv6Addr *Prefix, uint PrefixLength);

extern void
CopyPrefix(IPv6Addr *Addr, IPv6Addr *Prefix, uint PrefixLength);

extern uint
CommonPrefixLength(IPv6Addr *Addr, IPv6Addr *Addr2);

extern void
CopyPrefix(IPv6Addr *Addr, IPv6Addr *Prefix, uint PrefixLength);

//
// Function prototypes.
//

extern void
SeedRandom(uint Seed);

extern uint
Random(void);

extern uint
RandomNumber(uint Min, uint Max);

extern int
ParseNumber(WCHAR *S, uint Base, uint MaxNumber, uint *Number);

extern int
ParseV6Address(WCHAR *Sz, WCHAR **Terminator, IPv6Addr *Addr);

extern void
FormatV6AddressWorker(char *Sz, IPv6Addr *Addr);

extern char *
FormatV6Address(IPv6Addr *Addr);

extern int
ParseV4Address(WCHAR *Sz, WCHAR **Terminator, IPAddr *Addr);

extern void
FormatV4AddressWorker(char *Sz, IPAddr Addr);

extern char *
FormatV4Address(IPAddr Addr);

extern ushort
ChecksumPacket(PNDIS_PACKET Packet, uint Offset, uchar *Data, uint Length,
               IPv6Addr *Source, IPv6Addr *Dest, uchar NextHeader);

extern void
NeighborSolicitSend(NeighborCacheEntry *NCE, IPv6Addr *Source);

extern void
IPv6Send0(PNDIS_PACKET Packet, uint Offset, NeighborCacheEntry *NCE,
          IPv6Addr *DiscoveryAddress);

extern void
IPv6Send1(PNDIS_PACKET Packet, uint Offset, IPv6Header *IP,
          uint PayloadLength, RouteCacheEntry *RCE);

extern void
IPv6Send2(PNDIS_PACKET Packet, uint Offset, IPv6Header *IP,
          uint PayloadLength, RouteCacheEntry *RCE, ushort TransportProtocol,
          ushort SourcePort, ushort DestPort);

extern void
IPv6Forward(NetTableEntryOrInterface *RecvNTEorIF,
            PNDIS_PACKET Packet, uint Offset, IPv6Header *IP,
            uint PayloadLength, int Redirect, IPSecProc *IPSecToDo,
            RouteCacheEntry *RCE);

extern void
IPv6SendAbort(NetTableEntryOrInterface *NTEorIF,
              PNDIS_PACKET Packet, uint Offset,
              uchar ICMPType, uchar ICMPCode, ulong ErrorParameter,
              int MulticastOverride);

extern void
ICMPv6EchoTimeout(void);

extern void
IPULUnloadNotify(void);

extern Interface *
FindInterfaceFromIndex(uint Index);

extern uint
FindNextInterfaceIndex(Interface *IF);

extern void
IPv6Timeout(PKDPC MyDpcObject, void *Context, void *Unused1, void *Unused2);

extern uchar *
GetDataFromNdis(PNDIS_BUFFER SrcBuffer, uint SrcOffset, uint Length,
                uchar *DataBuffer);

extern IPv6Header UNALIGNED *
GetIPv6Header(PNDIS_PACKET Packet, uint Offset, IPv6Header *HdrBuffer);

extern void
AddNTEToInterface(Interface *IF, NetTableEntry *NTE);

extern void
AddInterface(Interface *IF);

extern void
UpdateLinkMTU(Interface *IF, uint MTU);

extern ushort
AddressScope(IPv6Addr *Addr);

extern NetTableEntry *
CreateNTE(Interface *IF, IPv6Addr *Address, uchar AutoConfigured,
          uint ValidLifetime, uint PreferredLifetime);

extern MulticastAddressEntry *
FindOrCreateMAE(Interface *IF, IPv6Addr *Addr, NetTableEntry *NTE);

extern MulticastAddressEntry *
FindAndReleaseMAE(Interface *IF, IPv6Addr *Addr);

extern int
FindOrCreateAAE(Interface *IF, IPv6Addr *Addr,
                NetTableEntryOrInterface *NTEorIF);

extern int
FindAndDeleteAAE(Interface *IF, IPv6Addr *Addr);

extern void
DestroyADEs(Interface *IF, NetTableEntry *NTE);

extern void
DestroyNTE(Interface *IF, NetTableEntry *NTE);

extern void
DestroyIF(Interface *IF);

extern AddressEntry **
FindADE(Interface *IF, IPv6Addr *Addr);

extern NetTableEntryOrInterface *
FindAddressOnInterface(Interface *IF, IPv6Addr *Addr, ushort *AddrType);

extern int
GetLinkLocalAddress(Interface *IF, IPv6Addr *Addr);

extern void
DeferSynchronizeMulticastAddresses(Interface *IF);

extern int
IPInit(WCHAR *RegKeyNameParam);

extern int Unloading;

extern void
IPUnload(void);

extern int
IPConfigure(WCHAR *RegKeyNameParam);

extern int
AddrConfUpdate(Interface *IF, IPv6Addr *Addr, uchar AutoConfigured,
               uint ValidLifetime, uint PreferredLifetime,
               int Authenticated, NetTableEntry **pNTE);

extern void
AddrConfReset(Interface *IF, uint MaxLifetime);

extern void
AddrConfTimeout(NetTableEntry *NTE);

extern void
NetTableTimeout(void);

extern void
NetTableCleanup(void);

extern void
InterfaceTimeout(void);

extern void
InterfaceCleanup(void);

extern NTSTATUS
ControlInterface(Interface *IF, int Advertises, int Forwards);

extern int
LanInit(WCHAR *RegKeyNameParam);

extern void
LanUnload(void);

extern NetTableEntry *
LoopbackInit(void);

extern void
ProtoTabInit(void);

extern void
ICMPv6Init(void);

extern int
IPSecInit(void);

extern int
TunnelInit(WCHAR *RegKeyNameParam);

extern void
TunnelUnload(void);

extern Interface *
TunnelGetPseudoIF(void);

extern ulong
NewFragmentId(void);

extern void
ReassemblyInit(void);

#endif // IPv6DEF_INCLUDED
