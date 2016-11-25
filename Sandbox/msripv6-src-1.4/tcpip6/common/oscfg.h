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
// General include file for definitions we want everywhere.
//


#ifndef OSCFG_INCLUDED
#define OSCFG_INCLUDED

//
// Common types.
//
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

//
// Network byte-order is big-endian.
// NT runs in little-endian mode on all supported architectures.
//
__inline ushort
net_short(ushort x)
{
    return (((x & 0xff) << 8) | ((x & 0xff00) >> 8));
}

__inline ulong
net_long(ulong x)
{
    return (((x & 0xffL) << 24) | ((x & 0xff00L) << 8) |
            ((x & 0xff0000L) >> 8) | ((x &0xff000000L) >> 24));
}


//
// Helpfull macros.
//
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


//
// NT specific definitions.
//

#include <ntddk.h>

#define BEGIN_INIT
#define END_INIT

#include <ndis.h>

//
// NDIS 5 makes an incompatible change to NDIS packet pools.
// As a result, the NdisFreePacket macro in the NT 4 DDK
// does not work properly on NT 5.
// This work-around prevents use of the macro.
//
#undef NdisFreePacket
VOID
NdisFreePacket(
	IN	PNDIS_PACKET			Packet
	);


#if DBG
//
// Support for debug event log.
//
// The debug event log allows for "real time" logging of events
// in a circular queue kept in non-pageable memory.  Each event consists
// of an id number and a arbitrary 32 bit value.  The LogDebugEvent
// function adds a 64 bit timestamp to the event and adds it to the log.
//

// DEBUG_LOG_SIZE must be a power of 2 for wrap around to work properly.
#define DEBUG_LOG_SIZE (8 * 1024)  // Number of debug log entries.

struct DebugLogEntry {
    LARGE_INTEGER Time;  // When.
    uint Event;          // What.
    int Arg;             // How/Who/Where/Why?
};

void LogDebugEvent(uint Event, int Arg);
#else
#define LogDebugEvent(Event, Arg)
#endif // DBG


#endif // OSCFG_INCLUDED
