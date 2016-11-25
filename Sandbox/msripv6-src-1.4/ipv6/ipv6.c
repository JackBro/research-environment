// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Dump current IPv6 state information.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <stdio.h>
#include <stdlib.h>


HANDLE Handle;

int Verbose = FALSE;

void QueryInterface(int argc, char *argv[]);
void ControlInterface(int argc, char *argv[]);
void DeleteInterface(int argc, char *argv[]);
void QueryNeighborCache(int argc, char *argv[]);
void QueryRouteCache(int argc, char *argv[]);
void QueryRouteTable(int argc, char *argv[]);
void UpdateRouteTable(int argc, char *argv[]);
void UpdateAddress(int argc, char *argv[]);
void QueryBindingCache(int argc, char *argv[]);
void FlushNeighborCache(int argc, char *argv[]);
void FlushRouteCache(int argc, char *argv[]);
void QuerySitePrefixTable(int argc, char *argv[]);
void UpdateSitePrefixTable(int argc, char *argv[]);

void
usage(void)
{
    printf("usage: ipv6 if [ifindex]\n");
    printf("       ipv6 ifc ifindex [forwards] [-forwards] [advertises] [-advertises] [mtu #bytes] [site site-identifer]\n");
    printf("       ipv6 ifd ifindex\n");
    printf("       ipv6 adu ifindex/address [lifetime validlifetime[/preflifetime]] [anycast] [unicast]\n");
    printf("       ipv6 nc [ifindex [address]]\n");
    printf("       ipv6 ncf [ifindex [address]]\n");
    printf("       ipv6 rc [ifindex address]\n");
    printf("       ipv6 rcf [ifindex [address]]\n");
    printf("       ipv6 bc\n");
    printf("       ipv6 rt\n");
    printf("       ipv6 rtu prefix ifindex[/address] [lifetime L] [preference P] [publish] [age] [spl SitePrefixLength]\n");
    printf("       ipv6 spt\n");
    printf("       ipv6 spu prefix ifindex [lifetime L]\n");
    exit(1);
}

int _CRTAPI1
main(int argc, char **argv)
{
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

    //
    // Parse any global options.
    //
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v"))
            Verbose = TRUE;
        else
            break;
    }

    argc -= i;
    argv += i;

    if (argc < 1) {
        usage();
    }

    if (!strcmp(argv[0], "if")) {
        QueryInterface(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "ifc")) {
        ControlInterface(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "ifd")) {
        DeleteInterface(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "adu")) {
        UpdateAddress(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "nc")) {
        QueryNeighborCache(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "ncf")) {
        FlushNeighborCache(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "rc")) {
        QueryRouteCache(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "rcf")) {
        FlushRouteCache(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "rt")) {
        QueryRouteTable(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "rtu")) {
        UpdateRouteTable(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "spt")) {
        QuerySitePrefixTable(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "spu")) {
        UpdateSitePrefixTable(argc - 1, argv + 1);
    }
    else if (!strcmp(argv[0], "bc")) {
        QueryBindingCache(argc - 1, argv + 1);
    }
    else {
        usage();
    }

    return 0;
}

#define MAX_LINK_LEVEL_ADDRESS_LENGTH   64

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
GetLifetimes(char *astr, uint *valid, uint *preferred)
{
    char *slash;
    uint length;

    slash = strchr(astr, '/');
    if (slash == NULL) {
        if (strcmp(astr, "infinite")) {
            if (! GetNumber(astr, valid))
                return FALSE;

            if (preferred != NULL)
                *preferred = *valid;
        }

        return TRUE;
    }

    if (preferred == NULL)
        return FALSE;

    *slash = '\0';

    if (strcmp(astr, "infinite"))
        if (! GetNumber(astr, valid))
            return FALSE;

    if (strcmp(slash+1, "infinite"))
        if (! GetNumber(slash+1, preferred))
            return FALSE;

    return TRUE;
}

