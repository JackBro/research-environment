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
// Abstract:
//
// IPv6 header compression.
// Init and compression routines.
// 
//


#include "oscfg.h"
#include "ndis.h"
#include "ntddk.h"
#include "comp.h"
#include "compi.h"

//* v6c_print_stat
//
//  Prints the compression stats.
//

void
v6c_print_stat(struct compStateList *csList)
{
    KdPrint(("\nv6s_full_c \t %d \n",    csList->cstat.v6s_full_c));
    KdPrint(("v6s_comp_c \t %d \n",      csList->cstat.v6s_comp_c));
    KdPrint(("v6s_unmod_c \t %d \n",     csList->cstat.v6s_unmod_c));
    KdPrint(("v6s_bbefore_c \t %d \n",   csList->cstat.v6s_bbefore_c));
    KdPrint(("v6s_bafter_c \t %d \n",    csList->cstat.v6s_bafter_c));
    KdPrint(("v6s_changes_c \t %d \n",   csList->cstat.v6s_changes_c));
    KdPrint(("v6s_unknown_c \t %d \n",   csList->cstat.v6s_unknown_c));
    KdPrint(("v6s_full_d \t %d \n",      csList->cstat.v6s_full_d));
    KdPrint(("v6s_comp_d \t %d \n",      csList->cstat.v6s_comp_d));
    KdPrint(("v6s_unmod_d \t %d \n",     csList->cstat.v6s_unmod_d));
    KdPrint(("v6s_dropped_d \t %d \n\n", csList->cstat.v6s_dropped_d));
}

// XXX Unix stuff that should be changed to windows dito,
// XXX but I do not know the function names at the moment.
	      
static void bzero(void *c, int size)
{
    int i;
    for(i=0;i<size;i++)
        ((uchar *)c)[i] =0;
}

static void bcopy(void *from, void *to, int size)
{
    int i;
    for(i=0;i<size;i++)
        ((uchar*)to)[i] = ((uchar*)from)[i];
}

static int bcmp(void *a, void *b, int len)
{
    int i;
    for(i=0;i<len;i++) {
        if ( ((uchar*)a)[i] != ((uchar*)b)[i] )
            return 1;
    }
    return 0;
}

// XXX end of Unix stuff 

//* v6c_clear_stat
//
//  Resets the compression stats. 
//

static void
v6c_clear_stat(struct compStateList *csList)
{
    bzero(&csList->cstat, sizeof(csList->cstat));
}

#define FREE_HEAD(p,l)          \
l = 0;                          \
if (p != NULL) {                \
    if (p->ipHead != NULL)      \
      ExFreePool(p->ipHead);    \
    ExFreePool(p);              \
} 

//* free_all
//
//  Frees up the all memory used for compression.
//

static void
free_all(struct compStateList *csList)
{
    TPI(("free_all: start\n"));
    FREE_HEAD(csList->send_t, csList->cfg.len_send_t);
    FREE_HEAD(csList->send_u, csList->cfg.len_send_u);
    FREE_HEAD(csList->recv_t, csList->cfg.len_recv_t);
    FREE_HEAD(csList->recv_u, csList->cfg.len_recv_u);
    TPI(("free_all: done\n"));
}

#define MA(ptr, type, size)   ptr = (type)ExAllocatePool(NonPagedPool, size)

#define MALL(p,l)                                                     \
          MA(p, struct compState*, l * sizeof(struct compState) );    \
          if ( p == NULL ) { free_all(csList); return(-2); }          \
          MA(p->ipHead, union ipHead*, l * cfg->max_header);          \
          if ( p->ipHead == NULL ) { free_all(csList); return(-2); }  \
          TPI(("v6c_init: malloc %x %d \n", (unsigned)p, l));
          
#define set_send(p,l,hs,is_tcp)                                     \
for (i=0; i < l; i++) {                                             \
    p[i].len = 0;                                                   \
    p[i].cid = i;                                                   \
    p[i].gen = 0;                                                   \
    p[i].head_req = 0;                                              \
    p[i].ipHead = (union ipHead *)(((uchar *)p->ipHead) + i * hs);  \
    p[i].isTcp = is_tcp;                                            \
    p[i].next = i+1 == l ? NULL : &(p[i+1]);                        \
    }


