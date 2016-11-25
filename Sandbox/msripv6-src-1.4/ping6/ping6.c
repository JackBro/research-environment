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
// Packet INternet Groper utility for IPv6.
//


#include <windows.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
// Need ntddip6 before ws2ip6 to get CopyTDIFromSA6 and CopySAFromTDI6.
#include <ntddip6.h>
#include <ws2ip6.h>


#define MAX_BUFFER_SIZE         (sizeof(ICMPV6_ECHO_REPLY) + 0xfff7)
#define DEFAULT_BUFFER_SIZE     (0x2000 - 8)
#define DEFAULT_SEND_SIZE       32
#define DEFAULT_COUNT           4
#define DEFAULT_TIMEOUT         1000L
#define MIN_INTERVAL            1000L

void
PrintUsage(void)
{
    printf("\nUsage: ping6 [-t] [-a] [-n count] [-l size]"
           " [-w timeout] [-s srcaddr] dest\n\n"
           "Options:\n"
           "-t             Ping the specifed host until interrupted.\n"
           "-a             Resolve addresses to hostnames.\n"
           "-n count       Number of echo requests to send.\n"
           "-l size        Send buffer size.\n"
           "-w timeout     Timeout in milliseconds to wait for each reply.\n"
           "-s srcaddr     Source address (required for link-local dest).\n"
           "-r             Use routing header to test reverse route also.\n");
}

//
// Can only be called once, because
// a) does not call freeaddrinfo
// b) uses a static buffer for some results
//
int
get_pingee(char *ahstr, int dnsreq, struct sockaddr_in6 *address, char **hstr)
{
    struct addrinfo hints;
    struct addrinfo *result;
    char *name = NULL;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = PF_INET6;

    if (getaddrinfo(ahstr, NULL, &hints, &result) != 0) {
        //
        // Not a numeric address.
        // Try again with DNS name resolution.
        //
        hints.ai_flags = AI_CANONNAME;
        if (getaddrinfo(ahstr, NULL, &hints, &result) != 0) {
            //
            // Failure - we can not resolve the name.
            //
            return FALSE;
        }

        name = result->ai_canonname;
    }
    else {
        //
        // Should we do a reverse-lookup to get a name?
        //
        if (dnsreq) {
            static char namebuf[NI_MAXHOST];

            if (getnameinfo(result->ai_addr, result->ai_addrlen,
                            namebuf, sizeof namebuf,
                            NULL, 0,
                            NI_NAMEREQD) == 0) {
                //
                // Reverse lookup succeeded.
                //
                name = namebuf;
            }
        }
    }

    *address = * (struct sockaddr_in6 *) result->ai_addr;
    *hstr = name;
    return TRUE;
}

int
get_source(char *astr, struct sockaddr_in6 *address)
{
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = PF_INET6;

    if (getaddrinfo(astr, NULL, &hints, &result) != 0)
        return FALSE;

    *address = * (struct sockaddr_in6 *) result->ai_addr;
    return TRUE;
}