int
GetAddress(char *astr, IPv6Addr *address)
{
    struct sockaddr_in6 sin6;
    int addrlen = sizeof sin6;

    sin6.sin6_family = AF_INET6; // shouldn't be required but is

    if ((WSAStringToAddress(astr, AF_INET6, NULL,
                           (struct sockaddr *)&sin6, &addrlen)
                    == SOCKET_ERROR) ||
        (sin6.sin6_port != 0) ||
        (sin6.sin6_scope_id != 0))
        return FALSE;

    // The user gave us a numeric IPv6 address.

    memcpy(address, &sin6.sin6_addr, sizeof *address);
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

int
GetNeighbor(char *astr, uint *ifindex, IPv6Addr *addr)
{
    struct sockaddr_in6 sin6;
    int addrlen = sizeof sin6;
    char *slash;
    uint length;

    slash = strchr(astr, '/');
    if (slash != NULL)
        *slash = '\0';

    if (! GetNumber(astr, ifindex))
        return FALSE;

    if (slash == NULL) {
        *addr = in6addr_any;
        return TRUE;
    }

    sin6.sin6_family = AF_INET6; // shouldn't be required but is

    if ((WSAStringToAddress(slash+1, AF_INET6, NULL,
                           (struct sockaddr *)&sin6, &addrlen)
                    == SOCKET_ERROR) ||
        (sin6.sin6_port != 0) ||
        (sin6.sin6_scope_id != 0))
        return FALSE;

    // The user gave us a numeric IPv6 address.

    *addr = sin6.sin6_addr;
    return TRUE;
}

char *
FormatIPv6Address(IPv6Addr *Address)
{
    static char buffer[128];
    DWORD buflen = sizeof buffer;
    struct sockaddr_in6 sin6;

    memset(&sin6, 0, sizeof sin6);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = *Address;

    if (WSAAddressToString((struct sockaddr *) &sin6,
                           sizeof sin6,
                           NULL,       // LPWSAPROTOCOL_INFO
                           buffer,
                           &buflen) == SOCKET_ERROR)
        strcpy(buffer, "<invalid>");

    return buffer;
}

char *
FormatIPv4Address(struct in_addr *Address)
{
    static char buffer[128];
    DWORD buflen = sizeof buffer;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_addr = *Address;

    if (WSAAddressToString((struct sockaddr *) &sin,
                           sizeof sin,
                           NULL,       // LPWSAPROTOCOL_INFO
                           buffer,
                           &buflen) == SOCKET_ERROR)
        strcpy(buffer, "<invalid>");

    return buffer;
}

char *
FormatLinkLevelAddress(uint length, uchar *addr)
{
    static char buffer[128];

    switch (length) {
    case 6: {
        int i, digit;
        char *s = buffer;

        for (i = 0; i < 6; i++) {
            if (i != 0)
                *s++ = '-';

            digit = addr[i] >> 4;
            if (digit < 10)
                *s++ = digit + '0';
            else
                *s++ = digit - 10 + 'a';

            digit = addr[i] & 0xf;
            if (digit < 10)
                *s++ = digit + '0';
            else
                *s++ = digit - 10 + 'a';
        }
        *s = '\0';
        break;
    }

    case 4:
        //
        // IPv4 address (6-over-4 link)
        //
        strcpy(buffer, FormatIPv4Address((struct in_addr *)addr));
        break;

    case 0:
        //
        // Null or loop-back address
        //
        buffer[0] = '\0';
        break;

    default:
        printf("unrecognized link-level address format\n");
        exit(1);
    }

    return buffer;
}

void
ForEachAddress(IPV6_INFO_INTERFACE *IF,
               void (*func)(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *))
{
    IPV6_QUERY_ADDRESS Query, NextQuery;
    IPV6_INFO_ADDRESS ADE;
    uint BytesReturned;

    NextQuery.IF.Index = IF->Query.Index;
    NextQuery.Address = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ADDRESS,
                             &Query, sizeof Query,
                             &ADE, sizeof ADE, &BytesReturned,
                             NULL)) {
            printf("bad address %s\n", FormatIPv6Address(&Query.Address));
            exit(1);
        }

        NextQuery = ADE.Query;

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            ADE.Query = Query;
            (*func)(IF, &ADE);
        }

        if (IN6_ADDR_EQUAL(&NextQuery.Address, &in6addr_any))
            break;
    }
}

char *
FormatDADState(uint DADState)
{
    switch (DADState) {
    case 0:
        return "invalid";
    case 1:
        return "duplicate";
    case 2:
        return "tentative";
    case 3:
        return "deprecated";
    case 4:
        return "preferred";
    }
    return "<bad state>";
}

