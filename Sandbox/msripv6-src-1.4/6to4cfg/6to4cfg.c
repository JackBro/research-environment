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
// 6to4 autoconfiguration
// This is basically a simple ping program from the samples of VC++ that was
// changed to configure an MSRIPv6 host.  
//
// The relay router at MSR is pinged to verify reachability.
// Then the 6to4 configuration changes are made directly
// or written out to a file for later executation.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <stdio.h>
#include <stdlib.h>

#define INFINITE_LIFETIME       0xffffffff

//
// Global configuration variables.
//
FILE *File = NULL;
int BeRouter = FALSE;
int Undo = FALSE;
int UseSiteLocals = FALSE;

extern int
InitIPv6Library(void);

extern int
ConfirmIPv4Reachability(IN_ADDR Dest, uint Timeout, IN_ADDR *Source);

extern int
ConfirmIPv6Reachability(IN6_ADDR *Dest, uint Timeout, IN6_ADDR *Source);

extern void
Configure6to4(IN_ADDR RelayAddr, IN_ADDR OurAddr);

extern void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context);

extern int
ControlInterface(IPV6_CONTROL_INTERFACE *Control);

extern int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route);

extern int
UpdateAddress(IPV6_UPDATE_ADDRESS *Address);

void
usage(void)
{
    fprintf(stderr,
            "usage: 6to4cfg [-r] [-u] [-s] [-R v4-addr] [filename]\n"
            "Connects to the global IPv6 network using 6to4 addresses.\n"
            "   -r              Become a router for the local network.\n"
            "   -s              Enable site-local addressing. Needs -r.\n"
            "   -u              Undoes the 6to4 configuration.\n"
            "   -R v4-addr      Specify IPv4 address of a 6to4 relay.\n"
            "   filename        Output for the configuration script.\n\n"
            "If no filename is specified, 6to4cfg just performs\n"
            "the configuration directly on the current machine.\n\n"
            "The default 6to4 relay is the MSR relay, 131.107.65.121.\n\n"
            "An error in direct configuration probably means you\n"
            "have a version mismatch between 6to4cfg and the IPv6 stack.\n");
    exit(1);
}

int _CRTAPI1
main(int argc, char **argv)
{
    char *Filename = NULL;
    char *Relay = "131.107.65.121";

    WSADATA wsaData;
    IN_ADDR Dest, Source;
    IN6_ADDR Dest6, Source6;
    TIMEVAL Timeout;
    int i;

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %u\n", GetLastError());
        exit(1);
    }

    if (!InitIPv6Library()) {
        fprintf(stderr, "MSR IPv6 protocol stack not installed?\n");
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r"))
            BeRouter = TRUE;
        else if (!strcmp(argv[i], "-s"))
            UseSiteLocals = TRUE;
        else if (!strcmp(argv[i], "-u"))
            Undo = TRUE;
        else if (!strcmp(argv[i], "-R") && (i+1 < argc))
            Relay = argv[++i];
        else if (argv[i][0] == '-')
            usage();
        else if (Filename == NULL)
            Filename = argv[i];
        else
            usage();
    }

    if (UseSiteLocals && !BeRouter)
        usage();

    //
    // Get the IPv4 address of a 6to4 relay router.
    // The relay router forwards between the 6to4 address space
    // and the native IPv6 address space.
    //
    Dest.s_addr = inet_addr(Relay);

    //
    // Probe the relay router to discover an appropriate IPv4 address,
    // assigned to this machine, that we can use to create
    // a 6to4 site prefix.
    //
    for (i = 0; i < 5; i++) {
        if (ConfirmIPv4Reachability(Dest, 1000/*ms*/, &Source)) {

            //
            // We have v4 connectivity... check v6 connectivity
            // using v4-compatible v6 address for the relay router.
            //
            memset(&Dest6, 0, sizeof Dest6);
            Dest6.s6_dwords[3] = Dest.s_addr;
            if (! ConfirmIPv6Reachability(&Dest6, 1000/*ms*/, &Source6)) {
                printf("Could not tunnel a v6 packet to the 6to4 relay - %s.\n"
                       "Perhaps there is a firewall in the way?\n",
                       inet6_ntoa(&Dest6));
                Sleep(1000);
                exit(0);
            }

            //
            // Open the output file, if specified.
            // We only do this after confirming reachability,
            // so if that fails then we avoid overwriting the file.
            // If there is no output file, we directly configure
            // the current machine.
            //
            if (Filename != NULL) {
                File = fopen(Filename, "w");
                if (File == NULL) {
                    fprintf(stderr, "Could not open %s for output.\n",
                            Filename);
                    exit(1);
                }
            }

            Configure6to4(Dest, Source);
            exit(0);
        }
    }

    printf("Could not reach the 6to4 relay router - %s\n", inet_ntoa(Dest));
    Sleep(1000);
    return 0;
}

