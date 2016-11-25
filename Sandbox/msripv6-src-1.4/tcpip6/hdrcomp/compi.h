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

#ifndef V6COMPI_H		// IPv6 header compression (interal use) 
#define V6COMPI_H

#include "comp.h"
#include "inc.h"
#include "ip6.h"


#define eq_ports(x,y)   ((*(int*)x) == (*(int*)y))
#define ip6_ip_v ip4.ip_v


#define VERSION 1		// compresion scheme  

#define REFRESH_REQUESTS 0	// are State Refresh Request messages enabled? 
				// 0 State Refresh Request messages 
				// must not be sent 
				// 1 (DEFAULT) State Refresh Request 
				// messages may be sent. 


#define F_CHANGE 1		// Number of compressed headers
				// between refreshes after a header
				// has changed. 

#define NILIDX 0xffff           // nil for index 
#define ESP_SIZE 8              // esp header size in bytes 


#define V6C_UDP_FULL4         0x8
#define V6C_UDP_FULL4REQ      0x9
#define V6C_UDP_FULL6         0xA
#define V6C_UDP_FULL6REQ      0xB

#define V6C_TCP_FULL4         0xC
#define V6C_TCP_FULL4REQ      0xD
#define V6C_TCP_FULL6         0xE
#define V6C_TCP_FULL6REQ      0xF


// XXX Authentication Header should be in ip6.h 
struct ip6_auth {
    uchar au_next_hdr;		// next header 
    uchar au_data_len;		// authentication data length 
    uint16 au_reserved;		// reserved 
    uint32 au_id;               // security association id 
};

union ipHead {
    struct ip  ip4;               
    struct IPv6Header ip6;               
    struct udphdr udp;
    struct tcphdr tcp;
    struct ip6_auth auth;	// XXX no auth support in netBSD yet 
};
 
struct compState {
    union ipHead *ipHead;
    struct compState *next;	// xmit only 
    uint16   tcp_cksum;		// pseudo cksum, not including length 
    uint16 len;
    uint16 cid;
    uint16 c_num;		// xmit udp only exp backoff 
    uint16 c_max;		// xmit udp only exp backoff   
#define tcp_delta c_num
#define tcp_payload  c_max
    uchar gen;
    uchar isTcp;		// these should be merged into a single char
    uchar head_req;
    uchar inUse;
};

struct compHead {
    uchar cid;
    uchar fg;			// flags/generation 
};

#define COMPH_SIZE 2

// ----- for tcp, most from VJ's rfc 1144   ------- 
//        
//  ENCODE encodes a number that is known to be non-zero.  ENCODEZ checks for
//  zero (zero has to be encoded in the long, 3 byte form).
//

#define ENCODE(n) {                                  \
     if ((uint16)(n) >= 256) {                       \
          *rdp++ = 0;                                \
          rdp[1] = (uchar)(n);                       \
          rdp[0] = (uchar)((n) >> 8);                \
          rdp += 2;                                  \
     } else {                                        \
          *rdp++ = (uchar)(n);                       \
     }                                               \
 }

#define ENCODEZ(n) {                                 \
     if ((uint16)(n) >= 256 || (uint16)(n) == 0) {   \
          *rdp++ = 0;                                \
          rdp[1] = (uchar)(n);                       \
          rdp[0] = (uchar)((n) >> 8);                \
          rdp += 2;                                  \
     } else {                                        \
          *rdp++ = (uchar)(n);                       \
     }                                               \
 } 

//
// DECODEL takes the (compressed) change at byte rdp and adds it to the
// current value of packet field 'f' (which must be a 4-byte (long) integer
// in network byte order).  DECODES does the same for a 2-byte (short) field.
// DECODEU takes the change at rdp and stuffs it into the (short) field f.
// 'rdp' is updated to point to the next field in the compressed header.
//

#define DECODEL(f) {                                        \
     if (*rdp == 0) {                                       \
          (f) = htonl(ntohl(f) + ((rdp[1] << 8) | rdp[2])); \
          rdp += 3;                                         \
     } else {                                               \
          (f) = htonl(ntohl(f) + (uint32)*rdp++);           \
     }                                                      \
 }

