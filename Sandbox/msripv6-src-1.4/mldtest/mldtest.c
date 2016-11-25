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
// Performs socket calls to add & drop multicast groups.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <stdio.h>
#include <stdlib.h>

int one = 1;
int GetAddress(char *astr, struct in6_addr *address);
void SendBSD(struct in6_addr Group, uint Port);
void ReceiveBSD(struct in6_addr Group, uint Port);
void SendWSA(struct in6_addr Group, uint Port);
void ReceiveWSA(struct in6_addr Group, uint Port);



#define BUFSIZE 1600
#define SEND_INTERVAL 2000      // In milliseconds, i.e. 2 seconds.
#define NUM_OF_PACKETS 5000     // Number of packets to send.

//
// Option values.
//
int Loopback = -1;
int HopLimit = -1;
int UseWinsockStyle = FALSE;
int Interface = 0;
int Sender;

void
usage(void)
{
    printf("usage: mldtest [options] s|r group-addr port\n"
           "       s = act as sender, r = act as receiver\n"
           "       -w           Use WSAJoinLeaf etc instead of usual API.\n"
           "       -l on|off    Control loopback when sending.\n"
           "       -h <num>     Specify a hop limit when sending.\n"
           "       -i <num>     Specify an interface index.\n");
    exit(1);
}


int __cdecl main(int argc, char **argv)
{
    WSADATA WsaData;
    int error;
    int i;
    struct in6_addr GroupAddr;
    uint Port;

    // Initialize winsock.
    error = WSAStartup( MAKEWORD(2, 0), &WsaData );
    if ( error ) {
        printf("mldtest: WSAStartup failed %ld:", WSAGetLastError());
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-w"))
            UseWinsockStyle = TRUE;
        else if (!strcmp(argv[i], "-l")) {
            if (++i == argc)
                usage();
            if (!strcmp(argv[i], "on"))
                Loopback = TRUE;
            else if (!strcmp(argv[i], "off"))
                Loopback = FALSE;
            else
                usage();
        }
        else if (!strcmp(argv[i], "-h")) {
            if (++i == argc)
                usage();
            HopLimit = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-i")) {
            if (++i == argc)
                usage();
            Interface = atoi(argv[i]);
        }
        else
            break;
    }

    if (argc - i != 3)
        usage();

    if (!strcmp(argv[i], "s"))
        Sender = TRUE;
    else if (!strcmp(argv[i], "r"))
        Sender = FALSE;
    else
        usage();

    //
    // Read the comand-line address.
    //
    if (GetAddress(argv[i+1], &GroupAddr) == FALSE) {
        printf("bad address supplied!\n");
        usage();
    }

    //
    // Read the port number.
    //
    Port = atoi(argv[i+2]);

    if (UseWinsockStyle) {
        if (Sender)
            SendWSA(GroupAddr, Port);
        else
            ReceiveWSA(GroupAddr, Port);
    }
    else {
        if (Sender)
            SendBSD(GroupAddr, Port);
        else
            ReceiveBSD(GroupAddr, Port);
    }

    WSACleanup();
    return 0;
}



int
GetAddress(char *astr, struct in6_addr *address)
{
    struct sockaddr_in6 sin;
    int addrlen = sizeof sin;

    sin.sin6_family = AF_INET; // Shouldn't be required but is.

    if ((WSAStringToAddress(astr, AF_INET6, NULL,
                           (struct sockaddr *)&sin, &addrlen)
                    == SOCKET_ERROR) ||
        (sin.sin6_port != 0))
        return FALSE;

    // The user gave us a numeric IP address.

    memcpy(address, &sin.sin6_addr, sizeof *address);
    return TRUE;
}



// Send data using a bsd-style socket.
void SendBSD(struct in6_addr Group, uint Port)
{
    int error, i;
    SOCKET bsdsend;
    struct sockaddr_in6 SA;
    char sendbuf[10];

    // Send some data to the group.
    bsdsend = socket(AF_INET6, SOCK_DGRAM, 0);
    if (bsdsend == INVALID_SOCKET) {
        printf("socket() failed: %ld\n", GetLastError( ) );
        exit(1);    
    }

    if (setsockopt(bsdsend, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
                   sizeof(one))) {
        printf("setsockopt(SO_REUSEADDR) failed: %u\n", WSAGetLastError());
        exit(1);
    }

    // Bind the socket to a local address & port.
    memset(&SA, 0, sizeof SA);
    SA.sin6_family = AF_INET6;
    if (bind(bsdsend, (struct sockaddr *)&SA, sizeof(SA))) {
        printf("bind failed: %d\n",WSAGetLastError());
        exit(1);
    }


    SA.sin6_addr = Group;
    SA.sin6_port = htons((ushort)Port);
    error = connect(bsdsend, (struct sockaddr *)&SA, sizeof(SA));
    if (error) {
        printf("connect() failed: %d\n",WSAGetLastError());
        exit(1);
    }

    
    if (Interface != 0) {
        //
        // Set the outgoing interface.
        //
        error = setsockopt(bsdsend, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                           (char *)&Interface, sizeof Interface);
        if (error) {
            printf("setsockopt(IPV6_MULTICAST_IF) failed: %u\n",
                   WSAGetLastError());
            exit(1);
        }
    }

    if (HopLimit != -1) {
        //
        // Set a hop limit.
        //
        error = setsockopt(bsdsend, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                           (char *)&HopLimit, sizeof HopLimit);
        if (error) {
            printf("setsockopt(IPV6_MULTICAST_HOPS) failed: %u\n",
                   WSAGetLastError());
            exit(1);
        }
    }

    if (Loopback != -1) {
        //
        // Set loopback.
        //
        error = setsockopt(bsdsend, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                           (char *)&Loopback, sizeof Loopback);
        if (error) {
            printf("setsockopt(IPV6_MULTICAST_LOOP) failed: %u\n",
                   WSAGetLastError());
            exit(1);
        }
    }


    for (i = 0; i < NUM_OF_PACKETS; i++) {
        sprintf(sendbuf, "%4d-bsd", i);
        send(bsdsend, (const char FAR *)sendbuf, 10, 0);
        Sleep(SEND_INTERVAL); 
    }

    closesocket(bsdsend);
}


