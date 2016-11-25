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

#ifndef V6COMP_H                // IPv6 header compression 
#define V6COMP_H

#define MAX_SLOTS_UDP 255       // XXX should be 65535 
#define MIN_SLOTS_UDP 3
#define MAX_SLOTS_TCP 255       
#define MIN_SLOTS_TCP 3

#define MAX_HEAD_SIZE 1000      
#define MIN_HEAD_SIZE 120       

#define F_MAX_TIME 255
// Upper bound on number of seconds between full headers (udp only) 

#define F_MAX_PERIOD 65535
// Largest number of compressed headers that may be sent without a refresh. 
// DEFAULT is 0, a special value which implies infinity. (udp only) 


typedef unsigned short uint16;
typedef short int16;
typedef unsigned long  uint32;
typedef int int32;


#define V6C_TSIZE 16            // default number of slots in (de)comp table 
#define V6C_HSIZE 200           // default max header size in (de)comp table 

#define V6C_FULL(c) ( ((uchar)(c)) & ((uchar)0x80) )
#define V6C_ISIPV6(c) ((((char)(c)) >> 4) == 6)

enum comprInfo  {ci_ctcp, ci_cudp, ci_regu, ci_full, ci_err};

struct v6stat {
    uint32      v6s_full_c;     // full headers sent 
    uint32      v6s_comp_c;     // compressed headers sent 
    uint32      v6s_unmod_c;    // packets that could not be compressed 
    uint32      v6s_bbefore_c;  // bytes before compression 
    uint32      v6s_bafter_c;   // bytes after compression 
    uint32      v6s_changes_c;  // ??? 
    uint32      v6s_unknown_c;  // unknown headers 
    uint32      v6s_full_d;     // full headers received 
    uint32      v6s_comp_d;     // compressed headers received 
    uint32      v6s_unmod_d;    // packets that could not be decomressed 
    uint32      v6s_dropped_d;  // compressed headers dropped 
};

struct compState;

struct ifpppv6ccfg {
    uint16      len_send_t;
    uint16      len_recv_t; 
    uint16      len_send_u; 
    uint16      len_recv_u;
    uint16      max_header;
    uint16      max_period;
    uchar       max_time;
    uchar       reordering;
    uchar       cid_comp;       // XXX not used 
};

struct compStateList {
    struct compState *send_t;   // tcp headers send 
    struct compState *recv_t;   // tcp headers recv 
    struct compState *send_u;   // udp headers send 
    struct compState *recv_u;   // udp headers recv 
    struct compState *free_t;   // free tcp headers 
    struct compState *free_u;   // free udp headers 
    struct compState *first;    // sorted list 
    struct ifpppv6ccfg cfg;
    struct v6stat cstat;
};


//
//  print stats
//

void 
v6c_print_stat(struct compStateList *);

// 0 = ok; error in args == -1; could not allocate == -2 
int v6c_init(struct compStateList *, struct ifpppv6ccfg *);

enum comprInfo
v6c_comp(struct compStateList *,
         uchar *,               // pointer to where the ip header starts 
         int*,                  // -> pkt len  <- pkt hdr len 
         char *,                // pointer to the compresses header 
         int *);                // new header length 
                

//
// -1=fail 
//

int
v6c_decompf(struct compStateList *,
            struct mbuf *, 
            char *,             // pointer to where the ip header starts 
            uint16,             // tot length (not including ppp header) 
            int);               // max buflen 
            


//
//   1 = ok
//   0 = failed
//  -1 = error   
//

int  
v6c_decomp(struct compStateList *,
           uchar *,             // pointer to where the ip header starts 
           int,                 // ip pkt len 
           int,                 // =1 if tcp flow  
           uchar **,            // pointer to the decompressed header 
           int *);		// length of the decompressed header 
	   
 
#endif //  V6COMP_H 
