void
PrintAddress(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE)
{
    if (!Verbose) {
        //
        // Suppress spurious anycast addresses on the loopback interface.
        //
        if ((IF->Query.Index == 1) &&
            !IN6_ADDR_EQUAL(&ADE->Query.Address, &in6addr_loopback))
            return;
    }

    switch (ADE->Type) {
    case 0:
        printf("    %s address %s, ",
               FormatDADState(ADE->DADState),
               FormatIPv6Address(&ADE->Query.Address));

        if (ADE->ValidLifetime == 0xffffffff)
            printf("infinite/");
        else
            printf("%us/", ADE->ValidLifetime);

        if (ADE->PreferredLifetime == 0xffffffff)
            printf("infinite");
        else
            printf("%us", ADE->PreferredLifetime);

        if (ADE->AutoConfigured)
            printf(" (addrconf)");
        printf("\n");
        break;

    case 1:
        printf("    anycast address %s\n",
               FormatIPv6Address(&ADE->Query.Address));
        break;

    case 2:
        printf("    multicast address %s, %u refs",
               FormatIPv6Address(&ADE->Query.Address),
               ADE->MCastRefCount);
        if (!(ADE->MCastFlags & 0x01))
            printf(", not reportable");
        if (ADE->MCastFlags & 0x02)
            printf(", last reporter");
        if (ADE->MCastTimer != 0)
            printf(", %u seconds until report", ADE->MCastTimer);
        printf("\n");
        break;
    }
}

IPV6_INFO_INTERFACE *
GetInterface(uint Index)
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    uint InfoSize, BytesReturned;

    Query.Index = Index;

    InfoSize = sizeof *IF + MAX_LINK_LEVEL_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) malloc(InfoSize);
    if (IF == NULL) {
        printf("malloc failed\n");
        exit(1);
    }
    
    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                         &Query, sizeof Query,
                         IF, InfoSize, &BytesReturned,
                         NULL)) {
        printf("bad index %u\n", Query.Index);
        exit(1);
    }

    if (BytesReturned != sizeof *IF + IF->LinkLevelAddressLength) {
        printf("inconsistent link-level address length\n");
        exit(1);
    }

    IF->Query = Query;
    return IF;
}

void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *))
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
            printf("bad index %u\n", Query.Index);
            exit(1);
        }

        NextQuery = IF->Query;

        if (Query.Index != 0) {

            if (BytesReturned != sizeof *IF + IF->LinkLevelAddressLength) {
                printf("inconsistent link-level address length\n");
                exit(1);
            }

            IF->Query = Query;
            (*func)(IF);
        }

        if (NextQuery.Index == 0)
            break;
    }

    free(IF);
}

void
PrintInterface(IPV6_INFO_INTERFACE *IF)
{
    printf("Interface %u (site %u):\n", IF->Query.Index, IF->SiteIndex);

    if (IF->MediaConnected == 1)
        printf("  cable unplugged\n");
    else if (IF->MediaConnected == 2)
        printf("  cable reconnected\n");
    if (IF->Discovers)
        printf("  uses Neighbor Discovery\n");
    else
        printf("  does not use Neighbor Discovery\n");
    if (IF->Advertises)
        printf("  sends Router Advertisements\n");
    if (IF->Forwards)
        printf("  forwards packets\n");

    printf("  link-level address: %s\n",
           FormatLinkLevelAddress(IF->LinkLevelAddressLength,
                                  (uchar *)(IF + 1)));

    ForEachAddress(IF, PrintAddress);

    printf("  link MTU %u (true link MTU %u)\n",
           IF->LinkMTU, IF->TrueLinkMTU);
    printf("  current hop limit %u\n", IF->CurHopLimit);
    printf("  reachable time %ums (base %ums)\n",
           IF->ReachableTime, IF->BaseReachableTime);
    printf("  retransmission interval %ums\n", IF->RetransTimer);
    printf("  DAD transmits %u\n", IF->DupAddrDetectTransmits);
}

IPV6_INFO_NEIGHBOR_CACHE *
GetNeighborCacheEntry(IPV6_INFO_INTERFACE *IF, IPv6Addr *Address)
{
    IPV6_QUERY_NEIGHBOR_CACHE Query;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;
    uint InfoSize, BytesReturned;

    InfoSize = sizeof *NCE + MAX_LINK_LEVEL_ADDRESS_LENGTH;
    NCE = (IPV6_INFO_NEIGHBOR_CACHE *) malloc(InfoSize);
    if (NCE == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    Query.IF.Index = IF->Query.Index;
    Query.Address = *Address;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_NEIGHBOR_CACHE,
                         &Query, sizeof Query,
                         NCE, InfoSize, &BytesReturned,
                         NULL)) {
        printf("bad address %s\n", FormatIPv6Address(&Query.Address));
        exit(1);
    }

    if (BytesReturned != sizeof *NCE + NCE->LinkLevelAddressLength) {
        printf("inconsistent link-level address length\n");
        exit(1);
    }

    NCE->Query = Query;
    return NCE;
}

