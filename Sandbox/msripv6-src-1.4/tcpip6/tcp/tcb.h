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
// Definitions for TCP Control Block management.
//


#define TCB_TABLE_SIZE 64

#define MAX_REXMIT_CNT 5
#define MAX_CONNECT_REXMIT_CNT 3

extern uint TCPTime;

//
// REVIEW: better hash function for IPv6 addresses?
//
#ifdef OLDHASH1
#define TCB_HASH(DA,SA,DP,SP) ((uint)(*(uchar *)&(DA) + *((uchar *)&(DA) + 1) \
    + *((uchar *)&(DA) + 2) + *((uchar *)&(DA) + 3)) % TCB_TABLE_SIZE)
#endif

#ifdef OLDHASH
#define TCB_HASH(DA,SA,DP,SP) (((DA) + (SA) + (uint)(DP) + (uint)(SP)) % \
                               TCB_TABLE_SIZE)
#endif

#define ROR8(x) (uchar)(((uchar)(x) >> 1) | (uchar)(((uchar)(x) & 1) << 7))

#define TCB_HASH(DA,SA,DP,SP) (((uint)(ROR8(ROR8(ROR8(ROR8(*((uchar *)&(DP) + 1) + \
*((uchar *)&(DP))) + \
*((uchar *)&(DA) + 3)) + \
*((uchar *)&(DA) + 2)) + \
*((uchar *)&(DA) + 1)) + \
*((uchar *)&(DA)) )) % TCB_TABLE_SIZE)

extern struct TCB *FindTCB(IPv6Addr *Src, IPv6Addr *Dest,
                           uint SrcScopeId, uint DestScopeId,
                           ushort SrcPort, ushort DestPort);
extern uint InsertTCB(struct TCB *NewTCB);
extern struct TCB *AllocTCB(void);
extern void FreeTCB(struct TCB *FreedTCB);
extern uint RemoveTCB(struct TCB *RemovedTCB);

extern uint ValidateTCBContext(void *Context, uint *Valid);
extern uint ReadNextTCB(void *Context, void *OutBuf);

extern int InitTCB(void);
extern void UnloadTCB(void);
extern void CalculateMSSForTCB(struct TCB *);
extern void TCBWalk(uint (*CallRtn)(struct TCB *, void *, void *, void *),
                    void *Context1, void *Context2, void *Context3);
extern uint DeleteTCBWithSrc(struct TCB *CheckTCB, void *AddrPtr,
                             void *Unused1, void *Unused2);
extern uint SetTCBMTU(struct TCB *CheckTCB, void *DestPtr, void *SrcPtr,
                      void *MTUPtr);
extern void ReetSendNext(struct TCB *SeqTCB, SeqNum DropSeq);

extern uint TCBWalkCount;