void ReceiveBSD(struct in6_addr Group, uint Port)
{
    int error, i, bytes;
    char buffer[BUFSIZE];
    SOCKET bsd;
    struct ipv6_mreq Query;
    struct sockaddr_in6 SA, RSA;
    int FromLen;


    // Open a socket to use.
    bsd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (bsd == INVALID_SOCKET) {
        printf("socket() failed: %ld\n", GetLastError( ) );
        exit(1);    
    }

    if (setsockopt(bsd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one))) {
        printf("setsockopt(SO_REUSEADDR) failed: %u\n", WSAGetLastError());
        exit(1);
    }

    // Bind the socket to the port.
    memset(&SA, 0, sizeof SA);
    SA.sin6_port = htons((ushort)Port);
    SA.sin6_family = AF_INET6;
    if (bind(bsd, (struct sockaddr *)&SA, sizeof(SA))) {
        printf("bind failed: %d\n",WSAGetLastError());
        exit(1);
    }

    // Notify IP of our interest in this group.
    Query.ipv6mr_multiaddr = Group;
    Query.ipv6mr_interface = Interface;
    error = setsockopt(bsd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                       (char *)&Query, sizeof Query);
    if (error) {
        printf("setsockopt(IPV6_ADD_MEMBERSHIP) failed: %u\n",
               WSAGetLastError());
        exit(1);
    }

    // Read some data from the socket.
    for (i = 0; i < 1000; i++) {
        FromLen = sizeof(RSA);
        bytes = recvfrom(bsd, buffer, BUFSIZE, 0, (struct sockaddr *)&RSA,
                         &FromLen);
        if (bytes != SOCKET_ERROR) {
            buffer[bytes] = '\0';
            printf("%d, %s\n",i,buffer);
        } else {
           error = WSAGetLastError();
           printf("receiveBSD: recv error %d\n",error);
        }
    }
    
    // OK we're done.
    closesocket(bsd);
    return;
}

void SendWSA(struct in6_addr Group, uint Port)
{
    int error, i;
    char buffer[BUFSIZE];
    struct sockaddr_in6 SA;
    DWORD  bytesout;
    int WSAProtoIds[] = { IPPROTO_UDP, 0};
    LPWSAPROTOCOL_INFO ProtoInfo;
    DWORD Bufsize = BUFSIZE;
    DWORD WSAFlags;
    SOCKET wsa, wsasend;
    WSABUF CalleeData;
    char sendbuf[10];

    // Get the protocol info structure for UDP service.
    error = WSAEnumProtocols(WSAProtoIds, (LPWSAPROTOCOL_INFO) buffer,
                             &Bufsize);
    if (error == SOCKET_ERROR) {
        printf("WSAEnumProtocols() failed: %d\n",WSAGetLastError());
        exit(1);
    }
    ProtoInfo = (LPWSAPROTOCOL_INFO) buffer;

    // Open a WSA socket.
    WSAFlags = WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF;
    wsa = WSASocket(AF_INET6, SOCK_DGRAM, 0, ProtoInfo, 0, WSAFlags);
    if (wsa == INVALID_SOCKET) {
        printf("WSASocket() failed: %d\n",WSAGetLastError());
        exit(1);
    }

    if (setsockopt(wsa, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one))) {
        printf("setsockopt(SO_REUSEADDR) failed: %u\n", WSAGetLastError());
        exit(1);
    }

    // Join the multicast group.
    memset(&SA, 0, sizeof SA);
    SA.sin6_family = AF_INET6;
    SA.sin6_addr = Group;
    WSAHtons(wsa, (ushort)Port, &SA.sin6_port);
    WSAFlags = JL_SENDER_ONLY;
    CalleeData.len = 0;
    CalleeData.buf = NULL;
    wsasend = WSAJoinLeaf(wsa, (struct sockaddr FAR *)&SA, sizeof(SA), NULL,
                          &CalleeData, NULL, NULL, WSAFlags);
    if (wsasend == INVALID_SOCKET) {
        printf("WSAJoinLeaf() failed: %d\n",WSAGetLastError());
        exit(1);
    }

    // Connect to group address for sending.
    error = WSAConnect ( wsasend, (struct sockaddr FAR *)&SA, sizeof(SA),
                         NULL, &CalleeData, NULL, NULL);
    if (error == SOCKET_ERROR) {
        printf("WSAConnect() failed: %d\n",WSAGetLastError());
        exit(1);
    }
 
    if (HopLimit != -1) {
        // Set the outbound multicast default TTL.
        error = WSAIoctl(wsasend, SIO_MULTICAST_SCOPE,
                         (LPVOID)&HopLimit, sizeof HopLimit,
                         (LPVOID) buffer, BUFSIZE,
                         (LPDWORD) &bytesout, NULL, NULL);
        if (error == SOCKET_ERROR) {
            printf("WSAIoctl() failed: %d\n",WSAGetLastError());
            exit(1);
        }
    }

    // Send some data.
    for (i = 0; i < 1000; i++) {
        sprintf(sendbuf, "%4d wsa", i);
        send(wsasend, (const char FAR *)sendbuf, 10, 0);
        Sleep(250); 
    }

    // OK we're done.
    closesocket(wsa);
    closesocket(wsasend);
}