void
ForEachNeighborCacheEntry(IPV6_INFO_INTERFACE *IF,
                          void (*func)(IPV6_INFO_NEIGHBOR_CACHE *))
{
    IPV6_QUERY_NEIGHBOR_CACHE Query, NextQuery;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;
    uint InfoSize, BytesReturned;

    InfoSize = sizeof *NCE + MAX_LINK_LEVEL_ADDRESS_LENGTH;
    NCE = (IPV6_INFO_NEIGHBOR_CACHE *) malloc(InfoSize);
    if (NCE == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    NextQuery.IF.Index = IF->Query.Index;
    NextQuery.Address = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_NEIGHBOR_CACHE,
                             &Query, sizeof Query,
                             NCE, InfoSize, &BytesReturned,
                             NULL)) {
            printf("bad address %s\n", FormatIPv6Address(&Query.Address));
            exit(1);
        }

        NextQuery = NCE->Query;

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            if (BytesReturned != sizeof *NCE + NCE->LinkLevelAddressLength) {
                printf("inconsistent link-level address length\n");
                exit(1);
            }

            NCE->Query = Query;
            (*func)(NCE);
        }

        if (IN6_ADDR_EQUAL(&NextQuery.Address, &in6addr_any))
            break;
    }

    free(NCE);
}

void
PrintNeighborCacheEntry(IPV6_INFO_NEIGHBOR_CACHE *NCE)
{
    printf("%u: %18s", NCE->Query.IF.Index,
           FormatIPv6Address(&NCE->Query.Address));

    if (NCE->NDState != 0)
        printf(" %-17s", FormatLinkLevelAddress(NCE->LinkLevelAddressLength,
                                                (uchar *)(NCE + 1)));
    else
        printf(" %-17s", "");

    switch (NCE->NDState) {
    case 0:
        printf(" incomplete");
        break;
    case 1:
        printf(" probe");
        break;
    case 2:
        printf(" delay");
        break;
    case 3:
        printf(" stale");
        break;
    case 4:
        printf(" reachable (%ums)", NCE->ReachableTimer);
        break;
    case 5:
        printf(" permanent");
        break;
    }

    if (NCE->IsRouter)
        printf(" (router)");

    if (NCE->IsUnreachable)
        printf(" (unreachable)");

    printf("\n");
}

void
QueryInterface(int argc, char *argv[])
{
    uint Index;
    IPV6_INFO_INTERFACE *IF;

    if (argc == 0) {
        ForEachInterface(PrintInterface);
    }
    else if (argc == 1) {

        if (! GetNumber(argv[0], &Index))
            usage();

        IF = GetInterface(Index);
        PrintInterface(IF);
        free(IF);
    }
    else {
        usage();
    }
}

void
ControlInterface(int argc, char *argv[])
{
    IPV6_CONTROL_INTERFACE Control;
    uint BytesReturned;
    int i;

    Control.Advertises = -1;
    Control.Forwards = -1;
    Control.LinkMTU = 0;
    Control.SiteIndex = 0;

    if (argc < 1)
        usage();

    if (! GetNumber(argv[0], &Control.Query.Index))
        usage();

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "advertises") ||
            !strcmp(argv[i], "adv") ||
            !strcmp(argv[i], "a"))
            Control.Advertises = TRUE;
        else if (!strcmp(argv[i], "-advertises") ||
                 !strcmp(argv[i], "-adv") ||
                 !strcmp(argv[i], "-a"))
            Control.Advertises = FALSE;
        else if (!strcmp(argv[i], "forwards") ||
                 !strcmp(argv[i], "forw") ||
                 !strcmp(argv[i], "f"))
            Control.Forwards = TRUE;
        else if (!strcmp(argv[i], "-forwards") ||
                 !strcmp(argv[i], "-forw") ||
                 !strcmp(argv[i], "-f"))
            Control.Forwards = FALSE;
        else if (!strcmp(argv[i], "mtu") && (i+1 < argc)) {
            if (! GetNumber(argv[i+1], &Control.LinkMTU))
                usage();
            i++;
        }
        else if (!strcmp(argv[i], "site") && (i+1 < argc)) {
            if (! GetNumber(argv[i+1], &Control.SiteIndex))
                usage();
            i++;
        }
        else
            usage();
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_CONTROL_INTERFACE,
                         &Control, sizeof Control,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("control interface error: %x\n", GetLastError());
        exit(1);
    }
}