#define set_recv(p,l,hs)                                            \
    for (i=0; i<l; i++) {                                           \
    p[i].len = 0;                                                   \
    p[i].cid = i;                                                   \
    p[i].gen = 0;                                                   \
    p[i].head_req = 0;                                              \
    p[i].ipHead = (union ipHead *)(((uchar *)p->ipHead) + i * hs);  \
}

//* v6c_init
//
//  Allocates memory and initializes the data structure.
//  XXX the memory allocation is wasting space. The mininmun 
//  amount of allocated space is one page. Therefor it would 
//  be better to allocate one big chunk instead of four minor 
//  ones.
//  0 = ok; error in args == -1; could not allocate == -2 
//
int
v6c_init(struct compStateList *csList, struct ifpppv6ccfg *cfg)
{
    int i;
  
    TPI(("v6c_init: start\n"));
    v6c_print_stat(csList);
    v6c_clear_stat(csList);
    if ( cfg == NULL ) {
      TPI(("v6c_init: cfg == NULL return -1\n"));
      return -1;
    }
    
    TPI(("v6c_init: len_send_t %d \n", cfg->len_send_t ));
    TPI(("v6c_init: len_recv_t %d \n", cfg->len_recv_t )); 
    TPI(("v6c_init: len_send_u %d \n", cfg->len_send_u )); 
    TPI(("v6c_init: len_recv_u %d \n", cfg->len_recv_u ));
    TPI(("v6c_init: max_header %d \n", cfg->max_header ));
    TPI(("v6c_init: max_period %d \n", cfg->max_period ));
    TPI(("v6c_init: max_time   %d \n", cfg->max_time   ));
    TPI(("v6c_init: reordering %d \n", cfg->reordering ));
    
    if ( cfg == NULL ||
         cfg->len_send_t > MAX_SLOTS_TCP ||
         cfg->len_recv_t > MAX_SLOTS_TCP ||
         cfg->len_send_u > MAX_SLOTS_UDP ||
         cfg->len_recv_u > MAX_SLOTS_UDP ||
         cfg->len_send_t < MIN_SLOTS_TCP ||
         cfg->len_recv_t < MIN_SLOTS_TCP ||
         cfg->len_send_u < MIN_SLOTS_UDP ||
         cfg->len_recv_u < MIN_SLOTS_UDP ||
         (cfg->max_header & 0x07) ||
         cfg->max_header > MAX_HEAD_SIZE ||
         cfg->max_header < MIN_HEAD_SIZE ||
         cfg->reordering  ) {   // not allowed, sorry
        TPI(("v6c_init: return -1\n"));
        return -1;
    }
    if (csList->send_t != NULL)	// free old space 
        free_all(csList);
    
    bcopy(cfg,&csList->cfg, sizeof(*cfg)); // copy cfg values 
    MALL(csList->send_t, csList->cfg.len_send_t);
    MALL(csList->send_u, csList->cfg.len_send_u);
    MALL(csList->recv_t, csList->cfg.len_recv_t);
    MALL(csList->recv_u, csList->cfg.len_recv_u);
    
    csList->first = NULL;
    csList->free_t = csList->send_t;
    csList->free_u = csList->send_u;
    
    set_send(csList->send_t, csList->cfg.len_send_t, cfg->max_header,1);
    set_send(csList->send_u, csList->cfg.len_send_u, cfg->max_header,0);
    set_recv(csList->recv_t, csList->cfg.len_recv_t, cfg->max_header);
    set_recv(csList->recv_u, csList->cfg.len_recv_u, cfg->max_header);
    
    TPI(( "v6c_init: init done.\n"));
    return 0;
}

//* newCid
//
//  Get a new cid of right type and place it first in the lru list.
//  (far from optimal :)
//

