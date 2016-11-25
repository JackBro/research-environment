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
// Test program for IPv6 APIs.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <stdio.h>
#include <stdlib.h>

//
// Prototypes for local functions.
//
int Test_inet6_ntoa();
int Test_inet_ntop();
int Test_inet6_addr();
int Test_inet_pton();
int Test_getipnodebyname(int argc, char **argv);
int Test_getipnodebyaddr();
int Test_getaddrinfo(int argc, char **argv);
int Test_getnameinfo();


//
// Global variables.
//
struct in_addr v4Address = {157, 55, 254, 211};
struct in6_addr v6Address = {0x3f, 0xfe, 0x1c, 0xe1, 0x00, 0x00, 0xfe, 0x01,
                             0x02, 0xa0, 0xcc, 0xff, 0xfe, 0x3b, 0xce, 0xef};
struct in6_addr DeadBeefCafeBabe = {0xde, 0xad, 0xbe, 0xef,
                                    0xca, 0xfe, 0xba, 0xbe,
                                    0x01, 0x23, 0x45, 0x67,
                                    0x89, 0xab, 0xcd, 0xef};
struct in6_addr MostlyZero = {0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
struct in6_addr v4Mapped = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0xff, 0xff, 157, 55, 254, 211};

struct sockaddr_in v4SockAddr = {AF_INET, 6400, {157, 55, 254, 211}, 0};
struct sockaddr_in6 v6SockAddr = {AF_INET6, 2, 0,
                                  {0x3f, 0xfe, 0x1c, 0xe1,
                                   0x00, 0x00, 0xfe, 0x01,
                                   0x02, 0xa0, 0xcc, 0xff,
                                   0xfe, 0x3b, 0xce, 0xef}, 0};
struct sockaddr_in6 DBCBSockAddr = {AF_INET6, 2, 0,
                                    {0xde, 0xad, 0xbe, 0xef,
                                     0xca, 0xfe, 0xba, 0xbe,
                                     0x01, 0x23, 0x45, 0x67,
                                     0x89, 0xab, 0xcd, 0xef}, 0};
struct sockaddr_in6 LinkLocalSockAddr = {AF_INET6, 0x1500, 0,
                                         {0xfe, 0x80, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00,
                                          0x01, 0x23, 0x45, 0x67,
                                          0x89, 0xab, 0xcd, 0xef}, 3};

int _CRTAPI1
main(int argc, char **argv)
{
    WSADATA wsaData;
    int Failed;

    //
    // Initialize Winsock.
    //
    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("WSAStartup failed\n");
        exit(1);
    }

    printf("Performing various tests...\n");

    //
    // Run tests.
    //
    Failed = Test_inet6_ntoa();
    Failed += Test_inet_ntop();
    Failed += Test_inet6_addr();
    Failed += Test_inet_pton();
    Failed += Test_getipnodebyname(argc, argv);
    Failed += Test_getipnodebyaddr();
    Failed += Test_getaddrinfo(argc, argv);
    Failed += Test_getnameinfo();

    printf("%d of the tests failed\n", Failed);

    return 0;
}


