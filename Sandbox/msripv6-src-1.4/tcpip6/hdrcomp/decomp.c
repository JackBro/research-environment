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

#include "oscfg.h"
#include "ndis.h"
#include "ntddk.h"
#include "compi.h"
#include "comp.h"

uint32 kkk = 0xFF;

// XXX Unix stuff that should be changed to windows dito,
// XXX but I do not know the function names at the moment.

static void 
bzero(void *c, int size)
{
  int i;
  for(i=0;i<size;i++)
    ((uchar *)c)[i] =0;
}
static void 
bcopy(void *from, void *to, int size)
{
  int i;
  for(i=0;i<size;i++)
    ((uchar*)to)[i] = ((uchar*)from)[i];
}

static int 
bcmp(void *a, void *b, int len)
{
  int i;
  for(i=0;i<len;i++) {
    if ( ((uchar*)a)[i] != ((uchar*)b)[i] )
      return 1;
  }
  return 0;
}

// XXX end of Unix stuff 

__inline
static uint16 
ipChSum(uint16 *s)
{
    register uint32 sum; 
    register uint16 csum;

    sum = s[0]+s[1]+s[2]+s[3]+s[4]+s[6]+s[7]+s[8]+s[9];
    while((csum = (ushort)(sum >> 16)) != 0)	// end-around-carry adjustment 
	sum = csum + (sum & 0xffffL);
    return ~((uint16)(sum & 0xffffl)); // complement and return 
}

// pseudo sum, length not included and not ~ 
__inline
static uint16 
ip_pseudo_sum(union ipHead *ipsum_ptr)
{
    register uint16 *s = (uint16 *)ipsum_ptr;
    register uint32 sum;
    register uint32 i; 
    register uint16 csum;
    
    if ( ipsum_ptr->ip4.ip_v == 4 ) { 
	sum = htons(IPPROTO_TCP) +s[6]+s[7]+s[8]+s[9] ;
    } else {
	sum = (htons(IPPROTO_TCP)); // XXX ugly 
	s+=4;
	for (i=0; i<16; i++)
	    sum += *s++;
    }
    while((csum = (ushort)(sum >> 16)) != 0)	// end-around-carry  
	sum = csum + (sum & 0xffffL);
    return (uint16)(sum & 0xffffl); // no complement 
}

__inline
static uint32 
tcp_sum(uint16 *s,int words)
{
    register uint32 sum=0;
    register uint16 *s2 = s;
    while(words--) { 
	sum += *s++;
    }
    sum -= s2[8];		// subtract checksum 
    return sum;
}struct compStateList *csList;
    

static void 
header_request(struct compStateList *csList,
	       unsigned int cid)
{
    if (cid >= csList->cfg.len_recv_t ) {
	ERROR(("v6c_decomp: large cid for head req %d\n",cid));
    } else {
	csList->recv_t[cid].head_req++;
    }
}


#define INCTEST(l,s) \
if (((l) += (s))>csList->cfg.max_header) { (l)-=(s); goto done;}

