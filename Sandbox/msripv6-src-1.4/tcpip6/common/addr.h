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
// This file contains the definitions of TDI address objects and related
// constants and structures.
//


#define ao_signature 0x20204F41  // 'AO  '.

#define AO_TABLE_SIZE 16         // Address object hash table size.

#define WILDCARD_PORT 0          // 0 means assign a port.

#define MIN_USER_PORT 1025       // Minimum value for a wildcard port.
#define MAX_USER_PORT 5000       // Maximim value for a user port.
#define NUM_USER_PORTS (uint)(MaxUserPort - MIN_USER_PORT + 1)

#define NETBT_SESSION_PORT 139

typedef struct AddrObj AddrObj;

// Datagram transport-specific send function.
typedef void (*DGSendProc)(AddrObj *SrcAO, void *SendReq);

//
// Definition of the structure of an address object.  Each object represents
// a local address, and the IP portion may be a wildcard.
//
typedef struct AddrObj {
#if DBG
    ulong ao_sig;
#endif
    struct AddrObj *ao_next;           // Next address object in chain.
    KSPIN_LOCK ao_lock;                // Lock for this object.
    struct AORequest *ao_request;      // Pointer to pending request.
    Queue ao_sendq;                    // Queue of sends waiting for transmit.
    Queue ao_pendq;                    // Linkage for pending queue.
    Queue ao_rcvq;                     // Receive queue.
    IPv6Addr ao_addr;                  // IP address for this address object.
    ulong ao_scope_id;                 // Scope ID of IP address (0 if none).
    ushort ao_port;                    // Local port for this address object.
    ushort ao_flags;                   // Flags for this object.
    uchar ao_prot;                     // Protocol for this AO.
    uchar ao_index;                    // Index into table of this AO.
    uchar ao_pad[2];                   // PAD PAD.
    uint ao_listencnt;                 // Number of listening connections.
    ushort ao_usecnt;                  // Count of 'uses' on AO.
    ushort ao_inst;                    // 'Instance' number of this AO.
    uint ao_mcast_if;                  // Our multicast source interface.
    short ao_mcast_hops;               // Hop count for outbound mcast packets.
    uchar ao_mcast_loop;               // The multicast loopback state.
    uchar ao_pad2;                     // PAD.
    Queue ao_activeq;                  // Queue of active connections.
    Queue ao_idleq;                    // Queue of inactive (no TCB) connctns.
    Queue ao_listenq;                  // Queue of listening connections.
    WORK_QUEUE_ITEM ao_workitem;       // Work-queue item to use for this AO.
    PConnectEvent ao_connect;          // Connect event handle.
    PVOID ao_conncontext;              // Receive DG context.
    PDisconnectEvent ao_disconnect;    // Disconnect event routine.
    PVOID ao_disconncontext;           // Disconnect event context.
    PErrorEvent ao_error;              // Error event routine.
    PVOID ao_errcontext;               // Error event context.
    PRcvEvent ao_rcv;                  // Receive event handler.
    PVOID ao_rcvcontext;               // Receive context.
    PRcvDGEvent ao_rcvdg;              // Receive DG event handler.
    PVOID ao_rcvdgcontext;             // Receive DG context.
    PRcvEvent ao_exprcv;               // Expedited receive event handler.
    PVOID ao_exprcvcontext;            // Expedited receive context.
    struct AOMCastAddr *ao_mcastlist;  // List of active multicast addresses.
    DGSendProc ao_dgsend;              // Datagram transport send function.
    ushort ao_maxdgsize;               // maximum user datagram size.
    ushort ao_udp_cksum_cover;         // UDP-Lite checksum coverage.
} AddrObj;

#define AO_RAW_FLAG     0x0200  // AO is for a raw endpoint.
#define AO_DHCP_FLAG    0x0100  // AO is bound to real 0 address.

#define AO_VALID_FLAG   0x0080  // AddrObj is valid.
#define AO_BUSY_FLAG    0x0040  // AddrObj is busy (i.e., has it exclusive).
#define AO_OOR_FLAG     0x0020  // AddrObj is out of resources, and on
                                // either the pending or delayed queue.
#define AO_QUEUED_FLAG  0x0010  // AddrObj is on the pending queue.

//                      0x0008     Unused.
#define AO_SEND_FLAG    0x0004  // Send is pending.
#define AO_OPTIONS_FLAG 0x0002  // Option set pending.
#define AO_DELETE_FLAG  0x0001  // Delete pending.


#define AO_VALID(A) ((A)->ao_flags & AO_VALID_FLAG)
#define SET_AO_INVALID(A) (A)->ao_flags &= ~AO_VALID_FLAG

#define AO_BUSY(A) ((A)->ao_flags & AO_BUSY_FLAG)
#define SET_AO_BUSY(A) (A)->ao_flags |= AO_BUSY_FLAG
#define CLEAR_AO_BUSY(A) (A)->ao_flags &= ~AO_BUSY_FLAG