void
DeleteInterface(int argc, char *argv[])
{
    IPV6_QUERY_INTERFACE Query;
    uint BytesReturned;

    if (argc != 1)
        usage();

    if (! GetNumber(argv[0], &Query.Index))
        usage();

    if (!DeviceIoControl(Handle, IOCTL_IPV6_DELETE_INTERFACE,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("delete interface error: %x\n", GetLastError());
        exit(1);
    }
}

void
PrintNeighborCache(IPV6_INFO_INTERFACE *IF)
{
    ForEachNeighborCacheEntry(IF, PrintNeighborCacheEntry);
}

void
QueryNeighborCache(int argc, char *argv[])
{
    uint Index;
    IPV6_INFO_INTERFACE *IF;
    IPv6Addr Address;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;

    if (argc == 0) {
        ForEachInterface(PrintNeighborCache);
    }
    else if (argc == 1) {

        if (! GetNumber(argv[0], &Index))
            usage();

        IF = GetInterface(Index);
        PrintNeighborCache(IF);
        free(IF);
    }
    else if (argc == 2) {

        if (! GetNumber(argv[0], &Index))
            usage();

        if (! GetAddress(argv[1], &Address))
            usage();

        IF = GetInterface(Index);
        NCE = GetNeighborCacheEntry(IF, &Address);
        PrintNeighborCacheEntry(NCE);
        free(NCE);
        free(IF);
    }
    else {
        usage();
    }
}

IPV6_INFO_ROUTE_CACHE *
GetRouteCacheEntry(uint Index, IPv6Addr *Address)
{
    IPV6_QUERY_ROUTE_CACHE Query;
    IPV6_INFO_ROUTE_CACHE *RCE;
    uint BytesReturned;

    Query.IF.Index = Index;
    Query.Address = *Address;

    RCE = (IPV6_INFO_ROUTE_CACHE *) malloc(sizeof *RCE);
    if (RCE == NULL) {
        printf("malloc failed\n");
        exit(1);
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ROUTE_CACHE,
                         &Query, sizeof Query,
                         RCE, sizeof *RCE, &BytesReturned,
                         NULL)) {
        printf("bad index or address\n");
        exit(1);
    }

    RCE->Query = Query;
    return RCE;
}

void
ForEachDestination(void (*func)(IPV6_INFO_ROUTE_CACHE *))
{
    IPV6_QUERY_ROUTE_CACHE Query, NextQuery;
    IPV6_INFO_ROUTE_CACHE RCE;
    uint BytesReturned;

    NextQuery.IF.Index = 0;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ROUTE_CACHE,
                             &Query, sizeof Query,
                             &RCE, sizeof RCE, &BytesReturned,
                             NULL)) {
            printf("bad index %u\n", Query.IF.Index);
            exit(1);
        }

        NextQuery = RCE.Query;

        if (Query.IF.Index != 0) {

            RCE.Query = Query;
            (*func)(&RCE);
        }

        if (NextQuery.IF.Index == 0)
            break;
    }
}

void
PrintRouteCacheEntry(IPV6_INFO_ROUTE_CACHE *RCE)
{
    printf("%s via ", FormatIPv6Address(&RCE->Query.Address));
    printf("%u/%s", RCE->NextHopInterface,
           FormatIPv6Address(&RCE->NextHopAddress));

    if (! RCE->Valid)
        printf(" (stale)");

    switch (RCE->Type) {
    case 0:
        printf(" (permanent)");
        break;
    case 2:
        printf(" (redirect)");
        break;
    }

    if (RCE->Flags & 1)
        printf(" (interface-specific)\n");
    else
        printf("\n");

    printf("     src %u/%s\n",
           RCE->Query.IF.Index,
           FormatIPv6Address(&RCE->SourceAddress));

    printf("     PMTU %u", RCE->PathMTU);
    if (RCE->Flags & 2)
        printf(" (send fragment header)");
    if (RCE->PMTUProbeTimer != 0xffffffff)
        printf(" (%u seconds until PMTU probe)\n", RCE->PMTUProbeTimer/1000);
    else
        printf("\n");

    if ((RCE->ICMPLastError != 0) &&
        (RCE->ICMPLastError < 10*60*1000))
        printf("     %d seconds since ICMP error\n", RCE->ICMPLastError/1000);

    if ((RCE->BindingSeqNumber != 0) ||
        (RCE->BindingLifetime != 0) ||
        ! IN6_ADDR_EQUAL(&RCE->CareOfAddress, &in6addr_any))
        printf("     careof %s seq %u life %us\n",
               FormatIPv6Address(&RCE->CareOfAddress),
               RCE->BindingSeqNumber,
               RCE->BindingLifetime);
}