void
ConfigureRouteTableUpdate(
    const IN6_ADDR *Prefix,
    uint PrefixLen,
    uint Interface,
    const IN6_ADDR *Neighbor,
    int Publish,
    int Immortal,
    uint Lifetime,
    uint SitePrefixLen)
{
    if (File != NULL) {
        char PrefixStr[64];
        char NeighborStr[64];

        inet_ntop(AF_INET6, Prefix, PrefixStr, sizeof PrefixStr);
        inet_ntop(AF_INET6, Neighbor, NeighborStr, sizeof NeighborStr);

        fprintf(File, "ipv6 rtu %s/%u", PrefixStr, PrefixLen);
        if (IN6_IS_ADDR_UNSPECIFIED(Neighbor))
            fprintf(File, " %u", Interface);
        else
            fprintf(File, " %u/%s", Interface, NeighborStr);

        if (Publish)
            fprintf(File, " pub");

        if (Immortal != Publish) {
            if (Immortal)
                fprintf(File, " noage");
            else
                fprintf(File, " age");
        }

        if (Lifetime != INFINITE_LIFETIME)
            fprintf(File, " life %u", Lifetime);

        if (SitePrefixLen != 0)
            fprintf(File, " spl %u", SitePrefixLen);

        fprintf(File, "\n");
    }
    else {
        IPV6_INFO_ROUTE_TABLE Route;

        memset(&Route, 0, sizeof Route);
        Route.Query.Prefix = *Prefix;
        Route.Query.PrefixLength = PrefixLen;
        Route.Query.Neighbor.IF.Index = Interface;
        Route.Query.Neighbor.Address = *Neighbor;
        Route.ValidLifetime = Lifetime;
        Route.Publish = Publish;
        Route.Immortal = Immortal;
        Route.SitePrefixLength = SitePrefixLen;

        if (!UpdateRouteTable(&Route)) {
            fprintf(stderr, "6to4cfg: UpdateRouteTable failed\n");
            exit(1);
        }
    }
}

void
ConfigureInterfaceControl(
    uint Interface,
    int Advertises,
    int Forwards)
{
    if (File != NULL) {
        fprintf(File, "ipv6 ifc %u", Interface);

        if (Forwards != -1) {
            if (Forwards)
                fprintf(File, " forw");
            else
                fprintf(File, " -forw");
        }

        if (Advertises != -1) {
            if (Advertises)
                fprintf(File, " adv");
            else
                fprintf(File, " -adv");
        }

        fprintf(File, "\n");
    }
    else {
        IPV6_CONTROL_INTERFACE Control;

        memset(&Control, 0, sizeof Control);
        Control.Query.Index = Interface;
        Control.Advertises = Advertises;
        Control.Forwards = Forwards;

        if (!ControlInterface(&Control)) {
            fprintf(stderr, "6to4cfg: ControlInterface failed\n");
            exit(1);
        }
    }
}

void
ConfigureAddressUpdate(
    uint Interface,
    IN6_ADDR *Addr,
    uint Lifetime,
    int Anycast)
{
    if (File != NULL) {
        char AddrStr[64];

        inet_ntop(AF_INET6, Addr, AddrStr, sizeof AddrStr);

        fprintf(File, "ipv6 adu %u/%s", Interface, AddrStr);

        if (Anycast)
            fprintf(File, " anycast");

        if (Lifetime != INFINITE_LIFETIME)
            fprintf(File, " life %u", Lifetime);

        fprintf(File, "\n");
    }
    else {
        IPV6_UPDATE_ADDRESS Address;

        memset(&Address, 0, sizeof Address);
        Address.Query.IF.Index = Interface;
        Address.Query.Address = *Addr;
        Address.ValidLifetime = Address.PreferredLifetime = Lifetime;
        Address.Type = Anycast ? 1 : 0;

        if (!UpdateAddress(&Address)) {
            fprintf(stderr, "6to4cfg: UpdateAddress failed\n");
            exit(1);
        }
    }
}

void
Configure6to4Subnets(IPV6_INFO_INTERFACE *IF, void *Context)
{
    IN_ADDR *OurAddr = (IN_ADDR *) Context;
    IN6_ADDR SubnetPrefix;
    IN6_ADDR SiteLocalPrefix;

    if (IF->Discovers) {
        //
        // Create a subnet prefix for the interface,
        // using the interface index as the subnet number.
        //
        memset(&SubnetPrefix, 0, sizeof SubnetPrefix);
        SubnetPrefix.s6_words[0] = htons(0x2002);
        *(IN_ADDR *)&SubnetPrefix.s6_words[1] = *OurAddr;
        SubnetPrefix.s6_words[3] = htons((ushort)IF->Query.Index);

        //
        // Create a site-local prefix for the interface,
        // using the interface index as the subnet number.
        //
        memset(&SiteLocalPrefix, 0, sizeof SiteLocalPrefix);
        SiteLocalPrefix.s6_words[0] = htons(0xfec0);
        SiteLocalPrefix.s6_words[3] = htons((ushort)IF->Query.Index);

        if (Undo) {
            //
            // Give the route a very short lifetime and make it age away.
            // This will allow a Router Advertisement to be sent
            // before a subsequent command disables advertising.
            //
            ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                                      IF->Query.Index, &in6addr_any,
                                      TRUE, FALSE, 2, 0);

            if (UseSiteLocals)
                ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                          IF->Query.Index, &in6addr_any,
                                          TRUE, FALSE, 2, 0);
        }
        else {
            //
            // Configure the subnet route.
            //
            ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                                      IF->Query.Index, &in6addr_any,
                                      TRUE, TRUE, 1800,
                                      UseSiteLocals ? 48 : 0);

            if (UseSiteLocals)
                ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                          IF->Query.Index, &in6addr_any,
                                          TRUE, TRUE, 1800, 0);
        }
    }
}

