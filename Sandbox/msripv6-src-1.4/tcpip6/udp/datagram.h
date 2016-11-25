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
// Common datagram processing definitions.
//


#ifndef _DATAGRAM_INCLUDED_
#define _DATAGRAM_INCLUDED_  1


//
// Structure used for maintaining DG send requests.
//
#define dsr_signature 0x20525338

typedef struct DGSendReq {
#if DBG
    ulong dsr_sig;
#endif
    Queue dsr_q;                     // Queue linkage when pending.
    IPv6Addr dsr_addr;               // Remote IP Address.
    ulong dsr_scope_id;              // Scope id of remote address (if any).
    PNDIS_BUFFER dsr_buffer;         // Buffer of data to send.
    RequestCompleteRoutine dsr_rtn;  // Completion routine.
    PVOID dsr_context;               // User context.
    ushort dsr_size;                 // Size of buffer.
    ushort dsr_port;                 // Remote port.
} DGSendReq;


//
// Structure used for maintaining DG receive requests.
//
#define drr_signature 0x20525238

typedef struct DGRcvReq {
#if DBG
    ulong drr_sig;
#endif
    Queue drr_q;                               // Queue linkage on AddrObj.
    IPv6Addr drr_addr;                         // Remote IP Addr acceptable.
    ulong drr_scope_id;                        // Acceptable scope id of addr.
    PNDIS_BUFFER drr_buffer;                   // Buffer to be filled in.
    PTDI_CONNECTION_INFORMATION drr_conninfo;  // Pointer to conn. info.
    RequestCompleteRoutine drr_rtn;            // Completion routine.
    PVOID drr_context;                         // User context.
    ushort drr_size;                           // Size of buffer.
    ushort drr_port;                           // Remote port acceptable.
} DGRcvReq;


//
// External definition of exported variables.
//
extern NDIS_HANDLE DGPacketPool, DGBufferPool;
extern KSPIN_LOCK DGSendReqLock;
extern KSPIN_LOCK DGRcvReqFreeLock;


//
// External definition of exported functions.
//
extern void DGSendComplete(PNDIS_PACKET Packet);

extern TDI_STATUS TdiSendDatagram(PTDI_REQUEST Request,
                                  PTDI_CONNECTION_INFORMATION ConnInfo,
                                  uint DataSize, uint *BytesSent,
                                  PNDIS_BUFFER Buffer);

extern TDI_STATUS TdiReceiveDatagram(PTDI_REQUEST Request,
                                     PTDI_CONNECTION_INFORMATION ConnInfo,
                                     PTDI_CONNECTION_INFORMATION ReturnInfo,
                                     uint RcvSize, uint *BytesRcvd,
                                     PNDIS_BUFFER Buffer);

extern void FreeDGRcvReq(DGRcvReq *RcvReq);
extern void FreeDGSendReq(DGSendReq *SendReq);
extern int InitDG(void);
extern void PutPendingQ(AddrObj *QueueingAO);

#endif // ifndef _DATAGRAM_INCLUDED_