void
QueryRouteCache(int argc, char *argv[])
{
    uint Index;
    IPv6Addr Address;
    IPV6_INFO_ROUTE_CACHE *RCE;

    if (argc == 0) {
        ForEachDestination(PrintRouteCacheEntry);
    }
    else if (argc == 2) {

        if (! GetNumber(argv[0], &Index))
            usage();

        if (! GetAddress(argv[1], &Address))
            usage();

        RCE = GetRouteCacheEntry(Index, &Address);
        PrintRouteCacheEntry(RCE);
        free(RCE);
    }
    else {
        usage();
    }
}

void
ForEachRoute(void (*func)(IPV6_INFO_ROUTE_TABLE *, uint), uint Arg)
{
    IPV6_QUERY_ROUTE_TABLE Query, NextQuery;
    IPV6_INFO_ROUTE_TABLE RTE;
    uint BytesReturned;

    NextQuery.Neighbor.IF.Index = 0;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ROUTE_TABLE,
                             &Query, sizeof Query,
                             &RTE, sizeof RTE, &BytesReturned,
                             NULL)) {
            printf("bad index %u\n", Query.Neighbor.IF.Index);
            exit(1);
        }

        NextQuery = RTE.Query;

        if (Query.Neighbor.IF.Index != 0) {

            RTE.Query = Query;
            (*func)(&RTE, Arg);
        }

        if (NextQuery.Neighbor.IF.Index == 0)
            break;
    }
}

void
PrintRouteTableEntry(IPV6_INFO_ROUTE_TABLE *RTE, uint Arg)
{
    static IPv6Addr LinkLocalPrefix =
        { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (!Verbose) {
        //
        // Suppress loopback routes.
        //
        if (IN6_ADDR_EQUAL(&RTE->Query.Neighbor.Address, &in6addr_loopback))
            return;

        //
        // Suppress default link-local routes.
        //
        if (IN6_ADDR_EQUAL(&RTE->Query.Prefix, &LinkLocalPrefix) &&
            (RTE->Query.PrefixLength == 10) &&
            IN6_ADDR_EQUAL(&RTE->Query.Neighbor.Address, &in6addr_any) &&
            (RTE->Preference == 1) &&
            (RTE->ValidLifetime == (uint)-1) &&
            !RTE->Publish &&
            !RTE->Immortal)
            return;
    }

    printf("%s/%u -> %u",
           FormatIPv6Address(&RTE->Query.Prefix),
           RTE->Query.PrefixLength,
           RTE->Query.Neighbor.IF.Index);

    if (! IN6_ADDR_EQUAL(&RTE->Query.Neighbor.Address, &in6addr_any))
        printf("/%s", FormatIPv6Address(&RTE->Query.Neighbor.Address));

    printf(" pref %u", RTE->Preference);

    if (RTE->ValidLifetime == (uint)-1)
        printf(" (lifetime infinite");
    else
        printf(" (lifetime %us", RTE->ValidLifetime);

    if (RTE->Publish)
        printf(", publish");
    if (RTE->Immortal)
        printf(", no aging");

    if (RTE->SitePrefixLength != 0)
        printf(", spl %u", RTE->SitePrefixLength);

    printf(")\n");
}

void
QueryRouteTable(int argc, char *argv[])
{
    if (argc == 0) {
        ForEachRoute(PrintRouteTableEntry, 0);
    }
    else {
        usage();
    }
}

void
UpdateRouteTable(int argc, char *argv[])
{
    IPV6_INFO_ROUTE_TABLE Route;
    uint BytesReturned;
    int i;

    Route.SitePrefixLength = 0;
    Route.ValidLifetime = 0xffffffff;
    Route.Preference = 0;
    Route.Publish = FALSE;
    Route.Immortal = -1;

    if (argc < 2)
        usage();

    if (! GetNeighbor(argv[1],
                      &Route.Query.Neighbor.IF.Index,
                      &Route.Query.Neighbor.Address))
        usage();

    if (! GetPrefix(argv[0],
                    &Route.Query.Prefix,
                    &Route.Query.PrefixLength))
        usage();

    for (i = 2; i < argc; i++) {
        if ((!strcmp(argv[i], "lifetime") ||
             !strcmp(argv[i], "life") ||
             !strcmp(argv[i], "l")) &&
            (i+1 < argc)) {

            if (! GetLifetimes(argv[++i], &Route.ValidLifetime, NULL))
                usage();
        }
        else if ((!strcmp(argv[i], "preference") ||
                  !strcmp(argv[i], "pref") ||
                  !strcmp(argv[i], "p")) &&
                 (i+1 < argc)) {

            if (! GetNumber(argv[++i], &Route.Preference))
                usage();
        }
        else if (!strcmp(argv[i], "spl") && (i+1 < argc)) {

            if (! GetNumber(argv[++i], &Route.SitePrefixLength))
                usage();
        }
        else if (!strcmp(argv[i], "advertise") ||
                 !strcmp(argv[i], "advert") ||
                 !strcmp(argv[i], "adv") ||
                 !strcmp(argv[i], "a") ||
                 !strcmp(argv[i], "publish") ||
                 !strcmp(argv[i], "pub")) {

            Route.Publish = TRUE;
        }
        else if (!strcmp(argv[i], "immortal") ||
                 !strcmp(argv[i], "i") ||
                 !strcmp(argv[i], "noaging") ||
                 !strcmp(argv[i], "noage") ||
                 !strcmp(argv[i], "no-aging") ||
                 !strcmp(argv[i], "no-age")) {

            Route.Immortal = TRUE;
        }
        else if (!strcmp(argv[i], "aging") ||
                 !strcmp(argv[i], "age")) {

            Route.Immortal = FALSE;
        }
        else
            usage();
    }

    if (Route.Immortal == -1)
        Route.Immortal = Route.Publish;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ROUTE_TABLE,
                         &Route, sizeof Route,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("route update error: %x\n", GetLastError());
        exit(1);
    }
}

