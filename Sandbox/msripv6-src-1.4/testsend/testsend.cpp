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
// Test tool for sending different types raw IPv6 packets.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2ip6.h>
#include <ip6.h>
#include <icmp6.h>

#include "testsend.h"

#define IP_VERSION 0x00000060

static ulong Id = 0;  // used for fragment ids.

ProtoTblStruct ProtocolTbl[] = {
#if 0
    { "icmp", IP_PROTOCOL_ICMPv6, CreateICMPMsg },
#endif
    { "ip", IP_PROTOCOL_NONE, CreateIPMsg },
    { NULL, 0, NULL }
};

MesgTypeTblStruct MesgTypeTbl[] = {
    { "frag", "fragment", FRAG, IP_PROTOCOL_FRAGMENT },
    { "frag0", "fragment0", FRAG0, IP_PROTOCOL_FRAGMENT }, // last fragment.
    { "hop", "hopbyhop", HOP, IP_PROTOCOL_HOP_BY_HOP },
    { "dest", "destopts", DEST, IP_PROTOCOL_DEST_OPTS },
    { "route", "routing", RTG, IP_PROTOCOL_ROUTING },
    { NULL, NULL, 0, 0 }
};

ErrorTypeTblStruct ErrorTypeTbl[] = {
    { "param", "parameter_problem", PARAM_PROB },
    { "unreach", "dest_unreachable", DEST_UNREACH },
    { "time", "time_exceeded", TIME_EXCEED },
    { "big", "too_big", TOO_BIG },
    { NULL, NULL, 0 }
};