static struct compState* 
newCid(struct compStateList *csList, int isTcp)
{
    struct compState *tmp, *cs;
    int count;
    
    if ( isTcp ) {
        if ( csList->free_t != NULL ) {
            cs = csList->free_t;
            csList->free_t = cs->next;
            cs->next = csList->first;
            csList->first = cs;
            return cs;
        }
    } else {
        if ( csList->free_u != NULL ) {
            cs = csList->free_u;
            csList->free_u = cs->next;
            cs->next = csList->first;
            csList->first = cs;
            return cs;
        }
    }	
    
    // have to find the lru slot 
    count = isTcp ? csList->cfg.len_send_t : csList->cfg.len_send_u ;
    if ( csList->first->isTcp == isTcp )
        count--;
    for (  tmp = csList->first ; ; tmp = tmp->next ) 
        if ( tmp->next->isTcp == isTcp )
            if ( ! --count )
                break;
    
    // tmp->next is the lru slot
    cs = tmp->next;
    tmp->next = cs->next;
    cs->next = csList->first;
    csList->first = cs;
    return cs;
}

//* freeFirstCid
//
//  Take the first item on the lru list and place in a free list
//

static void 
freeFirstCid(struct compStateList *csList)
{
    struct compState *cs;
    
    cs = csList->first;
    csList->first = cs->next;
    if ( cs->isTcp ) {
        cs->next = csList->free_t;
        csList->free_t = cs;
    } else {
        cs->next = csList->free_u;
        csList->free_u = cs;
    }	
}

enum searchInfo { si_tcp, si_nontcp, si_quit, si_err };

#define LENTEST(l,s) if (((l) += (s)) > csList->cfg.max_header) return 1

//* eqHead    parameters: in head, table head, info 
// 
//  Compares a header with an existing state.
//  Returns 1 on success 0 otherwise.
//  Sets s_info to si_tcp or si_nontcp on success.
//  Sets s_info to si_quit if we know that this 
//  header should not be compressed.
//

static int 
eqHead(struct compStateList *csList,
       union ipHead *ih,
       union ipHead *th,
       enum searchInfo *s_info,
       union ipHead **iph_ptr)
{
    uint16 len = 0 ;             // accumulated header length
    uint16 i,t;
    
    TPE(("eqhead: ipv = %d\n",ih->ip4.ip_v));
    if ( ih->ip4.ip_v != th->ip4.ip_v )
	return 0;
    if ( ih->ip6_ip_v == 6 )	// we know that it is 4 or 6
	goto ip6;
    
ip4:
    TPE(("eqHead: ip4\n"));
    LENTEST(len,sizeof(struct ip));
    // fragments and options XXX
    if ( ih->ip4.ip_hl != 5 ) {
        TPE(("eqHead: ip4 ip4.ip_hl != %d \n",ih->ip4.ip_hl)) ;
        goto bail;
    }
    if ( ih->ip4.ip_off & htons(0x3FFF) ) {
        TPE(("eqHead: ip4 fragment\n")) ;
        goto bail;
    }
    if ( ih->ip4.ip_src.s_addr != th->ip4.ip_src.s_addr ||
         ih->ip4.ip_dst.s_addr  != th->ip4.ip_dst.s_addr ||
         ih->ip4.ip_p != th->ip4.ip_p ) 
        return 0;
    
    *iph_ptr = ih;                                // for tcp comp 
    i = ih->ip4.ip_p;
    INC(ih,sizeof(struct ip));
    INC(th,sizeof(struct ip));
    switch (i) {
    case IPPROTO_TCP:  goto tcp; 
    case IPPROTO_UDP:  goto udp; 
    case IP6PROTO_IP4: goto ip4;  
    case IP6PROTO_IP6: goto ip6; 
    }
    TPE(("eqHead: unknown proto %d in ip4\n",i));
    goto bail;
    
ip6:
    TPE(("eqHead: ip6\n"));
    LENTEST(len,sizeof(struct IPv6Header)); // XXX IPv6Header==(vers:prio:flowlabel) 
    if ( ih->ip6.VersClassFlow != th->ip6.VersClassFlow || 
         ! SAME_ADDR6(ih->ip6.Source, th->ip6.Source) ||
         ! SAME_ADDR6( ih->ip6.Dest, th->ip6.Dest))
        return 0;
    