#define DECODES(f) {                                        \
     if (*rdp == 0) {                                       \
          (f) = htons(ntohs(f) + ((rdp[1] << 8) | rdp[2])); \
          rdp += 3;                                         \
     } else {                                               \
          (f) = htons(ntohs(f) + (uint16)((uint32)*rdp++)); \
     }                                                      \
 }

#define DECODEU(f) {                                        \
     if (*rdp == 0) {                                       \
          (f) = htons((rdp[1] << 8) | rdp[2]);              \
          rdp += 3;                                         \
     } else {                                               \
          (f) = htons((uint16)((uint32)*rdp++));            \
     }                                                      \
 }         
   

#define NEW_O          0x40	// new options 
#define NEW_I          0x20
#define TCP_PUSH_BIT   0x10

#define NEW_S          0x08 
#define NEW_A          0x04
#define NEW_W          0x02
#define NEW_U          0x01

// reserved, special-case values of above 
#define SPECIAL_I (NEW_S|NEW_W|NEW_U)        // echoed interactive traffic   
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U)  // unidirectional data  
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)


// ------ misc defines for decomp and comp --------- 

#define LEN_IP6  0xC000         // 1100 0000 0000 0000 
#define LEN_IP4  0x8000         // 1000 0000 0000 0000 
#define LEN_UDP  0x4000         // 0100 0000 0000 0000 

#define LEN_TYPE(x) ( (x) &  0xC000 )
#define LEN_OFFS(x) ( (x) & ~0xC000 )
#define LEN_MAX 6               // size of length stack 

#define GEN_IP4(ip)  (((uchar*)(ip))[2])
#define CID_IP4(ip)  (((uchar*)(ip))[3])
#define GEN_IP6(ip)  (((uchar*)(ip))[4])
#define CID_IP6(ip)  (((uchar*)(ip))[5])

#define INC(ptr,len) (((char*)ptr) += (len))
#define OPTLEN(ptr) (((((struct IPv6OptionsHeader*)ptr)->HeaderExtLength)+1)*8)
#define NEXTOP(ptr) (((struct IPv6OptionsHeader*)ptr)->NextHeader)

// trace and stuff 
// NB: These macros only work in a checked build,
// because they assume KdPrint(x) expands to an expression.

#if DBG

#define TP(x) KdPrint(x)
#define ERROR(x)        ass(x)
#define ass(x)          x?0:KdPrint(("*ASS FAILED* %s:%d\n",__FILE__,__LINE__))
#define CTP(x,y)        x?KdPrint(y):0
#define INC_ST(x)      ((x)++)
#define INC_ST2(x,y)   ((x) += (y) )

#else

#define TP(x) 
#define ERROR(x) TPERR(x)
#define ass(x) 
#define CTP(x,y)
#define INC_ST(x)      ((x)++)
#define INC_ST2(x,y)   ((x) += (y) )

#endif

#if DBG

#define TPC(x)  	CTP((kkk & 0x0010) ,x)	// compression 
#define TPI(x)  	CTP((kkk & 0x0020) ,x)	// init 
#define TPCI(x) 	CTP((kkk & 0x0040) ,x)	// cid 
#define TPE(x)  	CTP((kkk & 0x0080) ,x)	// eqhead 
		    
#define TPDF(x) 	CTP((kkk & 0x0004) ,x)	// decomp full 
#define TPD(x)  	CTP((kkk & 0x0008) ,x)	// decomp 
#define TPERR(x)	CTP((kkk & 0x0001) ,x)	// Error 

#else

#define TPC(x)			// compression 
#define TPI(x)			// init 
#define TPCI(x)			// cid 
#define TPE(x)			// eqhead 

#define TPDF(x)			// decomp full 
#define TPD(x)			// decomp 
#define TPERR(x)		// error

#endif 

extern uint32 kkk;

#endif //  V6COMPI_H 










