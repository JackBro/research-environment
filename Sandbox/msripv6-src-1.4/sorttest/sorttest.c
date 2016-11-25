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
// Test tool for destination address sorting.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <stdio.h>
#include <stdlib.h>

HANDLE Handle;

int _CRTAPI1
main(int argc, char **argv)
{
    IPV6_SORT_DEST_ADDRS *Sort;
    uint NumBytes;
    WSADATA wsaData;
    int i;

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

    NumBytes = sizeof *Sort + sizeof(TDI_ADDRESS_IP6) * (argc - 1);
    Sort = (IPV6_SORT_DEST_ADDRS *) malloc(NumBytes);
    if (Sort == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    Sort->NumAddresses = argc - 1;

    for (i = 1; i < argc; i++) {
        SOCKADDR_IN6 sin6;
        int addrlen = sizeof sin6;

        //
        // Convert the string address to a sockaddr_in6.
        //
        sin6.sin6_family = AF_INET6; // shouldn't be required but is
        if (WSAStringToAddress(argv[i], AF_INET6, NULL,
                               (struct sockaddr *)&sin6, &addrlen)
                                                        == SOCKET_ERROR) {
            printf("bad address %s\n", argv[i]);
            exit(1);
        }

        //
        // Convert the sockaddr_in6 to a TDI_ADDRESS_IP6.
        //
        Sort->Addresses[i-1].sin6_port = sin6.sin6_port;
        Sort->Addresses[i-1].sin6_flowinfo = sin6.sin6_flowinfo;
        memcpy(&Sort->Addresses[i-1].sin6_addr,
               &sin6.sin6_addr,
               sizeof(struct in6_addr));
        Sort->Addresses[i-1].sin6_scope_id = sin6.sin6_scope_id;
    }

    if (! DeviceIoControl(Handle, IOCTL_IPV6_SORT_DEST_ADDRS,
                          Sort, NumBytes,
                          Sort, NumBytes, &NumBytes,
                          NULL)) {
        printf("ioctl failed: %u\n", GetLastError());
        exit(1);
    }

    for (i = 0; i < (int)Sort->NumAddresses; i++) {
        SOCKADDR_IN6 sin6;
        char buffer[128];
        int buflen = sizeof buffer;

        //
        // Convert the TDI_ADDRESS_IP6 to a sockaddr_in6.
        //
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = Sort->Addresses[i].sin6_port;
        sin6.sin6_flowinfo = Sort->Addresses[i].sin6_flowinfo;
        memcpy(&sin6.sin6_addr,
               &Sort->Addresses[i].sin6_addr,
               sizeof(struct in6_addr));
        sin6.sin6_scope_id = Sort->Addresses[i].sin6_scope_id;

        //
        // Convert the sockaddr_in6 to a string address.
        //
        if (WSAAddressToString((struct sockaddr *) &sin6,
                               sizeof sin6,
                               NULL,       // LPWSAPROTOCOL_INFO
                               buffer,
                               &buflen) == SOCKET_ERROR)
            strcpy(buffer, "<invalid>");

        printf("%d: %s\n", i, buffer);
    }

    return 0;
}