void _CRTAPI1
main(int argc, char **argv)
{
    int Error, i;
    WSADATA WsaData;
    SOCKET Sock;
	unsigned char TempBuf[1000];
	IPv6Header *IP;
	int Count, ch, HdrLen = 0;
	uchar HopLimit, IPProto;
    struct sockaddr_in6 Us, Peer, TmpAddr;
    short Port;
    char *Host;
    char *arg, *strarg;
    char *proto = NULL;
    char *mesg_type = NULL;
    char *error_type = NULL;
    char *DestOpt, *TmpField;
    ushort TypeFlag = 0, ErrorFlag = 0;
    ProtoTblStruct *ProtoEntry;
    OptionsList DestOptionsHead;
    OptionsList *DestOptionsTailPtr = &DestOptionsHead;
    OptionsList *CurrOpt;
    IPv6HomeAddressOption *HomeAddrOpt;
    IPv6BindingUpdateOption *BindUpdOpt;

	Count = 29;                  // Amount of raw data to send after IPv6 hdr.
	Port = 4000;                 // Arbitrary (and meaningless).
	HopLimit = 10;               // Arbitrary.
	IPProto = IP_PROTOCOL_NONE;  // The "No Next Header" type.
    DestOptionsHead.Next = NULL;

    if (argc <= 2) usage();

    //
    // Initialize winsock.
    //
    Error = WSAStartup(MAKEWORD(2, 0), &WsaData);
    if (Error == SOCKET_ERROR) {
        printf("testsend: WSAStartup failed %ld:", WSAGetLastError());
    }

    
    //
    // Process command line.
    //
    for (i=1; i < argc; i++) {
        arg = argv[i];

        if (arg[0] == '-') {
            switch(arg[1]) {
            case '?':
                usage();
                goto error_exit;

            case 'p':
                if (!param(&proto, argv, argc, i)) {
                    goto error_exit;
                }
                break;

            case 't':
                if (!param(&mesg_type, argv, argc, i)) {
                    goto error_exit;
                }
                if (!verify_type(mesg_type, &TypeFlag)) {
                    goto error_exit;
                }
                break;
            case 'e':
                if (!param(&error_type, argv, argc, i)) {
                    goto error_exit;
                }
                if (!verify_error(error_type, &ErrorFlag)) {
                    goto error_exit;
                }
                break;


            case 'd':
                // 
                // Process specified destination options.
                //
                if (!param(&DestOpt, argv, argc, i)) {
                    goto error_exit;
                }

                switch (arg[2]) {
                case 'h':
                    // Home address option. 
                    // Format: HomeAddress
                    CurrOpt = (OptionsList *) malloc(sizeof OptionsList + 
                                sizeof IPv6HomeAddressOption);

                    if (CurrOpt == NULL)
                        err("can't alloc memory for dest option.");
                    CurrOpt->Next = NULL;
                    CurrOpt->SubOpts = NULL;
                    CurrOpt->TotalSubOptLength = 0;

                    if (!GetNextField(&DestOpt, &TmpField)) {
                        printf("couldn't parse home address option.\n");
                        goto error_exit;
                    }

	                memset((char *)&TmpAddr, 0, sizeof(TmpAddr));
	                if (!parse_addr(TmpField, &TmpAddr))
                        err("bad home address");

                    HomeAddrOpt = (IPv6HomeAddressOption UNALIGNED *) &CurrOpt->GenericOptHdr;
                    HomeAddrOpt->Type = OPT6_HOME_ADDRESS;
                    HomeAddrOpt->Length = 16;
                    HomeAddrOpt->HomeAddress = TmpAddr.sin6_addr;

                    //
                    // process any sub-options specified.
                    //
                    ProcessSubOpts(CurrOpt, &DestOpt);

                    // Add to list of options
                    DestOptionsTailPtr->Next = CurrOpt;
                    DestOptionsTailPtr = CurrOpt;
                    break;

                case 'u': 
                    // Binding update option.
                    // Format: Flags,Prefix Length,Seq number,Lifetime[,Care-of addr]
                    CurrOpt = (OptionsList *) malloc(sizeof OptionsList + 
                                sizeof IPv6BindingUpdateOption);
                    if (CurrOpt == NULL)
                        err("can't alloc memory for dest option.");
                    CurrOpt->Next = NULL;

                    // Fill in the option header
                    BindUpdOpt = (IPv6BindingUpdateOption UNALIGNED *) &CurrOpt->GenericOptHdr;
                    BindUpdOpt->Type = OPT6_BINDING_UPDATE;
                    BindUpdOpt->Length = 8;

                    // Fill in option specific info.
                    if (!GetNextField(&DestOpt, &TmpField)) {
                        printf("couldn't parse Flags field in binding update option.\n");
                        goto error_exit;
                    }
                    BindUpdOpt->Flags = atoi(TmpField);

                    if (!GetNextField(&DestOpt, &TmpField)) {
                        printf("couldn't parse Prefix length in binding update option.\n");
                        goto error_exit;
                    }
                    BindUpdOpt->PrefixLength = atoi(TmpField);

                    if (!GetNextField(&DestOpt, &TmpField)) {
                        printf("couldn't parse seq number in binding update option.\n");
                        goto error_exit;
                    }
                    BindUpdOpt->SeqNumber = htons(atoi(TmpField));

                    if (!GetNextField(&DestOpt, &TmpField)) {
                        printf("couldn't parse lifetime in binding update option.\n");
                        goto error_exit;
                    }
                    BindUpdOpt->Lifetime = htonl(atoi(TmpField));


                    // The care-of address is optional.
                    // Make sure the next field isn't a sub-option.
                    if (DestOpt[0] != '-') {
                        //
                        // It's not a sub-option.
                        //
                        if (GetNextField(&DestOpt, &TmpField)) {
                            memset((char *)&TmpAddr, 0, sizeof(TmpAddr));
	                        if (!parse_addr(TmpField, &TmpAddr))
                                err("bad care-of address");
                            BindUpdOpt->CareOfAddr = TmpAddr.sin6_addr;
                            BindUpdOpt->Length = 24;
                        }
                    }
                    
                    //
                    // process any sub-options specified.
                    //
                    ProcessSubOpts(CurrOpt, &DestOpt);
                    
                    // Add to list of options
                    DestOptionsTailPtr->Next = CurrOpt;
                    DestOptionsTailPtr = CurrOpt;
                    break;

                default: 
                    printf("%s is not a valid command option.\n", argv[i]);
                    usage();
                    break;
                }

                break;

#if 0
            case 's':
                // potential option for source addr.
                break;
#endif
            default:
                printf("%s is not a valid command option.\n", argv[i]);
                usage();
                break;

            }
        }
    }
    
    //
    // Create the specific protocol headers.
    //
    if (proto) {
        if (!(ProtoEntry = GetProtoFunc(proto))) {
            printf("testsend: %s is an unsupported protocol.\n", proto);
            usage();
        }
        IPProto = (*ProtoEntry->cfunc)(TempBuf + sizeof(IPv6Header), 
                                       TypeFlag, ErrorFlag, &HdrLen, &DestOptionsHead);
    } else {
        if (ErrorFlag & PARAM_PROB) {
            IPProto = IP_PROTOCOL_BOGUS;
        }
    }

    
    //
    // Establish our peer's address.
    //
	memset((char *)&Peer, 0, sizeof(Peer));
	Host = argv[i-2];     // REVIEW: a little fragile to do this.
	if (!parse_addr(Host, &Peer))
        err("bad hostname");
    printf("Dest: %s\n", inet6_ntoa(&Peer.sin6_addr));
	Peer.sin6_port = htons(Port);
	
    //
    // Establish our address.
    //
	memset((char *)&Us, 0, sizeof(Us));
	Host = argv[i-1];
	if (!parse_addr(Host, &Us))
        err("bad hostname");
    printf("Src: %s\n", inet6_ntoa(&Us.sin6_addr));
	Us.sin6_port = 0;
	Us.sin6_family = AF_INET6;

    //
    // Create a raw IPv6 socket.
    //
	Sock = socket(PF_INET6, SOCK_RAW, 0);
	if (Sock == INVALID_SOCKET) {
        printf("socket() failed: %ld\n", GetLastError( ) );
        exit(1);
	}

    //
    // Bind our source address to the socket.
    // For raw send, this has the effect of picking an outgoing interface.
    //
    if (bind(Sock, (PSOCKADDR)&Us, sizeof(Us)) < 0) 
        err("bind");

    //
    // Put an IPv6 header at the beginning of our packet buffer.
    //
    IP = (IPv6Header *)((uchar *)TempBuf);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IPProto;
    IP->HopLimit = HopLimit;
    IP->Source = Us.sin6_addr;
    IP->Dest = Peer.sin6_addr;
    IP->PayloadLength = htons((ushort)(Count+HdrLen));

    //
    // Send the packet.
    //
	if (sendto(Sock, (char *)TempBuf, sizeof(IPv6Header) + Count + HdrLen, 0,
               (PSOCKADDR)&Peer, sizeof(Peer)) < 0) {
        printf("sendto() failed: %ld\n", GetLastError( ) );
        exit(1);
    }

    //
    // Cleanup.
    //
    closesocket(Sock);
    WSACleanup();
    exit(0);


  error_exit:
    printf("testsend: invalid option argument.\n");
    usage();
}



