// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1999-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Lulea University of Technology contributed this code
// under the terms of the license agreement.
//

#ifndef V6INC_H    // structs and stuff that we should really get elsewhere 
#define V6INC_H

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#include "ip6.h"

#define IPPROTO_TCP             6
#define IPPROTO_UDP             17
#define IP6PROTO_IP4            4 // IP6PROTO_IPIP 
#define IP6PROTO_IP6            41

#define ntohs(x) (((x & 0xff00) >> 8 )|((x & 0x00ff) << 8)) // XXX 
#define htons ntohs  // XXX 
#define ntohl net_long  // XXX 
#define htonl net_long   // XXX 

#define SAME_ADDR6(a,b)  ((a).u.DWord[3] == (b).u.DWord[3] && \
			  (a).u.DWord[2] == (b).u.DWord[2] && \
			  (a).u.DWord[1] == (b).u.DWord[1] && \
			  (a).u.DWord[0] == (b).u.DWord[0])
//*** udp.h ***

struct udphdr {
	uint16    uh_sport;		// source port 
	uint16    uh_dport;		// destination port 
	int16	  uh_ulen;		// udp length 
	uint16    uh_sum;		// udp checksum 
};

//*** tcp.h ***

struct tcphdr {
	uint16    th_sport;		// source port 
	uint16    th_dport;		// destination port 
	uint32	  th_seq;		// sequence number 
	uint32	  th_ack;		// acknowledgement number 

// BYTE_ORDER == LITTLE_ENDIAN  
//  XXX ugly  
#if 1 

	uchar     th_x2:4,		// (unused) 
		  th_off:4;		// data offset 
#endif
#if 0
	uchar     th_off:4,		// data offset 
		  th_x2:4;		// (unused) 
#endif
	uchar     th_flags;
#define	TH_FIN	  0x01
#define	TH_SYN	  0x02
#define	TH_RST	  0x04
#define	TH_PUSH	  0x08
#define	TH_ACK	  0x10
#define	TH_URG	  0x20
	uint16    th_win;			// window 
	uint16    th_sum;			// checksum 
	uint16    th_urp;			// urgent pointer 
};

//** in.h and ip.h **

struct in_addr {
  uint32 s_addr;
};
struct ip {
#if 1 // BYTE_ORDER == LITTLE_ENDIAN 
	uchar     ip_hl:4,		// header length 
		  ip_v:4;		// version 
#endif
#if 0 // BYTE_ORDER == BIG_ENDIAN 
	uchar     ip_v:4,		// version 
		  ip_hl:4;		// header length 
#endif
	uchar     ip_tos;		// type of service 
	int16	  ip_len;		// total length 
	uint16    ip_id;		// identification 
	int16	  ip_off;		// fragment offset field 
#define	IP_DF 0x4000			// dont fragment flag 
#define	IP_MF 0x2000			// more fragments flag 
#define	IP_OFFMASK 0x1fff		// mask for fragmenting bits 
	uchar     ip_ttl;		// time to live 
	uchar     ip_p;			// protocol 
	uint16    ip_sum;		// checksum 
	struct	  in_addr ip_src, ip_dst; // source and dest address 
};

#endif //  V6INC_H 