    *iph_ptr = ih;                                // for tcp comp
    // IPv6 extension headers 
    i = ih->ip6.NextHeader;
    t = th->ip6.NextHeader;
    INC(ih,sizeof(struct IPv6Header));
    INC(th,sizeof(struct IPv6Header));
    
ip6h:
    TPE(("eqHead: ip6 options\n"));
    if (i != t) 
        return 0;
    switch(i) {
        // skip these 
    case IP_PROTOCOL_HOP_BY_HOP:
        if ( OPTLEN(ih) != OPTLEN(th) ) return 0; 
        LENTEST(len,OPTLEN(ih));
        i = NEXTOP(ih);
        t = NEXTOP(th);
        INC(ih,OPTLEN(ih));
        INC(th,OPTLEN(th));
        goto ip6h;
        
    case IP_PROTOCOL_ROUTING: 
        if ( OPTLEN(ih) != OPTLEN(th) ) 
            return 0; 
        if ( bcmp(ih,th,OPTLEN(ih))) 
            return 0; // DEF
        LENTEST(len,OPTLEN(ih));
        i = NEXTOP(ih);
        t = NEXTOP(th);
        INC(ih,OPTLEN(ih));
        INC(th,OPTLEN(th));
        goto ip6h;
        
    case IP_PROTOCOL_AH:           
        if ( OPTLEN(ih) != OPTLEN(th) ) 
            return 0; // XXX 
        if ( ih->auth.au_id != th->auth.au_id ) 
            return 0;
        LENTEST(len,OPTLEN(ih));
        i = NEXTOP(ih);
        t = NEXTOP(th);
        INC(ih,OPTLEN(ih));
        INC(th,OPTLEN(th));
        goto ip6h;
        
        // endpoints
    case IPPROTO_TCP:     goto tcp;
    case IPPROTO_UDP:     goto udp;
    case IP6PROTO_IP4:    goto ip4;
    case IP6PROTO_IP6:    goto ip6;
    case IP_PROTOCOL_ESP: goto esp;
        
        // bail on these
    case IP_PROTOCOL_FRAGMENT:
    case IP_PROTOCOL_NONE:	// uh?? XXX
    default:			// uh?? XXX
        TPE(("eqHead: unknown proto %d in ipv6\n",i));
        goto bail;
    }
    
esp:
    LENTEST(len,sizeof(uint32));
    if ( *(uint32*)ih == *(uint32*)th )
        return 1;
    return 0;
    
udp:
    TPE(("eqHead: udp\n"));
    LENTEST(len,sizeof(struct udphdr));
    if ( eq_ports(ih,th) ) {
        TPE(("eqHead: udp ports s %d, d %d\n",
             ntohs(ih->udp.uh_sport),ntohs(ih->udp.uh_dport)));
        return 1;
    }
    return 0;
    
tcp:
    TPE(("eqHead: tcp\n"));
    LENTEST(len,(ih->tcp.th_off << 2)); // size of tcp header in bytes
    if ( eq_ports(ih,th) ) {
        TPE(("eqHead: tcp ports s %d, d %d\n",
             ntohs(ih->tcp.th_sport),ntohs(ih->tcp.th_dport)));
        *s_info = si_tcp;
        return 1;
    }
    return 0;
    
bail:
    *s_info = si_quit;          // give up
    return 1;                   // signal to break loop
}


//* findHead
//
//  Finds a flow in the compression state list.
//  Has the side effect of moving the found flow
//  first in the list.
//  If it returns nonzero than *s_info != si_quit
//


static struct compState *
findHead(struct compStateList *csList, 
         union ipHead *iph, 
         enum searchInfo *s_info, 
         union ipHead **iph_ptr)
{
    struct compState *cs = csList->first;
    struct compState *tmp = 0 ;
    *s_info = si_nontcp;	// default
    
    if ( !cs ) 
        return 0;
    if ( eqHead(csList, iph, cs->ipHead, s_info, iph_ptr) )
        return cs;
    
    while(tmp=cs, cs=cs->next) {
        if ( eqHead(csList, iph, cs->ipHead, s_info, iph_ptr) ) {
            if ( *s_info == si_quit )
                return 0;
            tmp->next = cs->next;
            cs->next = csList->first;
            csList->first = cs;
            return cs;
        }
    }
    return 0;
}


#define INCTEST(l,s) \
if (((l) += (s))> csList->cfg.max_header ) { (l)-=(s); goto done;}

//* compress
//
//  Compress if possible. 
//  Update fields in compression state `cs'. 
//  Return the action taken.
//  tcp_iph points to the ip(4/6) head that is closest to the tcp header
//