void
usage()
{
	printf("Usage: testsend [-p protocol] [-t mesg_type] [-dh home_addr]\n");
    printf("       [-du bind_update] [-e error_type] dest src\n");
    printf("\n       where protocol   = {ip}\n");
    printf("             mesg_type  = {frag | hop | dest}\n");
    printf("             error_type = {param}\n");
    printf("             home_addr = ipv6 addr to use for home address option[,subopts]\n");
    printf("             bind_update = Flags,Prefix Length,Seq no,Lifetime[,care-of addr][,subopts]\n");
    printf("             subopts: -u,unique id\n");
    printf("                      -h,home agent1,home agent2,...\n");
    
    exit(1);
}

//
// Print an error message.
//
void err(char *s)
{
        fprintf(stderr,"testsend: ");
        perror(s);
        fprintf(stderr,"errno=%ld\n", WSAGetLastError( ));
        WSACleanup();
        exit(1);
}


//
// Does not currently support DNS names, but should.
// (If WSAStringToAddress fails, then try DNS.)
//
int
parse_addr(char *s, struct sockaddr_in6 *sin6)
{
    struct hostent *h;
    
    sin6->sin6_family = AF_INET6;
    
    if (!inet6_addr(s, &sin6->sin6_addr)) {
        // Presumably the user gave us a DNS name.

        h = getipnodebyname(s, AF_INET6, AI_DEFAULT, NULL);
        if (h == NULL)
            return FALSE;

        memcpy(&sin6->sin6_addr, h->h_addr, sizeof(struct in6_addr));
        freehostent(h);
    }

    return TRUE;
}

