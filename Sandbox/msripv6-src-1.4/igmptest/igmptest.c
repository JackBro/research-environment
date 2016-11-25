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
// Performs DeviceIoControl calls to add & drop multicast groups.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <stdio.h>
#include <stdlib.h>


HANDLE Handle;

void AddGroup(char *Group, char *Interface);
void DropGroup(char *Group, char *Interface);


void
usage(void)
{
    printf("usage: igmptest a group-address interface#\n");
    printf("       (The interface# can be zero for add.)\n");
    printf("usage: igmptest d group-address interface#\n");
    exit(1);
}

int _CRTAPI1
main(int argc, char **argv)
{
    WSADATA wsaData;

    //
    // We initialize Winsock only to have access
    // to WSAStringToAddress and WSAAddressToString.
    //
    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("WSAStartup failed\n");
        exit(1);
    }

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,      // access mode
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        printf("Could not access IPv6 protocol stack\n");
        exit(1);
    }

    switch(argv[1][0]) {
    case 'a':
    case 'A':
        if (argc < 4) 
            usage();
        AddGroup(argv[2], argv[3]);
        break;
    case 'd':
    case 'D':
        if (argc < 4) 
            usage();
        DropGroup(argv[2], argv[3]);
        break;
    default:
        usage();
    }

    return 0;
}

void
AddGroup(char *Group, char *Interface)
{
    IPv6Addr GroupAddr;
    struct ipv6_mreq Query;
    uint BytesReturned;

    // convert the comand-line address to the internal format
    if (!inet6_addr(Group, &GroupAddr)) {
        printf("bad address supplied!\n");
        exit(1);
    }

    // fill in the query structure
    Query.ipv6mr_multiaddr = GroupAddr;
    Query.ipv6mr_interface = atoi(Interface);

    if (!DeviceIoControl(Handle, IOCTL_IPV6_ADD_MEMBERSHIP,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned,
                         NULL)) {
        printf("ioctl failed\n");
        exit(1);
    }
}

void
DropGroup(char *Group, char *Interface)
{
    IPv6Addr GroupAddr;
    struct ipv6_mreq Query;
    uint BytesReturned;

    // convert the comand-line address to the internal format
    if (!inet6_addr(Group, &GroupAddr)) {
        printf("bad address supplied!\n");
        exit(1);
    }

    // fill in the query structure
    Query.ipv6mr_multiaddr = GroupAddr;
    Query.ipv6mr_interface = atoi(Interface);

    if (!DeviceIoControl(Handle, IOCTL_IPV6_DROP_MEMBERSHIP,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned,
                         NULL)) {
        printf("ioctl failed\n");
        exit(1);
    }
}