#define AO_OOR(A) ((A)->ao_flags & AO_OOR_FLAG)
#define SET_AO_OOR(A) (A)->ao_flags |= AO_OOR_FLAG
#define CLEAR_AO_OOR(A) (A)->ao_flags &= ~AO_OOR_FLAG

#define AO_QUEUED(A) ((A)->ao_flags & AO_QUEUED_FLAG)
#define SET_AO_QUEUED(A) (A)->ao_flags |= AO_QUEUED_FLAG
#define CLEAR_AO_QUEUED(A) (A)->ao_flags &= ~AO_QUEUED_FLAG

#define AO_REQUEST(A, f) ((A)->ao_flags & f##_FLAG)
#define SET_AO_REQUEST(A, f) (A)->ao_flags |= f##_FLAG
#define CLEAR_AO_REQUEST(A, f) (A)->ao_flags &= ~f##_FLAG
#define AO_PENDING(A) \
        ((A)->ao_flags & (AO_DELETE_FLAG | AO_OPTIONS_FLAG | AO_SEND_FLAG))

//
// Definition of an address object search context.  This is a data structure
// used when the address object table is to be read sequentially.
//
typedef struct AOSearchContext {
    AddrObj *asc_previous;  // Previous AO found.
    IPv6Addr asc_addr;      // IP address to be found.
    uint asc_scope_id;      // Scope id for IP address.
    ushort asc_port;        // Port to be found.
    uchar asc_prot;         // Protocol.
    uchar asc_pad;          // Pad to dword boundary.
} AOSearchContext;

//
// Definition of an AO request structure.  There structures are used only for
// queuing delete and option set requests.
//
#define aor_signature 0x20524F41

typedef struct AORequest {
#if DBG
    ulong aor_sig;
#endif
    struct AORequest *aor_next;      // Next pointer in chain.
    uint aor_id;                     // ID for the request.
    uint aor_length;                 // Length of buffer.
    void *aor_buffer;                // Buffer for this request.
    RequestCompleteRoutine aor_rtn;  // Complete routine for this request.
    PVOID aor_context;               // Request context;
} AORequest;

typedef struct AOMCastAddr {
    struct AOMCastAddr *ama_next;  // Next in list.
    IPv6Addr ama_addr;             // The address.
    uint ama_if;                   // The interface.
} AOMCastAddr;


//
// External declarations for exported functions.
//
extern AddrObj *GetAddrObj(IPv6Addr *LocalAddr, uint LocalScopeId,
                           ushort LocalPort, uchar Prot, AddrObj *PreviousAO);
extern AddrObj *GetNextAddrObj(AOSearchContext *SearchContext);
extern AddrObj *GetFirstAddrObj(IPv6Addr *LocalAddr, uint LocalScopeId,
                                ushort LocalPort, uchar Prot,
                                AOSearchContext *SearchContext);
extern TDI_STATUS TdiOpenAddress(PTDI_REQUEST Request,
                                 TRANSPORT_ADDRESS UNALIGNED *AddrList,
                                 uint Protocol, void *Reuse);
extern TDI_STATUS TdiCloseAddress(PTDI_REQUEST Request);
extern TDI_STATUS SetAddrOptions(PTDI_REQUEST Request, uint ID, uint OptLength,
                                 void *Options);
extern TDI_STATUS TdiSetEvent(PVOID Handle, int Type, PVOID Handler,
                              PVOID Context);
extern uchar GetAddress(TRANSPORT_ADDRESS UNALIGNED *AddrList,
                        IPv6Addr *Addr, ulong *ScopeId, ushort *Port);
extern int InitAddr(void);
extern void ProcessAORequests(AddrObj *RequestAO);
extern void DelayDerefAO(AddrObj *RequestAO);
extern void DerefAO(AddrObj *RequestAO);
extern void FreeAORequest(AORequest *FreedRequest);
extern uint ValidateAOContext(void *Context, uint *Valid);
extern uint ReadNextAO(void *Context, void *OutBuf);
extern void InvalidateAddrs(IPv6Addr *Addr, uint ScopeId);

extern uint MCastAddrOnAO(AddrObj *AO, IPv6Addr *Addr);
extern AOMCastAddr *FindAOMCastAddr(AddrObj *AO, IPv6Addr *Addr, uint IFNo, AOMCastAddr **PrevAMA);

#define GetBestAddrObj(addr, scope, port, prot) \
            GetAddrObj(addr, scope, port, prot, NULL)

#define REF_AO(a) (a)->ao_usecnt++

#define DELAY_DEREF_AO(a) DelayDerefAO((a))
#define DEREF_AO(a) DerefAO((a))
#define LOCKED_DELAY_DEREF_AO(a) (a)->ao_usecnt--; \
\
    if (!(a)->ao_usecnt && !AO_BUSY((a)) && AO_PENDING((a))) { \
        SET_AO_BUSY((a)); \
        ExQueueWorkItem(&(a)->ao_workitem, CriticalWorkQueue); \
    }