static enum comprInfo
compress(struct compStateList *csList,
         union ipHead *iph,
         int *iph_len,		// in pkt len, out pkt hdr len 
         uchar *chdr,		// pointer to the compresses header
         int *chdr_len,		// new header length
         struct compState *cs,
         union ipHead *tcp_iph)	// for ip4/tcp id
{
    struct compHead *ch = (struct compHead *)chdr;
    uchar *rdp, *rdp_save;      // random data ptr
    union ipHead *ih = iph;
    union ipHead *th;
    enum comprInfo cinfo;
    uint16 newcid = 0;
    uint16 i,t,hlen = 0;
    uint16 changed = 0;
    uint32  changes;            // tcp only
    struct tcphdr *it, *tt;     // tcp only
    uint16 t_ip_len, i_ip_len;	// tcp only
    uint16 t_data_len;          // tcp only
    uint32 deltaS, deltaA;      // tcp only
    uint16 ip4idbit = 0;        // tcp only
    union ipHead *ttcp_iph;     // tcp only
    uint32 tmp;                 // tcp only

    if ( cs == 0 ) {
        cs = newCid(csList, (int)tcp_iph);
        newcid = changed = 1;
        cs->c_max = F_CHANGE;
        cs->c_num = 0;
    }
    TPCI(("compress: Cid == %d %s\n",cs->cid,cs?"":"NEW"));
    
    changes = 0;		// set ch->fg later 
    ch->cid = (uchar)cs->cid;
    rdp = ((uchar*)ch) + COMPH_SIZE;
    if ( tcp_iph )		// adjust for checksum 
        rdp += 2;
    
    th = cs->ipHead;
    if ( ih->ip6_ip_v == 6 )	// we know that it is 4 or 6 
        goto ip6;

ip4:
    TPC(("compress: ip4:\n"));
    INCTEST(hlen,sizeof(struct ip));
    changed = (changed ||
               ih->ip4.ip_ttl!= th->ip4.ip_ttl ||
               ih->ip4.ip_off != th->ip4.ip_off ||
               ih->ip4.ip_tos != th->ip4.ip_tos);
    
    i = ih->ip4.ip_p;
    // do it VJ style if next proto is tcp 
    if ( i ==  IPPROTO_TCP && tcp_iph) {
        tmp = ntohs(ih->ip4.ip_id) - ntohs(th->ip4.ip_id);
        if (tmp != 1) {
            ENCODEZ(tmp); 
            ip4idbit =  NEW_I;                      
        }
        ttcp_iph = th;                  // for use in tcp 
    } else {			// random data 
        bcopy(&(ih->ip4.ip_id),rdp,2);
        rdp += 2;
    }
    INC(ih,sizeof(struct ip));  // XXX ip len 
    INC(th,sizeof(struct ip));  // XXX ip len 
    switch (i) {
        //      XXX should handle AUTH and ESP 
    case IPPROTO_TCP:  goto tcp; 
    case IPPROTO_UDP:  goto udp; 
    case IP6PROTO_IP4: goto ip4;  
    case IP6PROTO_IP6: goto ip6; 
    }
    ERROR(("compress: cant handle proto %x in ipv4\n",i));
    goto bail;
    
ip6:
    TPC(("compress: ip6:\n"));
    INCTEST(hlen,sizeof(struct IPv6Header));
    changed = (changed || ih->ip6.HopLimit !=  th->ip6.HopLimit);
    // ih->ip6.ip6_prio is checked in eqhead 
    ttcp_iph = th;                          // for use in tcp 
    i = ih->ip6.NextHeader;
    INC(ih,sizeof(struct IPv6Header));
    INC(th,sizeof(struct IPv6Header));
    
ip6h:
    switch(i) {
    case IP_PROTOCOL_HOP_BY_HOP:
        INCTEST(hlen,OPTLEN(ih));
        changed = (changed || bcmp(ih,th,OPTLEN(ih)) != 0);
        i = NEXTOP(th);
        INC(ih,OPTLEN(th));
        INC(th,OPTLEN(th));
        goto ip6h;
        
    case IP_PROTOCOL_ROUTING: 
        INCTEST(hlen,OPTLEN(ih));
        i = NEXTOP(th);
        INC(ih,OPTLEN(th));
        INC(th,OPTLEN(th));
        goto ip6h;
        
    case IP_PROTOCOL_AH:
        INCTEST(hlen,OPTLEN(ih));
        bcopy(((uchar*)ih)+8,rdp,OPTLEN(ih)-8);
        rdp += OPTLEN(ih)-8;
        i = NEXTOP(ih);
        INC(ih,OPTLEN(ih));
        INC(th,OPTLEN(th));
        goto ip6h;
        
    case IPPROTO_TCP:     goto tcp;
    case IPPROTO_UDP:     goto udp;
    case IP6PROTO_IP4:    goto ip4;
    case IP6PROTO_IP6:    goto ip6;
    case IP_PROTOCOL_ESP: goto esp;
        
        // bail on these 
    case IP_PROTOCOL_FRAGMENT:
    case IP_PROTOCOL_NONE:         // uh?? XXX 
    default:                    // uh?? XXX 
        ERROR(("compress: cant handle proto %x in ipv6\n", i));
        goto bail;
    }
    
esp:
    INCTEST(hlen,sizeof(uint32));
    goto done;
    
tcp:
    TPC(("compress: tcp\n"));
    it = &ih->tcp;
    tt = &th->tcp;
    INCTEST(hlen,(it->th_off << 2)); 
    TPC(("compress: tcp seg %u  ack %u\n",
         (unsigned)ntohl(it->th_seq),(unsigned)ntohl(it->th_ack)));
    if ((ih->tcp.th_flags & (TH_SYN | TH_FIN | TH_RST | TH_ACK)) != TH_ACK) {
        cinfo = ci_regu;
        goto done_tcp;
    }
    cinfo = ci_full;
    if ( changed  || it->th_off != tt->th_off ) // full 
        goto done_tcp;
    