void
UpdateAddress(int argc, char *argv[])
{
    IPV6_UPDATE_ADDRESS Update;
    uint BytesReturned;
    int i;

    Update.Type = 0; // Unicast.
    Update.AutoConfigured = FALSE;
    Update.ValidLifetime = 0xffffffff;
    Update.PreferredLifetime = 0xffffffff;

    if (argc < 1)
        usage();

    if ((strchr(argv[0], '/') == NULL) ||
        ! GetNeighbor(argv[0],
                      &Update.Query.IF.Index,
                      &Update.Query.Address))
        usage();

    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "lifetime") ||
             !strcmp(argv[i], "life") ||
             !strcmp(argv[i], "l")) &&
            (i+1 < argc)) {

            if (! GetLifetimes(argv[++i],
                               &Update.ValidLifetime,
                               &Update.PreferredLifetime))
                usage();
        }
        else if (!strcmp(argv[i], "unicast"))
            Update.Type = 0; // Unicast.
        else if (!strcmp(argv[i], "anycast"))
            Update.Type = 1; // Anycast.
        else if (!strcmp(argv[i], "addrconf"))
            Update.AutoConfigured = TRUE;
        else if (!strcmp(argv[i], "manual"))
            Update.AutoConfigured = FALSE;
        else
            usage();
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ADDRESS,
                         &Update, sizeof Update,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("address update error: %x\n", GetLastError());
        exit(1);
    }
}


void
ForEachBinding(void (*func)(IPV6_INFO_BINDING_CACHE *))
{
    IPV6_QUERY_BINDING_CACHE Query, NextQuery;
    IPV6_INFO_BINDING_CACHE BCE;
    uint BytesReturned;

    NextQuery.HomeAddress = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_BINDING_CACHE,
                             &Query, sizeof Query,
                             &BCE, sizeof BCE, &BytesReturned,
                             NULL)) {
            printf("bad home address %s\n",
                   FormatIPv6Address(&Query.HomeAddress));
            exit(1);
        }

        NextQuery = BCE.Query;

        if (!IN6_ADDR_EQUAL(&Query.HomeAddress, &in6addr_any)) {
            BCE.Query = Query;
            (*func)(&BCE);
        }

        if (IN6_ADDR_EQUAL(&NextQuery.HomeAddress, &in6addr_any))
            break;
    }
}

void
PrintBindingCacheEntry(IPV6_INFO_BINDING_CACHE *BCE)
{
    printf("home: %s\n", FormatIPv6Address(&BCE->HomeAddress));
    printf(" c/o: %s\n", FormatIPv6Address(&BCE->CareOfAddress));
    printf(" seq: %u   Lifetime: %us\n\n",
           BCE->BindingSeqNumber,
           BCE->BindingLifetime);
}

void
QueryBindingCache(int argc, char *argv[])
{
    if (argc == 0) {
        ForEachBinding(PrintBindingCacheEntry);
    } else {
        usage();
    }
}

