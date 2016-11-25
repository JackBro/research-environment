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

#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bh.h>
#include <inilib.h>

#define FORMAT_BUFFER_SIZE 80
#define BIG_FORMAT_BUFFER_SIZE 1024

#define XCHG(x)      MAKEWORD( HIBYTE(x), LOBYTE(x) )

// Definitions for handoff sets

extern char IniFile[];

#define  IP6_SECTION_NAME  "IP6_HandoffSet"

#define  IP6_TABLE_SIZE    10

// IP6 passes information to higher-level protocols
// via InstData.

INLINE DWORD MakeInstData(DWORD Length, DWORD Info)
{
    return (Length << 16) | Info;
}

INLINE DWORD GetLengthFromInstData(DWORD InstData)
{
    return InstData >> 16;
}

INLINE DWORD GetInfoFromInstData(DWORD InstData)
{
    return InstData & 0xffff;
}