    // XXX ip changes 
    
    // ip4 id allready there, do checksum 
    bcopy(&(it->th_sum), (char*)ch + COMPH_SIZE, 2);
    rdp_save = rdp;		// save for special cases below 
    
    deltaS = ntohl(it->th_seq) - ntohl(tt->th_seq);
    TPC(("compress: deltaS = %u iseq %u tseq %u\n",
         deltaS, (unsigned)ntohl(it->th_seq), (unsigned)ntohl(tt->th_seq)));
    
    if (deltaS) {
        if (deltaS > 0xffff)
            goto done_tcp;                      // full 
        ENCODE(deltaS);
        changes |= NEW_S;
    }  
    
    if ( (deltaA = ntohl(it->th_ack) - ntohl(tt->th_ack)) ) {
        if (deltaA > 0xffff)
            goto done_tcp;          // full 
        ENCODE(deltaA);  
        changes |= NEW_A;
    }
    
    if ( (tmp = (uint16) (ntohs(it->th_win) - ntohs(tt->th_win))) ) {
        ENCODE(tmp);
        changes |= NEW_W;
    }
    
    if (it->th_flags & TH_URG ) {
        tmp = ntohs(it->th_urp);
        ENCODEZ(tmp);
        changes |= NEW_U; 
    } else if ( it->th_urp != tt->th_urp) { // URG not set but urp changed 
        goto done_tcp;		// full 
    }
    