void
FlushNeighborCacheForInterface(IPV6_INFO_INTERFACE *IF)
{
    IPV6_QUERY_NEIGHBOR_CACHE Query;
    uint BytesReturned;

    Query.IF = IF->Query;
    Query.Address = in6addr_any;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("flush neighbor cache error: %x\n", GetLastError());
        exit(1);
    }
}

void
FlushNeighborCache(int argc, char *argv[])
{
    //
    // Rather than put code in the kernel ioctl to iterate
    // over the interfaces, we do it here in user space.
    //
    if (argc == 0) {
        ForEachInterface(FlushNeighborCacheForInterface);
    }
    else {
        IPV6_QUERY_NEIGHBOR_CACHE Query;
        uint BytesReturned;

        Query.IF.Index = 0;
        Query.Address = in6addr_any;

        switch (argc) {
        case 2:
            if (! GetAddress(argv[1], &Query.Address))
                usage();
            // fall-through

        case 1:
            if (! GetNumber(argv[0], &Query.IF.Index))
                usage();
            // fall-through

        case 0:
            break;

        default:
            usage();
        }

        if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE,
                             &Query, sizeof Query,
                             NULL, 0, &BytesReturned, NULL)) {
            printf("flush neighbor cache error: %x\n", GetLastError());
            exit(1);
        }
    }
}

void
FlushRouteCache(int argc, char *argv[])
{
    IPV6_QUERY_ROUTE_CACHE Query;
    uint BytesReturned;

    Query.IF.Index = 0;
    Query.Address = in6addr_any;

    switch (argc) {
    case 2:
        if (! GetAddress(argv[1], &Query.Address))
            usage();
        // fall-through

    case 1:
        if (! GetNumber(argv[0], &Query.IF.Index))
            usage();
        // fall-through

    case 0:
        break;

    default:
        usage();
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_ROUTE_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("flush route cache error: %x\n", GetLastError());
        exit(1);
    }
}

void
ForEachSitePrefix(void (*func)(IPV6_INFO_SITE_PREFIX *))
{
    IPV6_QUERY_SITE_PREFIX Query, NextQuery;
    IPV6_INFO_SITE_PREFIX SPE;
    uint BytesReturned;

    NextQuery.IF.Index = 0;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SITE_PREFIX,
                             &Query, sizeof Query,
                             &SPE, sizeof SPE, &BytesReturned,
                             NULL)) {
            printf("bad index %u\n", Query.IF.Index);
            exit(1);
        }

        NextQuery = SPE.Query;

        if (Query.IF.Index != 0) {

            SPE.Query = Query;
            (*func)(&SPE);
        }

        if (NextQuery.IF.Index == 0)
            break;
    }
}

void
PrintSitePrefix(IPV6_INFO_SITE_PREFIX *SPE)
{
    printf("%s/%u -> %u",
           FormatIPv6Address(&SPE->Query.Prefix),
           SPE->Query.PrefixLength,
           SPE->Query.IF.Index);

    if (SPE->ValidLifetime == (uint)-1)
        printf(" (lifetime infinite");
    else
        printf(" (lifetime %us", SPE->ValidLifetime);

    printf(")\n");
}

void
QuerySitePrefixTable(int argc, char *argv[])
{
    if (argc == 0) {
        ForEachSitePrefix(PrintSitePrefix);
    }
    else {
        usage();
    }
}

void
UpdateSitePrefixTable(int argc, char *argv[])
{
    IPV6_INFO_SITE_PREFIX SitePrefix;
    uint BytesReturned;
    int i;

    SitePrefix.ValidLifetime = 0xffffffff;

    if (argc < 2)
        usage();

    if (! GetNumber(argv[1],
                    &SitePrefix.Query.IF.Index))
        usage();

    if (! GetPrefix(argv[0],
                    &SitePrefix.Query.Prefix,
                    &SitePrefix.Query.PrefixLength))
        usage();

    for (i = 2; i < argc; i++) {
        if ((!strcmp(argv[i], "lifetime") ||
             !strcmp(argv[i], "life") ||
             !strcmp(argv[i], "l")) &&
            (i+1 < argc)) {

            if (! GetLifetimes(argv[++i], &SitePrefix.ValidLifetime, NULL))
                usage();
        }
        else
            usage();
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_SITE_PREFIX,
                         &SitePrefix, sizeof SitePrefix,
                         NULL, 0, &BytesReturned, NULL)) {
        printf("site prefix update error: %x\n", GetLastError());
        exit(1);
    }
}