//* Test_inet6_ntoa - Test inet6_ntoa.
//
int
Test_inet6_ntoa()
{
    char *ReturnValue;
    int Error;
    int NumberFailed = 0;

    printf("\ninet6_ntoa:\n\n");

    // Tests with reasonable input:
    WSASetLastError(0);
    ReturnValue = inet6_ntoa(&DeadBeefCafeBabe);
    Error = WSAGetLastError();
    printf("inet6_ntoa(&DeadBeefCafeBabe)\nReturns %s\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((strncmp("dead:beef:cafe:babe:123:4567:89ab:cdef", ReturnValue,
               128) != 0) || (Error != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    WSASetLastError(0);
    ReturnValue = inet6_ntoa(&MostlyZero);
    Error = WSAGetLastError();
    printf("inet6_ntoa(&MostlyZero)\nReturns %s\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((strncmp("3ffe::1", ReturnValue, 128) != 0) || (Error != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    WSASetLastError(0);
    ReturnValue = inet6_ntoa(&v4Mapped);
    Error = WSAGetLastError();
    printf("inet6_ntoa(&v4Mapped)\nReturns %s\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((strncmp("::ffff:157.55.254.211", ReturnValue, 128) != 0) ||
        (Error != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

#if 0  // inet6_ntoa faults if passed NULL, so this will fail:
    WSASetLastError(0);
    ReturnValue = inet6_ntoa(NULL);
    Error = WSAGetLastError();
    printf("inet6_ntoa(NULL)\nReturns %s\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
#endif

    return NumberFailed;
}


//* Test_inet_ntop - Test inet_ntop.
//
int
Test_inet_ntop()
{
    char Out[INET_ADDRSTRLEN];
    char Out6[INET6_ADDRSTRLEN];
    char Tiny[2];
    const char *ReturnValue;
    int Error;
    int NumberFailed = 0;

    printf("\ninet_ntop:\n\n");

    // Tests with reasonable input:
    memset(Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET, &v4Address, Out, sizeof Out);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET, &v4Address, Out, sizeof Out)\nReturns %s\n"
           "Out = %s\n", ReturnValue, Out);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((strncmp("157.55.254.211", ReturnValue, INET_ADDRSTRLEN) != 0) ||
        (ReturnValue != Out) || (Error != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET6, &DeadBeefCafeBabe, Out6, sizeof Out6);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET6, &DeadBeefCafeBabe, Out6, sizeof Out6)\n"
           "Returns %s\nOut6 = %s\n", ReturnValue, Out6);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((strncmp("dead:beef:cafe:babe:123:4567:89ab:cdef", ReturnValue,
                 INET6_ADDRSTRLEN) != 0) ||
        (ReturnValue != Out6) || (Error != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Test unknown AF.
    memset(Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_ntop(7734, &v4Address, Out, sizeof Out);
    Error = WSAGetLastError();
    printf("inet_ntop(7734, &v4Address, Out, sizeof Out)\nReturns %s\n"
           "Out = %s\n", ReturnValue, Out);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != NULL) || (Error != WSAEAFNOSUPPORT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

#if 0  // Test NULL Address (this faults).
    memset(Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET, NULL, Out, sizeof Out);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET, NULL, Out, sizeof Out)\nReturns %s\n"
           "Out = %s\n", ReturnValue, Out);
    printf("WSAGetLastError = %d\n\n", Error);
#endif

    // Test too-small output buffer.
    memset(Tiny, 0, sizeof Tiny);
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET, &v4Address, Tiny, sizeof Tiny);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET, &v4Address, Tiny, sizeof Tiny)\nReturns %s\n"
           "Tiny = %s\n", ReturnValue, Tiny);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != NULL) || (Error != WSAEFAULT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(Tiny, 0, sizeof Tiny);
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET6, &DeadBeefCafeBabe, Tiny, sizeof Tiny);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET6, &DeadBeefCafeBabe, Tiny, sizeof Tiny)\n"
           "Returns %s\nTiny = %s\n", ReturnValue, Tiny);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != NULL) || (Error != WSAEFAULT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Test NULL output buffer.
    WSASetLastError(0);
    ReturnValue = inet_ntop(AF_INET, &v4Address, NULL, 0);
    Error = WSAGetLastError();
    printf("inet_ntop(AF_INET, &v4Address, NULL, 0)\nReturns %s\n",
          ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != NULL) || (Error != WSAEFAULT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    return NumberFailed;
};


//* Test_inet6_addr - Test inet6_addr.
//
int
Test_inet6_addr()
{
    struct in6_addr Out6;
    int ReturnValue;
    int Error;
    int Test;
    int NumberFailed = 0;

    printf("\ninet6_addr:\n\n");

    // Tests with reasonable input:
    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr("dead:beef:cafe:babe:0123:4567:89ab:cdef", &Out6);
    Error = WSAGetLastError();
    Test = memcmp(&Out6, &DeadBeefCafeBabe, sizeof Out6);
    printf("inet6_addr(\"dead:beef:cafe:babe:0123:4567:89ab:cdef\", &Out6)\n"
           "Returns %d\nOut6 == DeadBeefCafeBabe? %s\n", ReturnValue,
           Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr("3ffe::1", &Out6);
    Error = WSAGetLastError();
    Test = memcmp(&Out6, &MostlyZero, sizeof Out6);
    printf("inet6_addr(\"3ffe::1\", &Out6)\n"
           "Returns %d\nOut6 == MostlyZero? %s\n", ReturnValue,
           Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr("3ffe:0::0:1", &Out6);
    Error = WSAGetLastError();
    Test = memcmp(&Out6, &MostlyZero, sizeof Out6);
    printf("inet6_addr(\"3ffe:0::0:1\", &Out6)\n"
           "Returns %d\nOut6 == MostlyZero? %s\n", ReturnValue,
           Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Tests with unreasonable input:
    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr("bad:addr::0:1", &Out6);
    Error = WSAGetLastError();
    printf("inet6_addr(\"bad:addr::0:1\", &Out6)\nReturns %d\nOut6 = %s\n",
           ReturnValue, inet6_ntoa(&Out6));
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 0) || (Error != WSAEINVAL)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

#if 0  // inet6_addr faults if passed a NULL input string, so this will fail:
    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr(NULL, &Out6);
    Error = WSAGetLastError();
    printf("inet6_addr(NULL, &Out6)\n Returns %d\nOut6 = %s\n", ReturnValue,
           inet6_ntoa(&Out6));
    printf("WSAGetLastError = %d\n\n", Error);
#endif

#if 0  // inet6_addr faults if passed a NULL Address, so this will fail:
    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet6_addr("3ffe::1", NULL);
    Error = WSAGetLastError();
    printf("inet6_addr(\"3ffe::1\", NULL)\nReturns %d\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
#endif

    return NumberFailed;
};


//* Test_inet_pton - test inet_pton.
//
int
Test_inet_pton()
{
    struct in_addr Out;
    struct in6_addr Out6;
    int ReturnValue;
    int Error;
    int Test;
    int NumberFailed = 0;

    printf("\ninet_pton:\n\n");

    // Tests with reasonable input:
    memset(&Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET, "157.55.254.211", &Out);
    Error = WSAGetLastError();
    Test = memcmp(&Out, &v4Address, sizeof Out);
    printf("inet6_pton(AF_INET, \"157.55.254.211\", &Out)\nReturns %d\n"
           "Out == v4Address? %s\n", ReturnValue, Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET6,
                            "dead:beef:cafe:babe:0123:4567:89ab:cdef", &Out6);
    Error = WSAGetLastError();
    Test = memcmp(&Out6, &DeadBeefCafeBabe, sizeof Out6);
    printf("inet6_pton(AF_INET6, \"dead:beef:cafe:babe:0123:4567:89ab:cdef\","
           " &Out6)\nReturns %d\nOut6 == DeadBeefCafeBabe? %s\n", ReturnValue,
           Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Test unknown AF.
    memset(&Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_pton(7734, "157.55.254.211", &Out);
    Error = WSAGetLastError();
    Test = memcmp(&Out, &v4Address, sizeof Out);
    printf("inet6_pton(7734, \"157.55.254.211\", &Out)\nReturns %d\n"
           "Out == v4Address? %s\n", ReturnValue,
           Test ? "FALSE" : "TRUE");
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != -1) || (Error != WSAEAFNOSUPPORT) || (Test == 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Test if it will interpret numbers with a leading zero as octal.
    memset(&Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET, "157.055.254.211", &Out);
    Error = WSAGetLastError();
    Test = memcmp(&Out, &v4Address, sizeof Out);
    printf("inet6_pton(AF_INET, \"157.055.254.211\", &Out)\nReturns %d\n"
           "Out == v4Address? %s\n", ReturnValue,
           memcmp(&Out, &v4Address, sizeof Out) ? "FALSE" : "TRUE");
    printf("Out = %s\n", inet_ntoa(Out));
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 1) || (Error != 0) || (Test != 0)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    // Tests with questionable input:
    memset(&Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET, "172.31.42.339", &Out);
    Error = WSAGetLastError();
    printf("inet6_pton(AF_INET, \"172.31.42.339\", &Out)\nReturns %d\n",
           ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 0) || (Error != WSAEINVAL)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET6, "bad:addr::0:1", &Out6);
    Error = WSAGetLastError();
    printf("inet6_pton(AF_INET6, \"bad:addr::0:1\", &Out6)\nReturns %d\n",
           ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 0) || (Error != WSAEINVAL)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out, 0, sizeof Out);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET, NULL, &Out);
    Error = WSAGetLastError();
    printf("inet6_pton(AF_INET, NULL, &Out)\nReturns %d\n", ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 0) || (Error != WSAEFAULT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    memset(&Out6, 0, sizeof Out6);
    WSASetLastError(0);
    ReturnValue = inet_pton(AF_INET6, NULL, &Out6);
    Error = WSAGetLastError();
    printf("inet6_pton(AF_INET6, NULL, &Out6)\nReturns %d\n",
           ReturnValue);
    printf("WSAGetLastError = %d\n\n", Error);
    if ((ReturnValue != 0) || (Error != WSAEFAULT)) {
        printf("FAILED!\n\n\n");
        NumberFailed++;
    }

    return NumberFailed;
};


//* DumpHostEntry - print the contents of a hostent structure to standard out.
//
void
DumpHostEntry(struct hostent *HostEntry)
{
    char **Item;
    int Count;
    char Buffer[INET6_ADDRSTRLEN];  // INET6_ADDRSTRLEN > INET_ADDRSTRLEN.

    if (HostEntry != NULL) {
        printf("HostEntry->h_name = %s\n", HostEntry->h_name);
        for (Item = HostEntry->h_aliases, Count = 0; *Item != NULL; Item++) {
            printf("HostEntry->h_aliases[%d] = %s\n", Count++, *Item);
        }
        printf("HostEntry->h_addrtype = %u\n", HostEntry->h_addrtype);
        printf("HostEntry->h_length = %u\n", HostEntry->h_length);
        for (Item = HostEntry->h_addr_list, Count = 0; *Item != NULL; Item++) {
            printf("HostEntry->h_addr_list[%d] = %s\n", Count++,
                   inet_ntop(HostEntry->h_addrtype, *Item, Buffer,
                             sizeof Buffer));
        }

    } else {
        printf("HostEntry = (null)\n");
    }
}


//* FreeHostEntry - Safely free a purported hostent structure.
//
void
FreeHostEntry(
    struct hostent *Free)
{
    if (Free != 0)
        freehostent(Free);
}


//* Test_getipnodebyname - Test getipnodebyname.
//
int
Test_getipnodebyname(int argc, char **argv)
{
    struct hostent *HostEntry;
    char *NodeName, *TestName;
    int Error;

    if (argc < 2)
        NodeName = "localhost";
    else
        NodeName = argv[1];

    printf("\ngetipnodebyname (nodename = %s):\n\n", NodeName);

    // Test with reasonable input:
    Error = 0;
    HostEntry = getipnodebyname(NodeName, AF_INET, 0, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET, 0, &Error)\n",
           NodeName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyname(NodeName, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET6, AI_ALL, &Error)\n",
           NodeName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    TestName = "157.55.254.211";
    HostEntry = getipnodebyname(TestName, AF_INET, 0, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET, 0, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    TestName = "157.55.254.211";
    HostEntry = getipnodebyname(TestName, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET6, AI_ALL, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    TestName = "dead:beef:cafe:babe:0123:4567:89ab:cdef";
    HostEntry = getipnodebyname(TestName, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET6, AI_ALL, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    // Test if it will interpret numbers with a leading zero as octal.
    Error = 0;
    TestName = "157.055.254.211";
    HostEntry = getipnodebyname(TestName, AF_INET, 0, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET, 0, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    TestName = "157.055.254.211";
    HostEntry = getipnodebyname(TestName, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET6, AI_ALL, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    // Tests with questionable input:
    Error = 0;
    TestName = "172.31.42.339";
    HostEntry = getipnodebyname(TestName, AF_INET, 0, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET, 0, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    TestName = "bad:addr::0:1";
    HostEntry = getipnodebyname(TestName, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(\"%s\", AF_INET6, AI_ALL, &Error)\n",
           TestName);
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyname(NULL, AF_INET, 0, &Error);
    printf("HostEntry = getipnodebyname(NULL, AF_INET, 0, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyname(NULL, AF_INET6, AI_ALL, &Error);
    printf("HostEntry = getipnodebyname(NULL, AF_INET6, AI_ALL, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    return 0;
};


//* Test_getipnodebyaddr - Test getipnodebyaddr.
//
int
Test_getipnodebyaddr()
{
    struct hostent *HostEntry;
    int Error;

    printf("\ngetipnodebyaddr:\n\n");

    // Test with reasonable input:
    Error = 0;
    HostEntry = getipnodebyaddr(&v4Address, sizeof v4Address, AF_INET, &Error);
    printf("HostEntry = getipnodebyaddr(&v4Address, sizeof v4Address,"
           " AF_INET, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyaddr(&v6Address, sizeof v6Address, AF_INET6,
                                &Error);
    printf("HostEntry = getipnodebyaddr(&v6Address, sizeof v6Address,"
           " AF_INET6, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyaddr(&DeadBeefCafeBabe, sizeof DeadBeefCafeBabe,
                                AF_INET6, &Error);
    printf("HostEntry = getipnodebyaddr(&DeadBeefCafeBabe,"
           " sizeof DeadBeefCafeBabe, AF_INET6, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    // Test unknown AF.
    Error = 0;
    HostEntry = getipnodebyaddr(&v4Address, sizeof v4Address, 7734, &Error);
    printf("HostEntry = getipnodebyaddr(&v4Address, sizeof v4Address,"
           " 7734, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    // Probably should test too small/big length value.

    // Test NULL addresses.
    Error = 0;
    HostEntry = getipnodebyaddr(NULL, 0, AF_INET, &Error);
    printf("HostEntry = getipnodebyaddr(NULL, 0, AF_INET, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    Error = 0;
    HostEntry = getipnodebyaddr(NULL, 0, AF_INET6, &Error);
    printf("HostEntry = getipnodebyaddr(NULL, 0, AF_INET6, &Error)\n");
    DumpHostEntry(HostEntry);
    printf("Error = %d\n\n", Error);
    FreeHostEntry(HostEntry);

    return 0;
};


//* DumpAddrInfo - print the contents of an addrinfo structure to standard out.
//
void
DumpAddrInfo(struct addrinfo *AddrInfo)
{
    int Count;

    for (Count = 1; AddrInfo!= NULL; AddrInfo = AddrInfo->ai_next) {
        printf("AddrInfo #%u:\n", Count++);
        printf("AddrInfo->ai_flags = %u\n", AddrInfo->ai_flags);
        printf("AddrInfo->ai_family = %u\n", AddrInfo->ai_family);
        printf("AddrInfo->ai_socktype = %u\n", AddrInfo->ai_socktype);
        printf("AddrInfo->ai_protocol = %u\n", AddrInfo->ai_protocol);
        printf("AddrInfo->ai_addrlen = %u\n", AddrInfo->ai_addrlen);
        printf("AddrInfo->ai_canonname = %s\n", AddrInfo->ai_canonname);
        if (AddrInfo->ai_addr != NULL) {
            if (AddrInfo->ai_addr->sa_family == AF_INET) {
                struct sockaddr_in *sin;

                sin = (struct sockaddr_in *)AddrInfo->ai_addr;
                printf("AddrInfo->ai_addr->sin_family = %u\n",
                       sin->sin_family);
                printf("AddrInfo->ai_addr->sin_port = %u\n",
                       ntohs(sin->sin_port));
                printf("AddrInfo->ai_addr->sin_addr = %s\n",
                       inet_ntoa(sin->sin_addr));

            } else if (AddrInfo->ai_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6;

                sin6 = (struct sockaddr_in6 *)AddrInfo->ai_addr;
                printf("AddrInfo->ai_addr->sin6_family = %u\n",
                       sin6->sin6_family);
                printf("AddrInfo->ai_addr->sin6_port = %u\n",
                       ntohs(sin6->sin6_port));
                printf("AddrInfo->ai_addr->sin6_flowinfo = %u\n",
                       sin6->sin6_flowinfo);
                printf("AddrInfo->ai_addr->sin6_scope_id = %u\n",
                       sin6->sin6_scope_id);
                printf("AddrInfo->ai_addr->sin6_addr = %s\n",
                       inet6_ntoa(&sin6->sin6_addr));

            } else {
                printf("AddrInfo->ai_addr->sa_family = %u\n",
                       AddrInfo->ai_addr->sa_family);
            }
        } else {
            printf("AddrInfo->ai_addr = (null)\n");
        }
    }

    printf("AddrInfo = (null)\n");
}


//* Test_getaddrinfo - Test getaddrinfo.
//
//  Note that getaddrinfo returns an error value,
//  instead of setting last error.
//
int
Test_getaddrinfo(int argc, char **argv)
{
    char *NodeName, *TestName, *ServiceName;
    int ReturnValue;
    struct addrinfo Hints, *AddrInfo;

    if (argc < 2)
        NodeName = "localhost";
    else
        NodeName = argv[1];

    printf("\ngetaddrinfo (nodename = %s):\n\n", NodeName);

    // Test with reasonable input:
    TestName = NULL;
    ServiceName = "ftp";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_flags = AI_PASSIVE;
    Hints.ai_socktype = SOCK_STREAM;
    ReturnValue = getaddrinfo(TestName, ServiceName, &Hints, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    TestName = NodeName;
    ServiceName = "smtp";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_CANONNAME;
    ReturnValue = getaddrinfo(TestName, ServiceName, &Hints, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    TestName = "dead:beef:cafe:babe:0123:4567:89ab:cdef";
    ServiceName = "69";
    memset(&Hints, 0, sizeof Hints);
    ReturnValue = getaddrinfo(TestName, ServiceName, NULL, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", NULL, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    TestName = "3/fe80::0123:4567:89ab:cdef";
    ServiceName = "telnet";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_flags = AI_NUMERICHOST;
    ReturnValue = getaddrinfo(TestName, ServiceName, NULL, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", NULL, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    TestName = "157.55.254.211";
    ServiceName = "exec";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    Hints.ai_family = PF_INET;
    ReturnValue = getaddrinfo(TestName, ServiceName, &Hints, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    // Ask for a stream-only service on a datagram socket.
    TestName = NULL;
    ServiceName = "exec";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_flags = AI_PASSIVE;
    Hints.ai_socktype = SOCK_DGRAM;
    ReturnValue = getaddrinfo(TestName, ServiceName, &Hints, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    // Ask for a numeric-only lookup, but give an ascii name.
    TestName = NodeName;
    ServiceName = "pop3";
    memset(&Hints, 0, sizeof Hints);
    Hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    Hints.ai_family = PF_INET6;
    ReturnValue = getaddrinfo(TestName, ServiceName, &Hints, &AddrInfo);
    printf("getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
           TestName, ServiceName);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        DumpAddrInfo(AddrInfo);
        freeaddrinfo(AddrInfo);
    }
    printf("\n");

    return 0;
};


//* Test_getnameinfo - Test getnameinfo.
//
//  Note that getnameinfo returns an error value,
//  instead of setting last error.
//
int
Test_getnameinfo()
{
    int ReturnValue;
    char NodeName[NI_MAXHOST];
    char ServiceName[NI_MAXSERV];
    char Tiny[2];
    int Error;

    printf("\ngetnameinfo:\n\n");

    // Test with reasonable input:
    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v6SockAddr,
                              sizeof v6SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&v6SockAddr, "
           "sizeof v6SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, NI_DGRAM);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_DGRAM)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&LinkLocalSockAddr,
                              sizeof LinkLocalSockAddr, NodeName,
                              sizeof NodeName, ServiceName,
                              sizeof ServiceName, NI_NUMERICHOST);
    printf("getnameinfo((struct sockaddr *)&LinkLocalSockAddr, "
           "sizeof LinkLocalSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_NUMERICHOST)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&LinkLocalSockAddr,
                              sizeof LinkLocalSockAddr, NodeName,
                              sizeof NodeName, ServiceName,
                              sizeof ServiceName, NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&LinkLocalSockAddr, "
           "sizeof LinkLocalSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName,
                              NI_NUMERICHOST | NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, "
           "NI_NUMERICHOST | NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    // Try to shoehorn too much into too little.
    memset(Tiny, 0, sizeof Tiny);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, Tiny, sizeof Tiny,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, Tiny, sizeof Tiny, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "Tiny = %s\nServiceName = %s\n", ReturnValue,
           Tiny, ServiceName);
    printf("\n");

    memset(Tiny, 0, sizeof Tiny);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, Tiny, sizeof Tiny,
                              ServiceName, sizeof ServiceName, NI_NUMERICHOST);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, Tiny, sizeof Tiny, "
           "ServiceName, sizeof ServiceName, NI_NUMERICHOST)\nReturns %d\n"
           "Tiny = %s\nServiceName = %s\n", ReturnValue,
           Tiny, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(Tiny, 0, sizeof Tiny);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              Tiny, sizeof Tiny, 0);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "Tiny, sizeof Tiny, 0)\nReturns %d\n"
           "NodeName = %s\nTiny = %s\n", ReturnValue,
           NodeName, Tiny);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(Tiny, 0, sizeof Tiny);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              Tiny, sizeof Tiny, NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "Tiny, sizeof Tiny, NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nTiny = %s\n", ReturnValue,
           NodeName, Tiny);
    printf("\n");

    return 0;
};