    if ( tcp_iph->ip4.ip_v == 4 ) {
        i_ip_len = ntohs(tcp_iph->ip4.ip_len); 
        t_ip_len = ntohs(ttcp_iph->ip4.ip_len);
        t_data_len = t_ip_len -(tt->th_off << 2) -((uchar*)tt -(uchar*)ttcp_iph);
    } else { 
        i_ip_len = ntohs(tcp_iph->ip6.PayloadLength); 
        t_ip_len = ntohs(ttcp_iph->ip6.PayloadLength); 
        t_data_len = t_ip_len - ( tt->th_off << 2);
    }
    switch (changes) {
    case 0:
        //
        // Nothing changed. If this packet contains data and the last
        // one didn't, this is probably a data packet following an
        // ack (normal on an interactive connection) and we send it
        // compressed.  Otherwise it's probably a retransmit,
        // retransmitted ack or window probe.  Send it uncompressed
        // in case the other side missed the compressed version.
        //
        TPC(("compress: NOTHING\n"));
        if (t_ip_len != i_ip_len && t_data_len == 0 )
            break; 
        
    case SPECIAL_I: 
    case SPECIAL_D:
        TPC(("compress: SPECIAL -> full\n"));
        goto done_tcp;                          // full 
        
    case NEW_S | NEW_A:
        TPC(("compress: NEW_S | NEW_A\n"));
        if (deltaS == deltaA && deltaS == t_data_len ) {
            // special case for echoed terminal traffic 
            changes = SPECIAL_I;
            rdp = rdp_save;
            TPC(("compress: SPECIAL_I\n"));
        }
        break;
        
    case NEW_S:
        TPC(("compress: NEW_S\n"));
        if (deltaS == t_data_len ) {
            // special case for data xfer 
            changes = SPECIAL_D;
            rdp = rdp_save;
            TPC(("compress: SPECIAL_D deltaS = %u\n",deltaS));
        }
        break;
    }
    tmp = (it->th_off - 5) << 2; // tmp = options size in bytes 
    if ( tmp && bcmp(it+1, tt+1, tmp)) {
        TPC(("compress: tcp options %u bytes\n",tmp));
        changes |= NEW_O;		// set O-bit and copy data 
        bcopy(it+1,rdp, tmp); 
        rdp += tmp;
    }
    if ( it->th_flags & TH_PUSH) 
        changes |= TCP_PUSH_BIT;
    ch->fg = (uchar)(changes | ip4idbit);
	TPC(("compress: TcpHc= %x %x %x %x %x\n", ((unsigned*)(it))[0],
	     ((unsigned*)(it))[1], ((unsigned*)(it))[2],
	     ((unsigned*)(it))[3], ((unsigned*)(it))[4] ));
	TPC(("compress: TcpHc=        %x %x %x\n",((unsigned*)(it))[5],
	     ((unsigned*)(it))[7], ((unsigned*)(it))[7]));
    
    cinfo = ci_ctcp;
    bcopy (iph, cs->ipHead, hlen); // update delta fields 
    goto done_tcp;
    
udp:
    TPC(("compress: udp\n"));
    INCTEST(hlen,sizeof(struct udphdr));
    bcopy(&(ih->udp.uh_sum),rdp,2);
    rdp += 2;
    goto done;
    
bail:
    ERROR(("compress: bailing out\n"));
    if (newcid)
        freeFirstCid(csList);
    return ci_err;
    
done:				// udp  backoff stuff 
    if ( changed ) {
        cs->c_num = 0;
        // f_last  XXX 
        cs->c_max = F_CHANGE;
        cinfo = ci_full;
        cs->gen++;
    } else if ( cs->c_num >= cs->c_max ) {  
        cs->c_num = 0;
        // f_last  XXX 
        cs->c_max = cs->c_max << 1;
        cs->c_max = cs->c_max > F_MAX_PERIOD ? F_MAX_PERIOD : cs->c_max;
        cinfo = ci_full;
    } else {
        cs->c_num++;
        cinfo = ci_cudp;
    }
    // XXX get time !!!
    // if ( time.tv_sec % 5 == 0 ) {
	// TPC(("compress: TimeOut???\n"));
    // } 
    ch->fg = cs->gen;
    // fall through to done_tcp
    
done_tcp:			// this is for both tcp and udp 
    TPC(("compress: done_tcp %d rdp %x chdr %x\n",
         cinfo, (unsigned)rdp , (unsigned)chdr));
    switch ( cinfo ) {
    case ci_full:
        cs->head_req = 0;
        bcopy (iph, cs->ipHead, hlen);
        if ( (iph)->ip4.ip_v == 4 ) {
            GEN_IP4(iph) = cs->gen ; // XXX not as in  draft 
            CID_IP4(iph) = (uchar)cs->cid; 
            (iph)->ip4.ip_v = tcp_iph ? V6C_TCP_FULL4 :  V6C_UDP_FULL4;
        } else {
            GEN_IP6(iph) = cs->gen; // XXX not as in  draft 
            CID_IP6(iph) = (uchar)cs->cid;
            (iph)->ip6_ip_v = tcp_iph ? V6C_TCP_FULL6 :  V6C_UDP_FULL6;
        }
        cs->len = hlen;
        INC_ST2(csList->cstat.v6s_bbefore_c, hlen);
        INC_ST2(csList->cstat.v6s_bafter_c,  hlen);
        break;
        
    case ci_cudp:
    case ci_ctcp:
        *chdr_len  = (uchar*)rdp - (uchar*)chdr;
        *iph_len = hlen;
        
        INC_ST2(csList->cstat.v6s_bbefore_c, hlen);
        INC_ST2(csList->cstat.v6s_bafter_c,  *chdr_len);
        TPC(("compress: CH = %0x,%0x,%0x,%0x %0x,%0x,%0x,%0x "
             "%0x,%0x,%0x,%0x %0x,%0x,%0x,%0x\n",
             chdr[0], chdr[1], chdr[2], chdr[3],
             chdr[4], chdr[5], chdr[6], chdr[7],
             chdr[8], chdr[9],chdr[10],chdr[11],
             chdr[12],chdr[13],chdr[14],chdr[15]));
        TPC(("compress: comp %d - %d = %d\n",hlen,hlen - *chdr_len,*chdr_len));
        break;
        
    case ci_regu:
        if (newcid)
            freeFirstCid(csList);
        break;
        
    default:
        ERROR(("compress: error cinfo = %d\n",cinfo));
        break;
    }
    return cinfo;
}


