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
// Link-level IPv6 interface definitions.
//
// This file contains the definitions defining the interface between IPv6
// and a link layer, such as Ethernet or Token-Ring.
//
// NB: We implicitly assume that all link-level addresses that a
// particular interface will communicate with are the same length,
// although different interfaces/links may be using addresses of
// different lengths.
//


#ifndef LLIP6IF_INCLUDED
#define LLIP6IF_INCLUDED

typedef void (*SetMulticastCompletionRoutine)(NDIS_STATUS Status,
                                              void *LinkAddresses,
                                              uint NumAddrs,
                                              void *CallBackContext);

//
// Structure of information passed to IPAddInterface.
//
// lip_defmtu must be less than or equal to lip_maxmtu.
// lip_defmtu and lip_maxmtu do NOT include lip_hdrsize.
//
typedef struct LLIPBindInfo {
    void *lip_context;  // LL context handle.
    uint lip_maxmtu;    // Max MTU that the link can use.
    uint lip_defmtu;    // Default MTU for the link.
    ulong lip_flags;    // Various indicators, see below.
    uint lip_hdrsize;   // Size of link-level header.
    uint lip_addrlen;   // Length of address in bytes.
    uchar *lip_addr;    // Pointer to interface address.

    //
    // The following four link-layer functions should return quickly.
    // They may be called from DPC context or with spin-locks held.
    //
    void (*lip_token)(void *Context, void *LinkAddress, IPv6Addr *Address);
    void *(*lip_rdllopt)(void *Context, uchar *OptionData);
    void (*lip_wrllopt)(void *Context, uchar *OptionData, void *LinkAddress);
    void (*lip_mcaddr)(void *Context, IPv6Addr *Address, void *LinkAddress);

    //
    // May be called from thread or DPC context, but should not be called
    // with spin-locks held. Calls are not serialized.
    //
    void (*lip_transmit)(void *Context, PNDIS_PACKET Packet,
                         uint Offset, void *LinkAddress);

    //
    // Called only from thread context; may block or otherwise
    // take a long time. Caller is responsible for serializing calls.
    //
    NDIS_STATUS (*lip_mclist)(void *Context, void *LinkAddresses,
                              uint NumKeep, uint NumAdd, uint NumDel);
    void (*lip_close)(void *Context);
} LLIPBindInfo;

#define LIP_MCLOOP_FLAG 0x00000001  // IF can disable mulicast loopback.


//
// The link-layer code calls these IPv6 functions.
//
extern NDIS_STATUS
CreateInterface(LLIPBindInfo *BindInfo, void **Context);

extern void
DestroyInterface(void *Context);

extern void
ReleaseInterface(void *Context);

extern void
SetInterfaceLinkStatus(void *Context, int MediaConnected);

extern void *
AdjustPacketBuffer(PNDIS_PACKET Packet, uint SpaceAvail, uint SpaceNeeded);

extern void
UndoAdjustPacketBuffer(PNDIS_PACKET Packet);

#endif // LLIP6IF_INCLUDED
