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
// Raw Internet Protocol interface definitions.
//


#include "datagram.h"


//
// This value is used to identify the RAW transport.
// It is out of the range of valid IP protocols.
//
#define IP_PROTOCOL_RAW 255


//
// External definitions.
//
extern void RawReceive(IPv6Packet *Packet, uint Protocol);

extern void RawStatus(uchar StatusType, IP_STATUS StatusCode,
                      IPv6Addr OrigDest, IPv6Addr OrigSrc, IPv6Addr Src,
                      ulong Param, void *Data);

extern void RawSend(AddrObj *SrcAO, DGSendReq *SendReq);


