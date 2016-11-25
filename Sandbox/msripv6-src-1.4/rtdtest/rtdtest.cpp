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
// Test tool for stressing route table lookup and the route cache.
// This doesn't stress scale, it stresses validity.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <winsock2.h>
// Need ntddip6 before ws2ip6 to get CopyTDIFromSA6 and CopySAFromTDI6.
#include <ntddip6.h>
#include <ws2ip6.h>

extern "C" int
InitIPv6Library(void);

extern "C" void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context);

extern "C" int
ControlInterface(IPV6_CONTROL_INTERFACE *Control);

extern "C" int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route);

extern "C" void
ForEachAddress(IPV6_QUERY_INTERFACE *IF,
               void (*func)(IPV6_INFO_ADDRESS *, void *), void *Context);

#define PAUSE 100 // Milliseconds.

struct PerInterface {
    uint Index;
    IPv6Addr OurAddress;
    IPv6Addr Neighbor;
    int OnLinkRouteExists;
    int ReachableRouteExists;
    int UnreachableRouteExists;
    int Forwarding;
};

extern uint
CountInterfaces(void);

extern void
InitInterfaces(PerInterface Interfaces[]);

extern uint
RandomNumber(uint Min, uint Max);

extern void
ChangeRoutes(uint NumInterfaces, PerInterface Interfaces[]);

extern void
ChangeForwarding(uint NumInterfaces, PerInterface Interfaces[]);

extern void
SendPacket(uint NumInterfaces, PerInterface Interfaces[]);

IPv6Addr Prefix;
uint PrefixLength;

int
GetNumber(char *astr, uint *number)
{
    uint num;

    num = 0;
    while (*astr != '\0') {
        if (('0' <= *astr) && (*astr <= '9'))
            num = 10 * num + (*astr - '0');
        else
            return FALSE;
        astr++;
    }

    *number = num;
    return TRUE;
}

int
GetPrefix(char *astr, IPv6Addr *prefix, uint *prefixlen)
{
    struct sockaddr_in6 sin6;
    int addrlen = sizeof sin6;
    char *slash;
    uint length;

    slash = strchr(astr, '/');
    if (slash == NULL)
        return FALSE;
    *slash = '\0';

    if (! GetNumber(slash+1, &length))
        return FALSE;
    if (length > 128)
        return FALSE;

    sin6.sin6_family = AF_INET6; // shouldn't be required but is

    if ((WSAStringToAddress(astr, AF_INET6, NULL,
                           (struct sockaddr *)&sin6, &addrlen)
                    == SOCKET_ERROR) ||
        (sin6.sin6_port != 0) ||
        (sin6.sin6_scope_id != 0))
        return FALSE;

    // The user gave us a numeric IPv6 address.

    memcpy(prefix, &sin6.sin6_addr, sizeof *prefix);
    *prefixlen = length;
    return TRUE;
}

void _CRTAPI1
main(int argc, char **argv)
{
    WSADATA WsaData;
    uint NumInterfaces;
    PerInterface *Interfaces;
    int i;

    //
    // Initialize winsock.
    //
    if (WSAStartup(MAKEWORD(2, 0), &WsaData)) {
        fprintf(stderr, "rtdtest: WSAStartup failed\n");
        exit(1);
    }

    if (!InitIPv6Library()) {
        fprintf(stderr, "rtdtest: MSR IPv6 protocol stack not installed?\n");
        exit(1);
    }

    (void) inet6_addr("3000::1", &Prefix);
    PrefixLength = 16;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") && (i+1 < argc)) {
            if (!GetPrefix(argv[i+1], &Prefix, &PrefixLength)) {
                fprintf(stderr, "rtdtest: bad prefix %s\n", argv[i+1]);
                exit(1);
            }
            i++;
        }
        else {
            fprintf(stderr, "usage: rtdtest\n");
            exit(1);
        }
    }

    NumInterfaces = CountInterfaces();
    Interfaces = new PerInterface[NumInterfaces];
    InitInterfaces(Interfaces);

    for (;;) {
        if (RandomNumber(0, 100) < 20)
            ChangeRoutes(NumInterfaces, Interfaces);

        if (RandomNumber(0, 100) < 5)
            ChangeForwarding(NumInterfaces, Interfaces);

        SendPacket(NumInterfaces, Interfaces);
        Sleep(PAUSE);
    }

    WSACleanup();
    exit(0);
}

