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
// Helper functions for dealing with the IPv6 protocol stack.
// Really these should be in a library of some kind.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LINK_LEVEL_ADDRESS_LENGTH   64

HANDLE Handle;

//
// Initialize this module.
// Returns FALSE for failure.
//
int
InitIPv6Library(void)
{
    //
    // Get a handle to the IPv6 device.
    // We will use this for ioctl operations.
    //
    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,      // access mode
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file

    return Handle != INVALID_HANDLE_VALUE;
}

void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context)
{
    IPV6_QUERY_INTERFACE Query, NextQuery;
    IPV6_INFO_INTERFACE *IF;
    uint InfoSize, BytesReturned;

    InfoSize = sizeof *IF + MAX_LINK_LEVEL_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) malloc(InfoSize);
    if (IF == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    NextQuery.Index = 0;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                             &Query, sizeof Query,
                             IF, InfoSize, &BytesReturned,
                             NULL)) {
            fprintf(stderr, "bad index %u\n", Query.Index);
            exit(1);
        }

        NextQuery = IF->Query;

        if (Query.Index != 0) {

            if (BytesReturned != sizeof *IF + IF->LinkLevelAddressLength) {
                fprintf(stderr, "inconsistent link-level address length\n");
                exit(1);
            }

            IF->Query = Query;
            (*func)(IF, Context);
        }

        if (NextQuery.Index == 0)
            break;
    }

    free(IF);
}

int
ControlInterface(IPV6_CONTROL_INTERFACE *Control)
{
    uint BytesReturned;

    return DeviceIoControl(Handle, IOCTL_IPV6_CONTROL_INTERFACE,
                           Control, sizeof *Control,
                           NULL, 0, &BytesReturned, NULL);
}

int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route)
{
    uint BytesReturned;

    return DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ROUTE_TABLE,
                           Route, sizeof *Route,
                           NULL, 0, &BytesReturned, NULL);
}

int
UpdateAddress(IPV6_UPDATE_ADDRESS *Address)
{
    uint BytesReturned;

    return DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ADDRESS,
                           Address, sizeof *Address,
                           NULL, 0, &BytesReturned, NULL);
}

void
ForEachAddress(IPV6_QUERY_INTERFACE *IF,
               void (*func)(IPV6_INFO_ADDRESS *, void *), void *Context)
{
    IPV6_QUERY_ADDRESS Query, NextQuery;
    IPV6_INFO_ADDRESS ADE;
    uint BytesReturned;

    NextQuery.IF = *IF;
    NextQuery.Address = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ADDRESS,
                             &Query, sizeof Query,
                             &ADE, sizeof ADE, &BytesReturned,
                             NULL)) {
            fprintf(stderr, "bad index %u\n", Query.IF.Index);
            exit(1);
        }

        NextQuery = ADE.Query;

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            ADE.Query = Query;
            (*func)(&ADE, Context);
        }

        if (IN6_ADDR_EQUAL(&NextQuery.Address, &in6addr_any))
            break;
    }
}