void ReceiveWSA(struct in6_addr Group, uint Port)
{
    int error, i, status;
    char buffer[BUFSIZE];
    struct in6_addr GroupAddr;
    struct sockaddr_in6 SA, RSA;
    int FromLen;
    DWORD  bytesout;

    int WSAProtoIds[] = {IPPROTO_UDP, 0};
    LPWSAPROTOCOL_INFO ProtoInfo;
    DWORD Bufsize = BUFSIZE;
    DWORD WSAFlags;
    SOCKET wsa, wsatrash;
    WSABUF CalleeData;

    // Get the protocol info structure for UDP service.
    error = WSAEnumProtocols(WSAProtoIds, (LPWSAPROTOCOL_INFO) buffer,
                             &Bufsize);
    if (error == SOCKET_ERROR) {
        printf("WSAEnumProtocols() failed: %d\n",WSAGetLastError());
        exit(1);
    }
    ProtoInfo = (LPWSAPROTOCOL_INFO) buffer;

    // Open a WSA socket.
    WSAFlags = WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF;
    wsa = WSASocket(AF_INET6, SOCK_DGRAM, 0, ProtoInfo, 0, WSAFlags);
    if (wsa == INVALID_SOCKET) {
        printf("WSASocket() failed: %d\n",WSAGetLastError());
        exit(1);
    }

    if (setsockopt(wsa, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one))) {
        printf("setsockopt(SO_REUSEADDR) failed: %u\n", WSAGetLastError());
        exit(1);
    }

    // Bind the socket to the multicast address & port.
    memset(&SA, 0, sizeof SA);
    SA.sin6_family = AF_INET6;
    SA.sin6_addr = Group;
    WSAHtons(wsa, (ushort)Port, &SA.sin6_port);
    if (bind(wsa, (struct sockaddr *)&SA, sizeof(SA))) {
        // bind to group address failed, try generic address.
        SA.sin6_addr = in6addr_any;
        if (bind(wsa, (struct sockaddr *)&SA, sizeof(SA))) {
            printf("bind failed: %d\n",WSAGetLastError());
            exit(1);
        }
    }

    // Join the multicast group.
    SA.sin6_addr = Group;
    WSAFlags = JL_RECEIVER_ONLY;
    CalleeData.len = 0;
    CalleeData.buf = NULL;
    wsatrash = WSAJoinLeaf(wsa, (struct sockaddr FAR *)&SA, sizeof(SA),
                           NULL, &CalleeData, NULL, NULL, WSAFlags);
    if (wsatrash == INVALID_SOCKET) {
        printf("WSAJoinLeaf() failed: %d\n",WSAGetLastError());
        exit(1);
    }

    // Read some data from the socket.
    for (i = 0; i < 1000; i++) {
        FromLen = sizeof(RSA);
        CalleeData.len = BUFSIZE;
        CalleeData.buf = buffer;
        status = WSARecvFrom (wsa, &CalleeData, 1, &Bufsize, &WSAFlags,
                              (struct sockaddr *)&RSA, &FromLen, NULL, NULL);
        if (status != SOCKET_ERROR) {
            buffer[Bufsize] = '\0';
            printf("%d, %s\n", i, buffer);
        } else {
           error = WSAGetLastError();
           printf("receiveBSD: recv error %d\n", error);
        }
    }
    
    // OK we're done.
    closesocket(wsa);
    closesocket(wsatrash);
}