//* v6c_comp 
//  Compresses the header if possible. 
//  Calls findhead() to get a hold of the correct state, 
//  then calls on compress to do the compression.
//

enum comprInfo
v6c_comp (struct compStateList *csList,
          uchar *iphdr,         // pointer to where the ip header starts 
          int  *iphdr_len,		// -> pkt len  <- pkt hdr len 
          char *chdr,           // pointer to the compressed header 
          int *chdr_len)        // new header length 
{
    struct compState *cs;
    enum searchInfo s_info = si_nontcp;
    union ipHead *iph_ptr = 0;
    union ipHead *iph = (union ipHead *) iphdr;
    enum comprInfo c_info;

    if (!iph) {
      TPC(("v6c_comp: ???????????? NO INPUT HEADER!!!!\n"));
      return ci_err;
    }
    switch ( iph->ip4.ip_v ) {
    case 4:
        TPC(("v6c_comp: ---------- ipv4 iphdr_len %d,  real len %d\n",
             *iphdr_len,ntohs(iph->ip4.ip_len)));
        break;
    case 6:
        TPC(("v6c_comp: ---------- ipv6 iphdr_len %d,  real len %d\n",
             *iphdr_len,ntohs(iph->ip6.PayloadLength)+40));
        break;
    default:
        INC_ST(csList->cstat.v6s_unknown_c); 
        TPC(("v6c_comp: ???????????? UNKNOWN HEADER TYPE !!!!\n"));
        return ci_err;
    }
    
    cs = findHead(csList, iph, &s_info, &iph_ptr);
    
    if ( cs ) 
        TPC(("findHead: cid = %u, info = %d\n",cs->cid,s_info));
    else 
        TPC(("findHead: not found, info = %d\n",s_info));
    
    switch ( s_info ) {
    case si_quit:
        INC_ST(csList->cstat.v6s_unmod_c);
        TPC(("v6c_comp: findHead => ci_regu\n"));
        return ci_regu;
    case si_err:
        INC_ST(csList->cstat.v6s_unmod_c);
        TPC(("v6c_comp: findHead => unknown\n"));
        return ci_err;
    case si_nontcp:
        iph_ptr = 0;
        break;
    }
    
    c_info = compress(csList, iph, iphdr_len, chdr, chdr_len, cs,  iph_ptr);
    
    switch (c_info) {
    case ci_full:
        INC_ST(csList->cstat.v6s_full_c);
        TPC(("v6c_comp: compress => ci_full\n"));
        break;
    case ci_cudp:
    case ci_ctcp:
        INC_ST(csList->cstat.v6s_comp_c);
        TPC(("v6c_comp: compress => ci_comp\n"));
        break;
    case ci_regu:
        INC_ST(csList->cstat.v6s_unmod_c);
        TPC(("v6c_comp: compress => ci_regu\n"));
        break;
    case ci_err:
        INC_ST(csList->cstat.v6s_unknown_c);
        TPC(("compress => unknown\n"));
        break;
    }
    return c_info;
}

