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
// This is derived from the VC++ ping program sample code.
//
// Confirms reachability to the specified destination IP address.
// If it is reachable, returns the source address
// that is used to reach the destination address.
// Otherwise returns the unspecified (zero) address.
//
// We do this by pinging the destination IP address.
// Unfortunately the IcmpSendEcho API does not return
// the source address that was used, so we use raw sockets.
//

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

#define ICMP_MIN 8  // Minimum 8 byte icmp packet (just header).

//
// The IP header.
//
typedef struct iphdr {
    unsigned char verlen;
    unsigned char tos;             // Type of service.
    unsigned short total_len;      // Total length of the packet.
    unsigned short ident;          // Unique identifier.
    unsigned short frag_and_flags; // Flags.
    unsigned char  ttl; 
    unsigned char proto;           // Protocol (TCP, UDP etc).
    unsigned short checksum;       // IP checksum.

    unsigned int sourceIP;
    unsigned int destIP;
} IpHeader;

//
// ICMP header.
//
typedef struct _ihdr {
    BYTE i_type;
    BYTE i_code;     // Type sub code.
    USHORT i_cksum;
    USHORT i_id;
    USHORT i_seq;
} IcmpHeader;

USHORT checksum(USHORT *buffer, int size)
{
  unsigned long cksum = 0;

  while (size > 1) {
      cksum += *buffer++;
      size -= sizeof(USHORT);
  }
  
  if (size) {
      cksum += *(UCHAR*)buffer;
  }

  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >> 16);
  return (USHORT)(~cksum);
}


int
ConfirmIPv4Reachability(IN_ADDR Dest, unsigned int Timeout, IN_ADDR *Source)
{
    struct timeval tv;
    SOCKET s;
    struct sockaddr_in sinDest;
    struct sockaddr_in sinFrom;
    int fromlen;
    FD_SET Fds;
    struct {
        IcmpHeader IcmpH;
    } Send;
    struct {
        IpHeader IpH;
        IcmpHeader IcmpH;
        char Data[1024];
    } Recv;

    s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s == INVALID_SOCKET)
        return FALSE;

    memset(&sinDest, 0, sizeof sinDest);
    sinDest.sin_family = AF_INET;
    sinDest.sin_addr = Dest;

    //
    // Initialize i_id and i_seq with the current process ID.
    // We check that any replies match up properly.
    //
    Send.IcmpH.i_type = ICMP_ECHO;
    Send.IcmpH.i_code = 0;
    *(ULONG *)&Send.IcmpH.i_id = GetCurrentProcessId();
    Send.IcmpH.i_cksum = 0;
    Send.IcmpH.i_cksum = checksum((USHORT *)&Send, sizeof Send);

    //
    // Send the ICMP Echo message.
    //
    if (sendto(s, (char *)&Send, sizeof Send, 0,
               (struct sockaddr *)&sinDest, sizeof sinDest) != sizeof Send) {
        closesocket(s);
        return FALSE;
    }

    //
    // Use select to wait (with a timeout) for a reply.
    //
    Fds.fd_count = 1;
    Fds.fd_array[0] = s;
    tv.tv_sec = Timeout/1000;
    tv.tv_usec = (Timeout%1000)*1000;
    if (select(1, &Fds, (FD_SET *)0, (FD_SET *)0, &tv) != 1) {
        closesocket(s);
        return FALSE;
    }

    //
    // Receive the reply.
    //
    fromlen = sizeof sinFrom;
    if (recvfrom(s, (char *)&Recv, sizeof Recv, 0,
                 (struct sockaddr *)&sinFrom, &fromlen)
                                < sizeof(IpHeader) + sizeof(IcmpHeader)) {
        closesocket(s);
        return FALSE;
    }

    //
    // Check that the reply is actually for us.
    //
    if ((Recv.IcmpH.i_type != ICMP_ECHOREPLY) ||
        (Recv.IcmpH.i_id != Send.IcmpH.i_id) ||
        (Recv.IcmpH.i_seq != Send.IcmpH.i_seq)) {
        closesocket(s);
        return FALSE;
    }

    //
    // We received a reply!
    // Return the destination of the ICMP Echo Reply.
    // This is an address assigned to this machine.
    //
    Source->s_addr = Recv.IpH.destIP;
    closesocket(s);
    return TRUE;
}
