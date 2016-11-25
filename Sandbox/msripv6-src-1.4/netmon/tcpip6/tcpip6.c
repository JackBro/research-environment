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

#include "tcpip6.h"
#include "ip6.h"
#include "icmp6.h"

ENTRYPOINTS IP6EntryPoints =
{
    IP6Register,
    IP6Deregister,
    IP6RecognizeFrame,
    IP6AttachProperties,
    IP6FormatProperties
};

HPROTOCOL hIP6 = NULL;


ENTRYPOINTS ICMP6EntryPoints =
{
    ICMP6Register,
    ICMP6Deregister,
    ICMP6RecognizeFrame,
    ICMP6AttachProperties,
    ICMP6FormatProperties
};

HPROTOCOL hICMP6 = NULL;


DWORD Attached = 0;
char IniFile[80];


//=============================================================================
//  FUNCTION: DllMain()
//
//=============================================================================

BOOL WINAPI DllMain(HANDLE hInstance, ULONG Command, LPVOID Reserved)
{
    //=========================================================================
    //  If we are loading!
    //=========================================================================

    if ( Command == DLL_PROCESS_ATTACH )
    {
        if ( Attached++ == 0 )
        {
            if (BuildINIPath(IniFile, "tcpip6.dll") == NULL)
                return FALSE;

            hIP6 = CreateProtocol("IP6", &IP6EntryPoints, ENTRYPOINTS_SIZE);
            hICMP6 = CreateProtocol("ICMP6", &ICMP6EntryPoints, ENTRYPOINTS_SIZE);
        }
    }

    //=========================================================================
    //  If we are unloading!
    //=========================================================================

    if ( Command == DLL_PROCESS_DETACH )
    {
        if ( --Attached == 0 )
        {
            DestroyProtocol(hIP6);
            DestroyProtocol(hICMP6);
        }
    }

    return TRUE;
}