uint RandomValue = 0;

//* Random - Generate a psuedo random value between 1 and 2^32.
//
//  This routine is a quick and dirty psuedo random number generator.
//  It has the advantages of being fast and consuming very little
//  memory (for either code or data).  The random numbers it produces are
//  not of the best quality, however.  A much better generator could be
//  had if we were willing to use an extra 256 bytes of memory for data.
//
//  This routine uses the linear congruential method (see Knuth, Vol II),
//  with specific values for the multiplier and constant taken from
//  Numerical Recipes in C Second Edition by Press, et. al.
//
uint  // Returns: A random value between 1 and 2^32.
Random(void)
{
    //
    // The algorithm is R = (aR + c) mod m, where R is the random number,
    // a is a magic multiplier, c is a constant, and the modulus m is the
    // maximum number of elements in the period.  We chose our m to be 2^32
    // in order to get the mod operation for free.
    //
    RandomValue = (1664525 * RandomValue) + 1013904223;

    return RandomValue;
}

//* RandomNumber
//
//  Returns a number randomly selected from a range [Min, Max)
//
uint
RandomNumber(uint Min, uint Max)
{
    uint Number;

    //
    // Note that the high bits of Random() are much more random
    // than the low bits.
    //
    Number = Max - Min; // Spread.
    Number = (uint)(((unsigned __int64)Random() * Number) >> 32);
    Number += Min;

    return Number;
}

void
CountInterface(IPV6_INFO_INTERFACE *IF, void *Context)
{
    uint *NumInterfaces = (uint *) Context;

    (*NumInterfaces)++;
}

uint
CountInterfaces(void)
{
    uint NumInterfaces = 0;
    ForEachInterface(CountInterface, &NumInterfaces);
    return NumInterfaces;
}

void
StoreLinkLocalAddress(IPV6_INFO_ADDRESS *ADE, void *Context)
{
    IPv6Addr *Addr = (IPv6Addr *) Context;

    if (IN6_IS_ADDR_UNSPECIFIED(Addr)) {
        //
        // No address stored yet...
        //
        if (IN6_IS_ADDR_LINKLOCAL(&ADE->Query.Address))
            *Addr = ADE->Query.Address;
    }
}

void
GetLinkLocalAddress(IPV6_QUERY_INTERFACE *IF, IPv6Addr *Addr)
{
    *Addr = in6addr_any;
    ForEachAddress(IF, StoreLinkLocalAddress, Addr);
}

void
GetNeighborAddress(IPv6Addr *SrcAddress, uint SrcScopeId,
                   IPv6Addr *Neighbor)
{
    ICMPV6_ECHO_REQUEST request;
    ICMPV6_ECHO_REPLY reply;
    HANDLE Handle;
    ulong BytesReturned;

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,          // desired access
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,       // security attributes
                         OPEN_EXISTING,
                         0,          // flags & attributes
                         NULL);      // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "rtdtest: SendICMP: CreateFile: %u\n", GetLastError());
        exit(1);
    }

    memset(&request.DstAddress, 0, sizeof request.DstAddress);
    (void) inet6_addr("ff02::1", (IPv6Addr *)&request.DstAddress.sin6_addr);
    memset(&request.SrcAddress, 0, sizeof request.SrcAddress);
    memcpy(&request.SrcAddress.sin6_addr, SrcAddress, sizeof *SrcAddress);
    request.SrcAddress.sin6_scope_id = SrcScopeId;
    request.Timeout = 1000; // 1 second
    request.TTL = 1;
    request.Flags = 0;

    if (! DeviceIoControl(Handle,
                          IOCTL_ICMPV6_ECHO_REQUEST,
                          &request, sizeof request,
                          &reply, sizeof reply,
                          &BytesReturned,
                          NULL)) {
        fprintf(stderr, "rtdtest: SendICMP: DeviceIoControl: %u\n", GetLastError());
        exit(1);
    }

    CloseHandle(Handle);

    if (reply.Status != IP_SUCCESS) {
        fprintf(stderr, "rtdtest: GetNeighborAddress: %s\n",
                GetErrorString(reply.Status));
        exit(1);
    }

    memcpy(Neighbor, &reply.Address.sin6_addr, sizeof *Neighbor);
    printf("Interface %u - using neighbor %s\n",
           SrcScopeId, inet6_ntoa(Neighbor));
}