//* CreateICMPMsg
uint
CreateIPMsg(uchar *TheBuf, 
            ushort TypeFlag, 
            ushort ErrorType, 
            int *Len,
            OptionsList *DestOpts)
{
    uint proto = 1;         // Value of 1 means protocol not specified.
    uchar *Buf = TheBuf;
    uchar *tmp_hdr = NULL;
    SubOptionsList *SubOptHdr;
    
    if (TypeFlag & HOP) {
        ExtensionHeaderPadded *hdr;

        // Create a minimum length, no-content, HopByHop Header.
        hdr = (ExtensionHeaderPadded*) Buf;        
        hdr->NextHeader = IP_PROTOCOL_NONE;
        hdr->HeaderExtLength = 0;  // Length is the min 8 bytes.
        hdr->Pad0 = 0;     
        hdr->Pad1 = 0;
        hdr->Pad2 = 0;
        
        // Save pointer to this header's next header value.
        tmp_hdr = Buf;

        // Increment buffer pointer.
        Buf += sizeof(ExtensionHeaderPadded);
        *Len += sizeof(ExtensionHeaderPadded);
        
        // Overwrite previous and make this the IP->NextHeader.
        proto = IP_PROTOCOL_HOP_BY_HOP;
    } 
    if (TypeFlag & DEST) {
        ExtensionHeader *hdr;
        OptionHeader *pad;
        int ExtLen = 0;
        int OptLen, PadBytes;
        

        // Create a minimum length, no-content, Destination Options Header.
        hdr = (ExtensionHeader*) Buf;        
        hdr->NextHeader = IP_PROTOCOL_NONE;
        hdr->HeaderExtLength = 0;  // Length is the min 8 bytes.

        // Increment buffer pointer.
        OptLen = sizeof ExtensionHeader;
        Buf += OptLen;
        *Len += OptLen;
        ExtLen += OptLen;

        // fill in the requested destination options.
        while (DestOpts->Next) {
            DestOpts = DestOpts->Next;
            OptLen = DestOpts->GenericOptHdr.DataLength + sizeof OptionHeader
                            - DestOpts->TotalSubOptLength;
            memcpy(Buf,&DestOpts->GenericOptHdr, OptLen);

            // Increment buffer pointer.
            Buf += OptLen;
            *Len += OptLen;
            ExtLen += OptLen;
            
            // 
            // Copy any sub-options. 
            //
            SubOptHdr = DestOpts->SubOpts;
            while (SubOptHdr) {
                OptLen = SubOptHdr->GenericSubOptHdr->DataLength + sizeof SubOptionHeader;
                memcpy(Buf,SubOptHdr->GenericSubOptHdr, OptLen);
           
                // Increment buffer pointer.
                Buf += OptLen;
                *Len += OptLen;
                ExtLen += OptLen;

                SubOptHdr = SubOptHdr->Next;
            }
        }

        // 
        // Now pad to an length divisible by EXT_LEN_UNIT.
        //
        PadBytes = ExtLen % EXT_LEN_UNIT;
        if (PadBytes > 0)
            PadBytes = EXT_LEN_UNIT - PadBytes;
        if (PadBytes == 1) {
            //
            // Add a 1 byte pad option.
            //
            *Buf = '\0';
        } else if(PadBytes > 1) {
             pad = (OptionHeader *)Buf;
             pad->Type = OPT6_PAD_N;
             pad->DataLength = PadBytes - sizeof OptionHeader;
             memset(Buf + sizeof OptionHeader,0,pad->DataLength);
        }

        Buf += PadBytes;
        *Len += PadBytes;
        ExtLen += PadBytes;

        // set the extension header length.
        hdr->HeaderExtLength = ExtLen/EXT_LEN_UNIT - 1;

        // Set DEST_OPT as next header in the previous header.
        if (tmp_hdr) {
            *tmp_hdr = IP_PROTOCOL_DEST_OPTS;
        }
        tmp_hdr = (uchar *)hdr;

        // If protocol not yet set, make this the IP->NextHeader.
        if (proto == 1) {
            proto = IP_PROTOCOL_DEST_OPTS;
        }
    } 
    if ((TypeFlag & FRAG) || (TypeFlag & FRAG0)) {
        FragmentHeader *hdr;
        
        hdr = (FragmentHeader*) Buf;        
        hdr->NextHeader = IP_PROTOCOL_NONE;
        hdr->Reserved = 0;
        if (TypeFlag & FRAG) {
            hdr->OffsetFlag = htons(1);   // indicate more fragments.
        } else {
            hdr->OffsetFlag = 0;              // indicate last fragment.
        }
        hdr->Id = Id++;

        // Set last header's next header pointer to this header type.
        if (tmp_hdr) {
            *tmp_hdr = IP_PROTOCOL_FRAGMENT;
        }
        tmp_hdr = Buf;

        // If protocol not yet set, make this the IP->NextHeader.
        if (proto == 1) {
            proto = IP_PROTOCOL_FRAGMENT;
        }

        *Len += sizeof(FragmentHeader);

        // We don't increment the buffer ptr here, leave it pointing
        // to the beginning of the Fragment header.
    } 


    // Introduce error into message.
    //
    // REVIEW: Currently, only paramenter problem errors are generated.
    //
    if (ErrorType & PARAM_PROB) {
        *tmp_hdr = (uchar) IP_PROTOCOL_BOGUS;
    }

    return proto;
}