char *
format_addr(struct sockaddr_in6 *address)
{
    static char buffer[128];

    if (getnameinfo((struct sockaddr *)address, sizeof *address,
                    buffer, sizeof buffer, NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(buffer, "<invalid>");

    return buffer;
}

ulong
param(char **argv, int argc, int current, ulong min, ulong max)
{
    ulong   temp;
    char    *dummy;

    if (current == (argc - 1) ) {
        printf("Value must be supplied for option %s.\n", argv[current]);
        exit(1);
    }

    temp = strtoul(argv[current+1], &dummy, 0);
    if (temp < min || temp > max) {
        printf("Bad value for option %s.\n", argv[current]);
        exit(1);
    }

    return temp;
}

int _CRTAPI1
main(int argc, char **argv)
{
    char    *arg;
    uint    i;
    uint    j;
    int     found_addr = FALSE;
    int     dnsreq = FALSE;
    char    *hostname = NULL;
    struct sockaddr_in6 dstaddr, srcaddr;
    uint    Count = DEFAULT_COUNT;
    ulong   Timeout = DEFAULT_TIMEOUT;
    DWORD   errorCode;
    HANDLE  Handle;
    int     err;
    BOOL    result;
    PICMPV6_ECHO_REQUEST request;
    PICMPV6_ECHO_REPLY  reply;
    char    *SendBuffer, *RcvBuffer;
    uint    RcvSize;
    uint    SendSize = DEFAULT_SEND_SIZE;
    uint    ReplySize;
    DWORD   bytesReturned;
    WSADATA WsaData;
    int     Reverse = FALSE;

    memset(&srcaddr, 0, sizeof srcaddr);
    memset(&dstaddr, 0, sizeof dstaddr);

    err = WSAStartup(MAKEWORD(2, 0), &WsaData);
    if (err) {
        printf("Unable to initialize Windows Sockets interface, error code %d\n", GetLastError());
        exit(1);
    }

    if (argc < 2) {
        PrintUsage();
        exit(1);
    } else {
        i = 1;
        while (i < (uint) argc) {
            arg = argv[i];
            if (arg[0] == '-') {        // Have an option
                switch (arg[1]) {
                case '?':
                    PrintUsage();
                    exit(0);

                case 'l':
                    // Avoid jumbo-grams, we don't support them yet.
                    // Need to allow 8 bytes for the Echo Request header.
                    SendSize = (uint)param(argv, argc, i++, 0, 0xffff - 8);
                    break;

                case 't':
                    Count = (uint)-1;
                    break;

                case 'n':
                    Count = (uint)param(argv, argc, i++, 1, 0xffffffff);
                    break;

                case 'w':
                    Timeout = param(argv, argc, i++, 0, 0xffffffff);
                    break;

                case 'a':
                    dnsreq = TRUE;
                    break;

                case 'r':
                    Reverse = TRUE;
                    break;

                case 's':
                    if (!get_source(argv[++i], &srcaddr)) {
                        printf("Bad IPv6 address %s.\n", argv[i]);
                        exit(1);
                    }
                    break;

                default:
                    printf("Bad option %s.\n\n", arg);
                    PrintUsage();
                    exit(1);
                    break;
                }
                i++;
            } else {  // Not an option, must be an IPv6 address.
                if (found_addr) {
                    printf("Bad parameter %s.\n", arg);
                    exit(1);
                }
                if (get_pingee(arg, dnsreq, &dstaddr, &hostname)) {
                    found_addr = TRUE;
                    i++;
                } else {
                    printf("Bad IPv6 address %s.\n", arg);
                    exit(1);
                }
            }
        }
    }

    if (!found_addr) {
        printf("IPv6 address must be specified.\n");
        exit(1);
    }

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,          // desired access
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,       // security attributes
                         OPEN_EXISTING,
                         0,          // flags & attributes
                         NULL);      // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        printf("Unable to contact IPv6 driver, error code %d.\n",
               GetLastError() );
        exit(1);
    }

    request = LocalAlloc(LMEM_FIXED, sizeof *request + SendSize);
    if (request == NULL) {
        printf("Unable to allocate required memory.\n");
        exit(1);
    }

    SendBuffer = (char *)(request + 1);

    //
    // Calculate receive buffer size and try to allocate it.
    //
    if (SendSize <= DEFAULT_SEND_SIZE) {
        RcvSize = DEFAULT_BUFFER_SIZE;
    }
    else {
        RcvSize = MAX_BUFFER_SIZE;
    }

    reply = LocalAlloc(LMEM_FIXED, sizeof *reply + RcvSize);
    if (reply == NULL) {
        printf("Unable to allocate required memory.\n");
        exit(1);
    }

    RcvBuffer = (char *)(reply + 1);

    //
    // Initialize the request buffer.
    //
    CopyTDIFromSA6(&request->DstAddress, &dstaddr);
    CopyTDIFromSA6(&request->SrcAddress, &srcaddr);
    request->Flags = 0;
    if (Reverse)
        request->Flags |= ICMPV6_ECHO_REQUEST_FLAG_REVERSE;
    request->Timeout = Timeout;
    request->TTL = 0; // default TTL

    //
    // Initialize the request data pattern.
    //
    for (i = 0; i < SendSize; i++)
        SendBuffer[i] = 'a' + (i % 23);

    if (hostname) {
        printf("\nPinging %s [%s] with %u bytes of data:\n\n",
               hostname, format_addr(&dstaddr), SendSize);
    } else {
        printf("\nPinging %s with %u bytes of data:\n\n",
               format_addr(&dstaddr), SendSize);
    }

    for (i = 0; i < Count; i++) {

        if (! DeviceIoControl(Handle,
                              IOCTL_ICMPV6_ECHO_REQUEST,
                              request,
                              sizeof *request + SendSize,
                              reply, sizeof *reply + RcvSize,
                              &bytesReturned,
                              NULL) ||
            (bytesReturned < sizeof *reply)) {

            errorCode = GetLastError();

            if (errorCode < IP_STATUS_BASE) {
                printf("PING: transmit failed, error code %u\n", errorCode);
            }
            else {
                printf("PING: %s\n", GetErrorString(errorCode));
            }
        }
        else {
            struct sockaddr_in6 ReplyAddress;

            ReplySize = bytesReturned - sizeof *reply;

            CopySAFromTDI6(&ReplyAddress, &reply->Address);

            if (! IN6_ADDR_EQUAL(&ReplyAddress.sin6_addr, &in6addr_any))
                printf("Reply from %s: ", format_addr(&ReplyAddress));

            if (reply->Status == IP_SUCCESS) {

                printf("bytes=%u ", ReplySize);

                if (ReplySize != SendSize) {
                    printf("(sent %u) ", SendSize);
                }
                else {
                    char *sendptr, *recvptr;

                    sendptr = SendBuffer;
                    recvptr = RcvBuffer;

                    for (j = 0; j < SendSize; j++)
                        if (*sendptr++ != *recvptr++) {
                            printf("- MISCOMPARE at offset %u - ", j);
                            break;
                        }
                }

                if (reply->RoundTripTime) {
                    printf("time=%ums ", reply->RoundTripTime);
                }
                else {
                    printf("time<1ms ");
                }

                printf("\n");
            }
            else {
                printf("%s\n", GetErrorString(reply->Status));

                if ((reply->Status == IP_DEST_NO_ROUTE) &&
                    (IN6_IS_ADDR_LINKLOCAL(&dstaddr.sin6_addr) ||
                     IN6_IS_ADDR_SITELOCAL(&dstaddr.sin6_addr) ||
                     IN6_IS_ADDR_MULTICAST(&dstaddr.sin6_addr)) &&
                    (dstaddr.sin6_scope_id == 0)) {
                    printf("  Specify a scope-id or use -s to specify source address.\n");
                }
            }

            if (i < (Count - 1)) {

                if (reply->RoundTripTime < MIN_INTERVAL) {
                    Sleep(MIN_INTERVAL - reply->RoundTripTime);
                }
            }
        }
    }

    return 0;
}