void
InitInterface(IPV6_INFO_INTERFACE *IF, void *Context)
{
    PerInterface **pInterface = (PerInterface **) Context;
    PerInterface *Interface = (*pInterface)++;

    if (IF->Discovers) {
        Interface->Index = IF->Query.Index;
        Interface->OnLinkRouteExists = FALSE;
        Interface->ReachableRouteExists = FALSE;
        Interface->UnreachableRouteExists = FALSE;
        Interface->Forwarding = FALSE;
        GetLinkLocalAddress(&IF->Query, &Interface->OurAddress);
        GetNeighborAddress(&Interface->OurAddress, Interface->Index,
                           &Interface->Neighbor);
    }
    else {
        Interface->Index = 0;
    }
}

void
InitInterfaces(PerInterface Interfaces[])
{
    PerInterface *NextInterface = Interfaces;
    ForEachInterface(InitInterface, &NextInterface);
}

uint
PickInterface(uint NumInterfaces, PerInterface Interfaces[])
{
    uint i;

    do
        i = RandomNumber(0, NumInterfaces);
    while (Interfaces[i].Index == 0);

    return i;
}

void
ChangeRoutes(uint NumInterfaces, PerInterface Interfaces[])
{
    IPV6_INFO_ROUTE_TABLE Route;
    uint i;

    //
    // Pick an interface.
    //
    i = PickInterface(NumInterfaces, Interfaces);

    //
    // Initialize the route.
    //
    Route.Query.Prefix = Prefix;
    Route.Query.PrefixLength = PrefixLength;
    Route.Query.Neighbor.IF.Index = Interfaces[i].Index;
    // Route.Query.Neighbor.Address initialized below
    Route.SitePrefixLength = 0;
    // Route.ValidLifetime initialized below
    Route.Preference = 0;
    Route.Publish = FALSE;
    Route.Immortal = FALSE;

    //
    //
    // Pick a route to change.
    //
    switch (RandomNumber(0, 3)) {
    case 0:
        Route.Query.Neighbor.Address = in6addr_any;
        if (Interfaces[i].OnLinkRouteExists) {
            Route.ValidLifetime = 0; // Remove the route.
            Interfaces[i].OnLinkRouteExists = FALSE;
        } else {
            Route.ValidLifetime = (uint)-1; // Add the route.
            Interfaces[i].OnLinkRouteExists = TRUE;
        }
        break;

    case 1:
        Route.Query.Neighbor.Address = Interfaces[i].Neighbor;
        if (Interfaces[i].ReachableRouteExists) {
            Route.ValidLifetime = 0; // Remove the route.
            Interfaces[i].ReachableRouteExists = FALSE;
        } else {
            Route.ValidLifetime = (uint)-1; // Add the route.
            Interfaces[i].ReachableRouteExists = TRUE;
        }
        break;

    case 2:
        (void) inet6_addr("fe80::1", &Route.Query.Neighbor.Address);
        if (Interfaces[i].UnreachableRouteExists) {
            Route.ValidLifetime = 0; // Remove the route.
            Interfaces[i].UnreachableRouteExists = FALSE;
        } else {
            Route.ValidLifetime = (uint)-1; // Add the route.
            Interfaces[i].UnreachableRouteExists = TRUE;
        }
        break;
    }

    UpdateRouteTable(&Route);
}

void
ChangeForwarding(uint NumInterfaces, PerInterface Interfaces[])
{
    IPV6_CONTROL_INTERFACE Control;
    int i = PickInterface(NumInterfaces, Interfaces);

    memset(&Control, 0, sizeof Control);
    Control.Query.Index = Interfaces[i].Index;

    if (Interfaces[i].Forwarding) {
        Control.Forwards = FALSE;
        Interfaces[i].Forwarding = FALSE;
    } else {
        Control.Forwards = TRUE;
        Interfaces[i].Forwarding = TRUE;
    }

    ControlInterface(&Control);
}