//* CreateICMPMsg
uint
CreateICMPMsg(uchar *Buf, ushort TypeFlag, ushort ErrorType, int *Len)
{
    ICMPv6Header Hdr;
    uint proto;

#if 0    
    // Just create ICMP error message.
    Hdr = (ICMPv6Header *)((uchar *)TempBuf);
#endif

    return IP_PROTOCOL_NONE;
}


// * GetProtoFunc
//
ProtoTblStruct *GetProtoFunc(char *proto_name)
{
    ProtoTblStruct *p;

    for(p = ProtocolTbl; p->Name; p++) {
        //printf("getprotofunc: %s %s\n", proto_name, p->Name);
        if (strcmp(proto_name, p->Name) == 0)
            return  p;
    }
    return NULL;
}

BOOLEAN
param(
    char **strarg,
    char **argv,
    int argc,
    int current)
{
    if (current == (argc - 1)) {
        printf("A value must be supplied for option %s.\n", argv[current]);
        return(FALSE);
    }
    *strarg = argv[current+1];
    //printf("param: %s \n", *strarg);
    return TRUE;
}

BOOLEAN verify_type(char *type, ushort *Flag)
{
    MesgTypeTblStruct *m;
    
    for(m = MesgTypeTbl; m->Name0; m++) {
        if ((strcmp(type, m->Name0) == 0) || (strcmp(type, m->Name1) == 0)) {
            *Flag |= m->Value;
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN verify_error(char *type, ushort *Flag)
{
    ErrorTypeTblStruct *m;
    
    for(m = ErrorTypeTbl; m->Name0; m++) {
        if ((strcmp(type, m->Name0) == 0) || (strcmp(type, m->Name1) == 0)) {
            *Flag |= m->Value;
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN GetNextField(char **Arg, char **Dest)
{
    int Len;
    char *Comma;
    // 
    // Extract the next field from the given argument string.
    // If successful, move Arg pointer.
    //
    if ((Comma = strchr(*Arg, ',')) == NULL) {
        Len = strlen(*Arg);
        if (Len == 0)
            // No more fields to extract.
            return FALSE;
        // This is the last field.
        *Dest = *Arg;
        *Arg += Len;
        return TRUE;
    }

    // Found a comma.  Replace with NULL and return a pointer to the field.
    *Comma = '\0';
    *Dest = *Arg;
    *Arg = Comma+1;

    return TRUE;
}

void ProcessSubOpts(OptionsList *CurrOpt, char **DestOpt)
{
    int Count, Len;
    struct sockaddr_in6 TmpAddr;
    char *TmpField, *TmpArg, *TmpArgSave; 
    IPv6UniqueIdSubOption *UniqueId;
    IPv6HomeAgentsListSubOption *HomeAgents;
    struct in6_addr *HomeAgentAddr;
    SubOptionsList **SubOptPtr;

    SubOptPtr = &CurrOpt->SubOpts;
    while (GetNextField(DestOpt, &TmpField)) {
        switch (TmpField[1]) {
        case 'u':
            //
            // unique id sub-option.
            //
            if (!GetNextField(DestOpt, &TmpField))
                err("poorly formed unique id sub-option.");

            *SubOptPtr = (SubOptionsList *) 
                malloc(sizeof (SubOptionsList) + sizeof (*UniqueId));
            if (*SubOptPtr == NULL)
                err("can't alloc mem for sub-option");
            (*SubOptPtr)->Next = NULL;
            (*SubOptPtr)->GenericSubOptHdr = (SubOptionHeader *) (*SubOptPtr + 1);

            UniqueId = (IPv6UniqueIdSubOption *) (*SubOptPtr)->GenericSubOptHdr;
            UniqueId->Type = SUBOPT6_UNIQUE_ID;
            UniqueId->Length = 2;
            UniqueId->UniqueId = htons(atoi(TmpField));
            
            //
            // Update the destination option length.
            //
            CurrOpt->GenericOptHdr.DataLength += sizeof (*UniqueId);
            CurrOpt->TotalSubOptLength += sizeof (*UniqueId);
            SubOptPtr = &(*SubOptPtr)->Next;
            break;

        case 'h':
            //
            // Home agents list sub-option.
            //
                
            //
            // We need to count the number of addresses to allow for 
            // in the sub-option.
            //
            Len = strlen(*DestOpt);
            TmpArg = (char *) malloc(Len);
            TmpArgSave = TmpArg;
            if (TmpArg == NULL)
                err("cannot malloc memory for TmpArg");
            memcpy(TmpArg, *DestOpt, Len+1);
            
            Count = 0;
            // 
            // Loop until we reach the end or another sub-option.
            //
            while (TmpArg[0] != '-') {
                if (!GetNextField(&TmpArg, &TmpField))
                    //
                    // If we've reached the last argument.
                    //
                    break;
                Count++;
            }
            free(TmpArgSave);

            *SubOptPtr = (SubOptionsList *) 
                malloc(sizeof (SubOptionsList) + sizeof (*HomeAgents)
                + Count * sizeof (struct in6_addr));
            (*SubOptPtr)->Next = NULL;
            (*SubOptPtr)->GenericSubOptHdr = (SubOptionHeader *) (*SubOptPtr + 1);

            HomeAgents = (IPv6HomeAgentsListSubOption *) (*SubOptPtr)->GenericSubOptHdr;
            HomeAgents->Type = SUBOPT6_HOME_AGENTS_LIST;
            HomeAgents->Length = sizeof (struct in6_addr) * Count;
            HomeAgentAddr = (struct in6_addr *) (HomeAgents + 1);
           
            // 
            // Now copy all of the addresses into the sub-option.
            //
            while ((*DestOpt)[0] != '-') {
                if (!GetNextField(DestOpt, &TmpField))
                    //
                    // If we've reached the last argument.
                    //
                    break;

                if (!parse_addr(TmpField, &TmpAddr)) 
                    err("bad home agent address");

                memcpy(HomeAgentAddr, &TmpAddr.sin6_addr, sizeof (struct in6_addr));
                HomeAgentAddr++;
            }
                
            //
            // Update the destination option length.
            //
            CurrOpt->GenericOptHdr.DataLength += sizeof (*HomeAgents) + HomeAgents->Length;
            CurrOpt->TotalSubOptLength += sizeof (*HomeAgents) + HomeAgents->Length;
            SubOptPtr = &(*SubOptPtr)->Next;
            break;

        default:
            err("unrecoginized sub-option");
        }
    }
}