void
Configure6to4Routing(IPV6_INFO_INTERFACE *IF, void *Context)
{
    if (IF->Discovers) {
        if (Undo) {
            //
            // Disable routing on the interface.
            //
            ConfigureInterfaceControl(IF->Query.Index, FALSE, FALSE);
        }
        else {
            //
            // Enable routing on the interface.
            //
            ConfigureInterfaceControl(IF->Query.Index, TRUE, TRUE);
        }
    }
}

void
Configure6to4(IN_ADDR RelayAddr, IN_ADDR OurAddr)
{
    IN6_ADDR SixToFourPrefix;
    IN6_ADDR RelayRouterAddress;
    IN6_ADDR OurAddress;

    //
    // Create a 6to4 address for this machine.
    //

    memset(&OurAddress, 0, sizeof OurAddress);
    OurAddress.s6_words[0] = htons(0x2002);
    *(IN_ADDR *)&OurAddress.s6_words[1] = OurAddr;
    *(IN_ADDR *)&OurAddress.s6_words[6] = OurAddr;

    if (Undo) {
        //
        // Remove the address.
        //
        ConfigureAddressUpdate(2, &OurAddress, 0, FALSE);
    }
    else {
        //
        // Configure the address.
        //
        ConfigureAddressUpdate(2, &OurAddress, INFINITE_LIFETIME, FALSE);
    }

    //
    // If this is a router, create an anycast address as well.
    //
    if (BeRouter) {
        IN6_ADDR AnycastAddress;

        memset(&AnycastAddress, 0, sizeof AnycastAddress);
        AnycastAddress.s6_words[0] = htons(0x2002);
        *(IN_ADDR *)&AnycastAddress.s6_words[1] = OurAddr;

        if (Undo) {
            //
            // Remove the address.
            //
            ConfigureAddressUpdate(2, &AnycastAddress, 0, TRUE);
        }
        else {
            //
            // Configure the address.
            //
            ConfigureAddressUpdate(2, &AnycastAddress, INFINITE_LIFETIME, TRUE);
        }
    }

    //
    // Create a route for the 6to4 prefix.
    // This route causes packets sent to 6to4 addresses
    // to be encapsulated and sent to the extracted v4 address.
    //

    memset(&SixToFourPrefix, 0, sizeof SixToFourPrefix);
    SixToFourPrefix.s6_words[0] = htons(0x2002);

    if (Undo) {
        //
        // Give the route a very short lifetime and make it age away.
        // This will allow a Router Advertisement to be sent
        // before a subsequent command disables advertising.
        //
        ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                  2, &in6addr_any,
                                  TRUE, FALSE, 2, 0);
    }
    else {
        //
        // Configure the 6to4 route.
        //
        ConfigureRouteTableUpdate(&SixToFourPrefix, 16,
                                  2, &in6addr_any,
                                  TRUE, TRUE, 1800, 0);
    }

    //
    // Create a default configured tunnel to the relay router
    // for all other packets.
    //

    memset(&RelayRouterAddress, 0, sizeof RelayRouterAddress);
    *(IN_ADDR *)&RelayRouterAddress.s6_words[6] = RelayAddr;

    if (Undo) {
        //
        // Give the route a very short lifetime and make it age away.
        // This will allow a Router Advertisement to be sent
        // before a subsequent command disables advertising.
        //
        ConfigureRouteTableUpdate(&in6addr_any, 0,
                                  2, &RelayRouterAddress,
                                  TRUE, FALSE, 2, 0);
    }
    else {
        //
        // Create the tunnel.
        //
        ConfigureRouteTableUpdate(&in6addr_any, 0,
                                  2, &RelayRouterAddress,
                                  TRUE, TRUE, 1800, 0);
    }

    if (BeRouter) {
        //
        // Configure subnets on each real interface.
        //
        ForEachInterface(Configure6to4Subnets, &OurAddr);

        //
        // Configure routing on each real interface.
        //
        if (Undo && (File == NULL)) {
            //
            // But first, wait a bit so that Router Advertisements
            // based on the new small-lifetime routes get generated.
            //
            Sleep(2000);
        }
        ForEachInterface(Configure6to4Routing, NULL);

        if (Undo) {
            //
            // Disable forwarding on the tunnel pseudo-interface.
            //
            ConfigureInterfaceControl(2, -1, FALSE);
        }
        else {
            //
            // Enable forwarding on the tunnel pseudo-interface.
            //
            ConfigureInterfaceControl(2, -1, TRUE);
        }
    }
}
