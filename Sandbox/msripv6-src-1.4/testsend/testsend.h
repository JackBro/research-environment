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

#ifndef TESTSEND_INCLUDED
#define TESTSEND_INCLUDED 1

#define IP_PROTOCOL_HOP_BY_HOP 0
#define IP_PROTOCOL_ROUTING 43
#define IP_PROTOCOL_FRAGMENT 44
#define IP_PROTOCOL_ICMPv6 58
#define IP_PROTOCOL_NONE   59
#define IP_PROTOCOL_DEST_OPTS 60

#define IP_PROTOCOL_BOGUS  30


//
// used to chain together options.
//
typedef struct SubOptionsList {
    struct SubOptionsList *Next;
    SubOptionHeader *GenericSubOptHdr;
} SubOptionsList;

typedef struct OptionsList {
    struct OptionsList *Next;
    uint TotalSubOptLength;
    SubOptionsList *SubOpts;
    OptionHeader GenericOptHdr;
} OptionsList;


//
// Protocol table.
typedef struct ProtoTblStruct {
    char *Name;
    uint ProtoValue;
    uint (*cfunc)(uchar *Buff, ushort TypeFlag, ushort ErrorType, int *Len, 
        OptionsList *DestOpts);
} ProtoTblStruct;


typedef struct MesTypeTblStruct {
    char *Name0;
    char *Name1;
    ushort Value;
    ushort IPProto;
} MesgTypeTblStruct;

typedef struct ErrorTypeTblStruct {
    char *Name0;
    char *Name1;
    ushort Value;
} ErrorTypeTblStruct;


BOOLEAN param(char **strarg, char **argv, int argc, int current);

// Extension header flags.
#define HOP   1
#define DEST  2
#define FRAG  4
#define FRAG0 8
#define RTG   16

// Error flags.
#define PARAM_PROB   0x1
#define TOO_BIG      0x2
#define DEST_UNREACH 0x4
#define TIME_EXCEED  0x8


ProtoTblStruct *GetProtoFunc(char *proto_name);
uint CreateICMPMsg(uchar *Buff, ushort TypeFlag, ushort ErrorType, int *Len);
uint CreateIPMsg(uchar *Buff, ushort TypeFlag, ushort ErrorType, int *Len,
            OptionsList *DestOpts);
void usage();
void err(char *s);
int parse_addr(char *s, struct sockaddr_in6 *sin6);
BOOLEAN verify_type(char *type, ushort *Flag);
BOOLEAN verify_error(char *type, ushort *Flag);
BOOLEAN param(char **strarg, char **argv, int argc, int current);
BOOLEAN GetNextField(char **Arg, char **Dest);
void ProcessSubOpts(OptionsList *CurrOpt, char **DestOpt);


//
// Generic Extension Header, with padding.
// 
typedef struct ExtensionHeaderPadded {
    uchar NextHeader;
    uchar HeaderExtLength;  // In 8-byte units, not counting first 8.
    uchar Pad0;            
    uchar Pad1;             // pad header to 4 bytes.
    uint  Pad2;             // pad header to 8 bytes. dangerous.
} ExtensionHeaderPadded;


#endif  // TESTSEND__INCLUDED