void
SendUDP(struct sockaddr_in6 *src, struct sockaddr_in6 *dst)
{
    SOCKET s = socket(PF_INET6, SOCK_DGRAM, 0);
    if (s == SOCKET_ERROR) {
        fprintf(stderr, "rtdtest: SendUDP: socket: %u\n", WSAGetLastError());
        exit(1);
    }

    if (bind(s, (struct sockaddr *)src, sizeof *src) < 0) {
        fprintf(stderr, "rtdtest: SendUDP: bind: %u\n", WSAGetLastError());
        exit(1);
    }

    if (connect(s, (struct sockaddr *)dst, sizeof *dst) < 0) {
        fprintf(stderr, "rtdtest: SendUDP: connect: %u\n", WSAGetLastError());
        exit(1);
    }

    if (send(s, "hello", 5, 0) < 5) {
        if (WSAGetLastError() != WSAEADDRNOTAVAIL) {
            fprintf(stderr, "rtdtest: SendUDP: send: %u\n", WSAGetLastError());
            exit(1);
        }
    }

    closesocket(s);
}

void
SendTCP(struct sockaddr_in6 *src, struct sockaddr_in6 *dst)
{
    SOCKET s = socket(PF_INET6, SOCK_STREAM, 0);
    if (s == SOCKET_ERROR) {
        fprintf(stderr, "rtdtest: SendTCP: socket: %u\n", WSAGetLastError());
        exit(1);
    }

    ulong cmd = 1;
    if (ioctlsocket(s, FIONBIO, &cmd) < 0) {
        fprintf(stderr, "rtdtest: SendTCP: ioctl: %u\n", WSAGetLastError());
        exit(1);
    }

    if (bind(s, (struct sockaddr *)src, sizeof *src) < 0) {
        fprintf(stderr, "rtdtest: SendTCP: bind: %u\n", WSAGetLastError());
        exit(1);
    }

    if (connect(s, (struct sockaddr *)dst, sizeof *dst) < 0) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            fprintf(stderr, "rtdtest: SendTCP: connect: %u\n", WSAGetLastError());
            exit(1);
        }
    }

    closesocket(s);
}

void
SendICMP(struct sockaddr_in6 *src, struct sockaddr_in6 *dst)
{
    ICMPV6_ECHO_REQUEST request;
    ICMPV6_ECHO_REPLY reply;
    HANDLE Handle;
    ulong BytesReturned;

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,          // desired access
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,       // security attributes
                         OPEN_EXISTING,
                         0,          // flags & attributes
                         NULL);      // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "rtdtest: SendICMP: CreateFile: %u\n", GetLastError());
        exit(1);
    }

    CopyTDIFromSA6(&request.DstAddress, dst);
    CopyTDIFromSA6(&request.SrcAddress, src);
    request.Timeout = 0;
    request.TTL = 0;
    request.Flags = 0;

    if (! DeviceIoControl(Handle,
                          IOCTL_ICMPV6_ECHO_REQUEST,
                          &request, sizeof request,
                          &reply, sizeof reply,
                          &BytesReturned,
                          NULL)) {
        fprintf(stderr, "rtdtest: SendICMP: DeviceIoControl: %u\n", GetLastError());
        exit(1);
    }

    CloseHandle(Handle);
}

void
SendPacket(uint NumInterfaces, PerInterface Interfaces[])
{
    struct sockaddr_in6 src, dst;

    memset(&src, 0, sizeof src);
    memset(&dst, 0, sizeof dst);

    //
    // Pick a destination address.
    //
    dst.sin6_family = AF_INET6;
    dst.sin6_port = htons(5728);
    if (RandomNumber(0, 100) < 80) {

        //
        // Destination address that matches our routes.
        //
        dst.sin6_addr = Prefix;
        dst.sin6_scope_id = 0;

    } else {

        //
        // Link-local destination address.
        //
        (void) inet6_addr("fe80::1", &dst.sin6_addr);
        if (RandomNumber(0, 100) < 80)
            dst.sin6_scope_id = 0;
        else
            dst.sin6_scope_id = Interfaces[PickInterface(NumInterfaces, Interfaces)].Index;
    }

    //
    // Pick a source address.
    //
    src.sin6_family = AF_INET6;
    if (RandomNumber(0, 100) < 80) {

        //
        // Unspecified source address.
        //

    } else {

        //
        // Specify a particular interface.
        //
        uint i = PickInterface(NumInterfaces, Interfaces);
        src.sin6_addr = Interfaces[i].OurAddress;
        src.sin6_scope_id = Interfaces[i].Index;
    }

    //
    // Pick a method for sending a packet.
    //
    switch (RandomNumber(0, 3)) {
    case 0:
        SendUDP(&src, &dst);
        break;
    case 1:
        SendTCP(&src, &dst);
        break;
    case 2:
        SendICMP(&src, &dst);
        break;
    }
}