int v6c_decompf(struct compStateList *csList, // comp state 
		struct mbuf *m,	// the mbuf 
		char  *iphdr,	// pointer to where the ip header starts 
		ushort len,	// tot length (not including ppp header) 
		int bufsize)	// size of continuous memory 
{
    ushort cid, gen;
    ushort hlen = 0;
    ushort i;
    union ipHead *ih = (union ipHead *) iphdr;
    union ipHead *ihSave = ih;
    union ipHead *ipsum_ptr = ih;
    struct compState *states = csList->recv_u;

    TPDF(("v6c_decompf: ----------  len=%d\n",len));
    switch (ih->ip4.ip_v) {
    case V6C_TCP_FULL4:
    case V6C_TCP_FULL4REQ:
    case V6C_UDP_FULL4:
    case V6C_UDP_FULL4REQ:
	TPDF(("v6c_decompf: ipv4\n"));
	if ( len < sizeof(struct ip) )
	    goto bail;
	cid = CID_IP4(ih);
	gen = GEN_IP4(ih);
	ih->ip4.ip_len = htons(len);
	ih->ip4.ip_v = 4;
	goto ip4;

    case V6C_TCP_FULL6:
    case V6C_TCP_FULL6REQ:
    case V6C_UDP_FULL6:
    case V6C_UDP_FULL6REQ:
	TPDF(("v6c_decompf: ipv6\n"));
	if ( len < sizeof(struct IPv6Header) )
	    goto bail;
	cid = CID_IP6(ih);
	gen = GEN_IP6(ih);
	ih->ip6.PayloadLength  = htons(len - sizeof(struct IPv6Header));
	ih->ip6_ip_v = 6;
	goto ip6;

    default:
	ERROR(("v6c_decompf: strange input %u\n",ih->ip4.ip_v));
	INC_ST(csList->cstat.v6s_dropped_d);
	return -1;
    }
ip4:
    TPDF(("v6c_decompf: ip4\n"));
    INCTEST(hlen,sizeof(struct ip));
    ipsum_ptr = ih;
    i = ih->ip4.ip_p;
    INC(ih,sizeof(struct ip));
    switch (i) {
    case IPPROTO_TCP:  goto tcp; 
    case IPPROTO_UDP:  goto udp; 
    case IP6PROTO_IP4: goto ip4;  
    case IP6PROTO_IP6: goto ip6; 
    }
    ERROR(("v6c_decompf: men vad?\n"));
    goto bail;
    
ip6:
    TPDF(("v6c_decompf: ip6\n"));    
    INCTEST(hlen,sizeof(struct IPv6Header));
    ipsum_ptr = ih;
    i = ih->ip6.NextHeader;
    INC(ih,sizeof(struct IPv6Header));
    
ip6h:
    switch(i) {
	// skip these 
    case IP_PROTOCOL_HOP_BY_HOP:
    case IP_PROTOCOL_ROUTING: 
	INCTEST(hlen,OPTLEN(ih));
	i = NEXTOP(ih);
	INC(ih,OPTLEN(ih));
	goto ip6h;
		
    case IP_PROTOCOL_AH:
	INCTEST(hlen,OPTLEN(ih));
	i = NEXTOP(ih);
	INC(ih,OPTLEN(ih));
	goto ip6h;
		
	// endpoints 
    case IPPROTO_TCP:  goto tcp;
    case IPPROTO_UDP:  goto udp;
    case IP6PROTO_IP4: goto ip4;
    case IP6PROTO_IP6: goto ip6;
    case IP_PROTOCOL_ESP: goto esp;
		
	// bail on these 
    case IP_PROTOCOL_FRAGMENT:
    case IP_PROTOCOL_NONE:		// uh?? XXX 
    default:			// uh?? XXX 
	ERROR(("v6c_decompf: vad i hela...\n"));
	goto bail;
    }
    
esp:
    INCTEST(hlen,sizeof(uint32));
    goto done;
    
udp:
    TPDF(("v6c_decompf: udp\n"));    
    INCTEST(hlen,sizeof(struct udphdr));
    goto done;
    
tcp:
    TPDF(("v6c_decompf: tcp\n"));
    INCTEST(hlen,(ih->tcp.th_off << 2));
    TPDF(("v6c_decompf: tcp header is %d bytes long\n",
	  (unsigned)(ih->tcp.th_off << 2)));
    states = csList->recv_t;
    states[cid].tcp_payload = len - hlen;
    states[cid].tcp_cksum = ip_pseudo_sum(ipsum_ptr);
    goto done;
    
bail:
    ERROR(("v6c_decompf: bailing out\n"));
    INC_ST(csList->cstat.v6s_dropped_d);
    return -1;
	
done:
    TPDF(("v6c_decompf: donefull cid %d gen %d len %d \n", cid, gen, hlen));
    bcopy((void*)ihSave, states[cid].ipHead ,hlen);
    states[cid].len = hlen;
    states[cid].gen = (uchar)gen;
    INC_ST(csList->cstat.v6s_full_d);
    return 0;
}

int v6c_decomp(struct compStateList *csList, // comp state 
	       uchar  *iphdr,	// pointer to where the comp ip header starts
	       int ip_len,	// tot length (not including ppp header) 
	       int isTcp,	// =1 if tcp flow 
	       uchar **hdrp,	// pointer to the decompressed header 
	       int *hlenp)	// length of the decompressed header 
{
    unsigned int cid;
    unsigned char fgbyte;
    struct compState *states, *cs;
    uint16 t;
    uint16 hlen = 0;
    uchar *rdp;		// random data ptr 
    union ipHead *th;
    uchar *thBase;
    uint16 tot_size;
    uint16 nlengths = 0;
    uint16 lengths[LEN_MAX];
    struct tcphdr *tcp_hdr = 0;
    uint32 ipcksum,tcpcksum,packsum,c_sum,cksum,comp_len,tmp;
    
    cid = ((struct compHead*)iphdr)->cid;
    fgbyte = ((struct compHead*)iphdr)->fg;
    TPD(("v6c_decomp: ------  CH = %0x,%0x,%0x,%0x %0x,%0x,%0x,%0x "
	 "%0x,%0x,%0x,%0x %0x,%0x,%0x,%0x \n" ,
	 iphdr[0], iphdr[1], iphdr[2], iphdr[3],
	 iphdr[4], iphdr[5], iphdr[6], iphdr[7],
	 iphdr[8], iphdr[9], iphdr[10],iphdr[11],
	 iphdr[12],iphdr[13], iphdr[14],iphdr[15]));
    
    if (  isTcp ) {
	TPD(("v6c_decomp: type TCP\n"));
	states = csList->recv_t; 
	if (cid >= csList->cfg.len_recv_t ) {
	    ERROR(("v6c_decomp: large tcp cid %d\n",cid));
	    INC_ST(csList->cstat.v6s_dropped_d);
	    return -1;			
	}
	if ( states[cid].len == 0 ) {
	    ERROR(("v6c_decomp: tcp cid not in use %d\n",cid));
	    INC_ST(csList->cstat.v6s_dropped_d);
	    return -1;
	}
    } else {
	TPD(("v6c_decomp: type UDP\n"));
	states = csList->recv_u;
	if (cid >=  csList->cfg.len_recv_u ) {
	    ERROR(("v6c_decomp: large udp cid %d\n",cid));
	    INC_ST(csList->cstat.v6s_dropped_d);
	    return -1;			
	}
	if ( states[cid].len == 0 ) {
	    ERROR(("v6c_decomp: udp cid not in use %d\n",cid));
	    INC_ST(csList->cstat.v6s_dropped_d);
	    return -1;
	}
	if ( states[cid].gen != fgbyte ) {
	    ERROR(("v6c_decomp: wrong generation %d %d \n",
		   states[cid].gen, fgbyte));
	    INC_ST(csList->cstat.v6s_dropped_d);
	    return 0;
	}
    }
    cs = states + cid; 
    rdp = iphdr + COMPH_SIZE;	// random data ptr 
    if ( isTcp )		// adjust for checksum 
	rdp += 2;
    
    th     = cs->ipHead;
    thBase = (uchar*) cs->ipHead;
    
    switch (th->ip4.ip_v) {
    case 4: goto ip4;
    case 6: goto ip6;
    default:			// XXX not needed 
	ERROR(("v6c_decomp: hmm... ip version %u\n",th->ip4.ip_v));
	INC_ST(csList->cstat.v6s_dropped_d);
	return -1;
    }
    
ip4:
    TPD(("v6c_decomp: IP4\n"));
    INCTEST(hlen,sizeof(struct ip));
    lengths[nlengths++] = (uint16)((uchar*)th - thBase) | LEN_IP4;
    t = th->ip4.ip_p;
    
    // If we have an ip4 header next to the tcp header then do 
    //  the ip4 id vj style otherwise treat it as random data  
    if ( isTcp &&  t ==  IPPROTO_TCP) {
	if ( fgbyte & NEW_I ) {
	    DECODES(th->ip4.ip_id);
	} else {
	    th->ip4.ip_id = htons(ntohs(th->ip4.ip_id) + 1);
	}
    } else {			// random data 
	bcopy(rdp,&(th->ip4.ip_id),2);
	rdp += 2;
    }
    
    INC(th,sizeof(struct ip));
    switch (t) {
	// XXX auth 
    case IPPROTO_TCP:  goto tcp; 
    case IPPROTO_UDP:  goto udp; 
    case IP6PROTO_IP4: goto ip4;  
    case IP6PROTO_IP6: goto ip6; 
    }
    ERROR(("v6c_decomp: nej!\n"));
    goto bail;
    
ip6:
    TPD(("v6c_decomp: IP6\n"));
    INCTEST(hlen,sizeof(struct IPv6Header));
    lengths[nlengths++] = (uint16)((uchar*)th - thBase) | LEN_IP6;
    t = th->ip6.NextHeader;
    INC(th,sizeof(struct IPv6Header));
    
ip6h:
    switch(t) {
    case IP_PROTOCOL_HOP_BY_HOP:
    case IP_PROTOCOL_ROUTING: 
	INCTEST(hlen,OPTLEN(th));
	t = NEXTOP(th);
	INC(th,OPTLEN(th));
	goto ip6h;
		
    case IP_PROTOCOL_AH:
	INCTEST(hlen,OPTLEN(th));
	bcopy(rdp,((uchar*)th)+8,OPTLEN(th)-8);
	rdp += OPTLEN(th)-8;
	t = NEXTOP(th);
	INC(th,OPTLEN(th));
	goto ip6h;
		
	// endpoints 
    case IPPROTO_TCP:  goto tcp;
    case IPPROTO_UDP:  goto udp;
    case IP6PROTO_IP4: goto ip4;
    case IP6PROTO_IP6: goto ip6;
    case IP_PROTOCOL_ESP: goto esp;
		
	// bail on these 
    case IP_PROTOCOL_FRAGMENT:
    case IP_PROTOCOL_NONE:		// uh?? XXX 
    default:			// uh?? XXX 
	ERROR(("v6c_decomp: vad i hela...\n"));
	goto bail;
    }
    
esp:
    INCTEST(hlen,sizeof(uint32));
    goto done;
	
tcp:
    TPD(("v6c_decomp: TCP  len = %u \n", th->tcp.th_off));
    INCTEST(hlen,(th->tcp.th_off << 2));
    tcp_hdr = (struct tcphdr *)th;
    th->tcp.th_sum = htons((iphdr[2] << 8) | iphdr[3]);
	
    if (fgbyte & TCP_PUSH_BIT)
	th->tcp.th_flags |= TH_PUSH;
    else
	th->tcp.th_flags &= ~TH_PUSH;

    switch (fgbyte & SPECIALS_MASK) {
    case SPECIAL_I:		// SPECIAL_(I/D) could be merged  
	TPD(("v6c_decomp: SPECIAL_I\n"));
	th->tcp.th_ack = htonl(ntohl(th->tcp.th_ack) + cs->tcp_payload);
	th->tcp.th_seq = htonl(ntohl(th->tcp.th_seq) + cs->tcp_payload);
	break;
		
    case SPECIAL_D:
	TPD(("v6c_decomp: SPECIAL_DATA seq = %u\n",
	     (unsigned)ntohl(th->tcp.th_seq)));
	th->tcp.th_seq = htonl(ntohl(th->tcp.th_seq) + cs->tcp_payload);
	break;
		
    default:
	if (fgbyte & NEW_S)
	    DECODEL(th->tcp.th_seq);
	if (fgbyte & NEW_A)
	    DECODEL(th->tcp.th_ack);
	if (fgbyte & NEW_W)
	    DECODES(th->tcp.th_win);
	if (fgbyte & NEW_U) {
	    th->tcp.th_flags |= TH_URG;
	    DECODEU(th->tcp.th_urp);
	} else { 
	    th->tcp.th_flags &= ~TH_URG;
	}
	break;
    }

    tmp = (th->tcp.th_off - 5) << 2; // tmp = options size in bytes 
    if ( fgbyte & NEW_O ) { 
	if ( tmp ) {
	    bcopy(rdp, tcp_hdr+1, tmp);
	    rdp += tmp;
	} else {
	    ERROR(("v6c_decomp: NEW_O but no options in table\n"));
	    goto bail;
	}
    }
    
    // ip4 id have bin done at label ip4: 
    // checksum gets done at label done: 
    // save tcp payload size 
    cs->tcp_payload = ip_len - (uint16)(rdp - iphdr); 
    goto done;

udp:
    TPD(("v6c_decomp: UDP\n"));
    INCTEST(hlen,sizeof(struct udphdr));
    lengths[nlengths++] = (uint16)((uchar*)th - thBase) | LEN_UDP;
    bcopy(rdp,&(th->udp.uh_sum),2);
    rdp += 2;
    goto done;
    
bail:
    ERROR(("v6c_decomp: bailing out\n"));
    INC_ST(csList->cstat.v6s_dropped_d);
    return -1;
done:
    tot_size = cs->len + ip_len - (uint16)(rdp - iphdr);
    // fix lengths 
    for(t=0; t < nlengths; t++) {
	th = (union ipHead *) (thBase + LEN_OFFS(lengths[t]));
	switch ( LEN_TYPE(lengths[t]) ) {
	case LEN_IP6:
	    th->ip6.PayloadLength =
		htons(tot_size - LEN_OFFS(lengths[t]) - sizeof(struct IPv6Header));
	    break;
	case LEN_IP4:
	    th->ip4.ip_len = htons(tot_size - LEN_OFFS(lengths[t]));
	    th->ip4.ip_sum = ipChSum((uint16*)th); // checksum 
	    break;
	case LEN_UDP:
	    th->udp.uh_ulen = htons(tot_size - LEN_OFFS(lengths[t]));
	    break;
	}
    }
    

    TPD(("v6c_decomp: hlen %d   cs->len %d\n",hlen ,cs->len));
    ass(hlen == cs->len);
    *hdrp = thBase;
    // rdp now points at the payload (after the random data)  
    *hlenp = hlen;
    TPD(("v6c_decomp: cid %d  gen %d  hlen %d  comp len %d\n",
	cs->cid, cs->gen, hlen, rdp - iphdr));
    INC_ST(csList->cstat.v6s_comp_d);
    return rdp - iphdr;
}
  
