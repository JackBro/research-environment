// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1993-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Bloodhound parser Internet Protocol version 6
// Bloodhound parser Internet Control Message Protocol version 6
// Based on RFCs 1885, 1970.
//
// Bugs/deficiencies:
//      Does not parse embedded packet data.
//

#include "tcpip6.h"
#include "ip6.h"
#include "icmp6.h"

//=============================================================================
//  ICMP6 property database.
//=============================================================================

enum ICMP6_PROP_IDS
{
    ICMP6_SUMMARY=0,
    ICMP6_TYPE,
    ICMP6_CODE,
    ICMP6_CHECKSUM,

    ICMP6_DATA,
    ICMP6_ERROR,

    // Properties that depend on the type of message

    ICMP6_CODE_DESTINATION_UNREACHABLE,
    ICMP6_UNUSED,
    ICMP6_IDENTIFIER,
    ICMP6_SEQUENCE_NUMBER,
    ICMP6_MTU,
    ICMP6_CODE_TIME_EXCEEDED,
    ICMP6_CODE_PARAMETER_PROBLEM,
    ICMP6_POINTER,
    ICMP6_MAXIMUM_RESPONSE_DELAY,
    ICMP6_MULTICAST_ADDRESS,
    ICMP6_RESERVED,
    ICMP6_TARGET_ADDRESS,
    ICMP6_NEIGHBOR_FLAGS,
    ICMP6_DEST_ADDRESS,
    ICMP6_CUR_HOP_LIMIT,
    ICMP6_ROUTER_ADVERTISEMENT_FLAGS,
    ICMP6_ROUTER_LIFETIME,
    ICMP6_REACHABLE_TIME,
    ICMP6_RETRANS_TIMER,

    // Neighbor discovery options
    ICMP6_ND_OPTIONS,
    ICMP6_UNRECOGNIZED_OPTION,
    ICMP6_OPTION_TYPE,
    ICMP6_OPTION_LENGTH,
    ICMP6_OPTION_DATA,
    ICMP6_SOURCE_LL_ADDR,
    ICMP6_TARGET_LL_ADDR,
    ICMP6_REDIRECT_DATA,
    ICMP6_PREFIX_LENGTH,
    ICMP6_SITE_PREFIX_LENGTH,
    ICMP6_PREFIX_FLAGS,
    ICMP6_VALID_LIFETIME,
    ICMP6_PREFERRED_LIFETIME,
    ICMP6_PREFIX,
};

PROPERTYINFO ICMP6Database[] =
{
    {   //  ICMP6_SUMMARY
        0,0,
        "Summary",
        "ICMP 6 packet",
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE,
        IP6FormatString},

    {   // ICMP6_TYPE
        0,0, 
        "Type",     
        "Message Type.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_LABELED_SET, 
        &ICMP6TypesSet,
        FORMAT_BUFFER_SIZE, 
        IP6FormatNamedByte},

    {   // ICMP6_CODE
        0,0, 
        "Code",
        "Message Code.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_CHECKSUM
        0,0, 
        "Checksum",
        "Checksum.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatChecksum},

    {   // ICMP6_DATA
        0,0, 
        "Data",
        "Data.",
        PROP_TYPE_RAW_DATA,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_ERROR
        0,0, 
        "Error",
        "Error.",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_CODE_DESTINATION_UNREACHABLE
        0,0, 
        "Code",     
        "Message Code.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_LABELED_SET, 
        &ICMP6ErrorCodeSets[ICMP6_DESTINATION_UNREACHABLE],
        FORMAT_BUFFER_SIZE, 
        IP6FormatNamedByte},

    {   // ICMP6_UNUSED
        0,0, 
        "Unused",
        "Unused.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_IDENTIFIER
        0,0, 
        "Identifier",
        "Identifier.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_SEQUENCE_NUMBER
        0,0, 
        "Sequence Number",
        "Sequence Number.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_MTU
        0,0, 
        "MTU",
        "Maximum Transmission Unit.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_CODE_TIME_EXCEEDED
        0,0, 
        "Code",     
        "Message Code.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_LABELED_SET, 
        &ICMP6ErrorCodeSets[ICMP6_TIME_EXCEEDED],
        FORMAT_BUFFER_SIZE, 
        IP6FormatNamedByte},

    {   // ICMP6_CODE_PARAMETER_PROBLEM
        0,0, 
        "Code",     
        "Message Code.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_LABELED_SET, 
        &ICMP6ErrorCodeSets[ICMP6_PARAMETER_PROBLEM],
        FORMAT_BUFFER_SIZE, 
        IP6FormatNamedByte},

    {   // ICMP6_POINTER
        0,0, 
        "Pointer",
        "Byte offset of the problem.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_MAXIMUM_RESPONSE_DELAY
        0,0, 
        "Maximum Response Delay",
        "Maximum Response Delay.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_MULTICAST_ADDRESS
        0,0, 
        "Multicast Address",     
        "Multicast Address.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // ICMP6_RESERVED
        0,0, 
        "Reserved",
        "Reserved.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_TARGET_ADDRESS
        0,0, 
        "Target Address",     
        "Target Address.", 
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // IP6_NEIGHBOR_FLAGS
        0,0,
        "Flags",
        "Neighbor Advertisement Flags.",
        PROP_TYPE_DWORD,
        PROP_QUAL_FLAGS,
        &ICMP6NeighborFlagsSet,
        FORMAT_BUFFER_SIZE * 3,
        FormatPropertyInstance},

    {   // IP6_DEST_ADDRESS
        0,0, 
        "Destination Address",     
        "Destination Address.", 
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // ICMP6_CUR_HOP_LIMIT
        0,0, 
        "Current Hop Limit",
        "Current Hop Limit.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_ROUTER_ADVERTISEMENT_FLAGS
        0,0,
        "Flags",
        "Router Advertisement Flags.",
        PROP_TYPE_BYTE,
        PROP_QUAL_FLAGS,
        &ICMP6RouterAdvertisementFlagsSet,
        FORMAT_BUFFER_SIZE * 2,
        FormatPropertyInstance},

    {   // ICMP6_ROUTER_LIFETIME
        0,0, 
        "Router Lifetime",
        "Router Lifetime.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_REACHABLE_TIME
        0,0, 
        "Reachable Time",
        "Reachable Time.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_RETRANS_TIMER
        0,0, 
        "Retransmission Timer",
        "Retransmission Timer.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_ND_OPTIONS
        0,0, 
        "Neighbor Discovery Options",
        "Neighbor Discovery Options.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_UNRECOGNIZED_OPTION
        0,0, 
        "Unrecognized option",
        "Unrecognized option.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_OPTION_TYPE
        0,0, 
        "Type",
        "Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_OPTION_LENGTH
        0,0, 
        "Length",
        "Option Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_OPTION_DATA
        0,0, 
        "Data",
        "Option Data.",
        PROP_TYPE_RAW_DATA,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_SOURCE_LL_ADDR
        0,0, 
        "Source Link-level Address",     
        "Source Link-level Address.", 
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_TARGET_LL_ADDR
        0,0, 
        "Target Link-level Address",
        "Target Link-level Address.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_REDIRECT_DATA
        0,0, 
        "Redirected Packet",
        "Redirected Packet.",
        PROP_TYPE_RAW_DATA,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_PREFIX_LENGTH
        0,0, 
        "Prefix Length",
        "Prefix Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_SITE_PREFIX_LENGTH
        0,0, 
        "Site Prefix Length",
        "Site Prefix Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_PREFIX_FLAGS
        0,0,
        "Prefix Flags",
        "Prefix Flags.",
        PROP_TYPE_BYTE,
        PROP_QUAL_FLAGS,
        &ICMP6PrefixFlagsSet,
        FORMAT_BUFFER_SIZE * 3,
        FormatPropertyInstance},

    {   // ICMP6_VALID_LIFETIME
        0,0, 
        "Valid Lifetime",
        "Prefix Valid Lifetime.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // ICMP6_PREFERRED_LIFETIME
        0,0, 
        "Preferred Lifetime",
        "Prefix Preferred Lifetime.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_PREFIX
        0,0, 
        "Prefix",     
        "Prefix.", 
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},
};

DWORD nICMP6Properties = ((sizeof ICMP6Database) / PROPERTYINFO_SIZE);


//=============================================================================
//  FUNCTION: ICMP6Register()
//
//=============================================================================

VOID WINAPI ICMP6Register(HPROTOCOL hICMP6Protocol)
{
    DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hICMP6Protocol, nICMP6Properties);

    for(i = 0; i < nICMP6Properties; ++i)
    {
        AddProperty(hICMP6Protocol, &ICMP6Database[i]);
    }
}


//=============================================================================
//  FUNCTION: Deregister()
//
//=============================================================================

VOID WINAPI ICMP6Deregister(HPROTOCOL hICMP6Protocol)
{
    DestroyPropertyDatabase(hICMP6Protocol);
}


//=============================================================================
//  FUNCTION: ICMP6RecognizeFrame()
//
//=============================================================================

LPBYTE WINAPI ICMP6RecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          ICMP6Frame,                   //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         pInstData)                  //... Next protocol instance data.
{
    DWORD Length;

    // We are careful not to look beyond the IP6 packet.
    // (There might be padding beyond it.)

    Length = GetLengthFromInstData(*pInstData);
    if (BytesLeft > Length)
        BytesLeft = Length;

    if (BytesLeft >= sizeof(ICMP6_HEADER)) {

        // Claim the entire packet to stop parsing.

        *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
        return ICMP6Frame + BytesLeft;
    }
    else {
        // We do not have a complete header.

        *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return ICMP6Frame;
    }
}


//=============================================================================
//  FUNCTION: ICMP6AttachProperties()
//
//  We attach an error property when we notice something wrong,
//  because Bloodhound currently ignores IFLAG_ERROR.
//
//=============================================================================

LPBYTE WINAPI ICMP6AttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    ICMP6Frame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{
    LPICMP6_HEADER pICMP6 = ( LPICMP6_HEADER ) ICMP6Frame;
    LPBYTE pRawData = NULL;
    LPBYTE pNDOptions = NULL;
    LPSTR pErrorMsg = NULL;
    DWORD Length;

    // We are careful not to look beyond the IP6 packet.
    // (There might be padding beyond it.)

    Length = GetLengthFromInstData(InstData);
    if (BytesLeft > Length)
        BytesLeft = Length;

    ICMP6AttachSummary(hFrame, pICMP6, BytesLeft);

    // We assume without checking that the previous protocol is IP6.

    ICMP6AttachChecksum(hFrame, pICMP6, (LPIP6_HEADER)(Frame + nPreviousProtocolOffset), InstData, BytesLeft);

    AttachPropertyInstance(hFrame,
                   ICMP6Database[ICMP6_TYPE].hProperty,
                   sizeof (pICMP6->Type),
                   &pICMP6->Type,
                   0, 1, 0);    // 1 is the level of indentation

    switch (pICMP6->Type) {
    case ICMP6_DESTINATION_UNREACHABLE: {
        LPICMP6_ERROR_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE_DESTINATION_UNREACHABLE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "destination-unreachable packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ERROR_HEADER) pICMP6;

        if (pHdr->Parameter != 0)
            pErrorMsg = "unused field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_UNUSED].hProperty,
                       sizeof (pHdr->Parameter),
                       &pHdr->Parameter,
                       0, 1, pErrorMsg != NULL);

        pRawData = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_PACKET_TOO_BIG: {
        LPICMP6_ERROR_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "packet-too-big packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ERROR_HEADER) pICMP6;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_MTU].hProperty,
                       sizeof (pHdr->Parameter),
                       &pHdr->Parameter,
                       0, 1, IFLAG_SWAPPED);

        pRawData = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_TIME_EXCEEDED: {
        LPICMP6_ERROR_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE_TIME_EXCEEDED].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "time-exceeded packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ERROR_HEADER) pICMP6;

        if (pHdr->Parameter != 0)
            pErrorMsg = "unused field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_UNUSED].hProperty,
                       sizeof (pHdr->Parameter),
                       &pHdr->Parameter,
                       0, 1, pErrorMsg != NULL);

        pRawData = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_PARAMETER_PROBLEM: {
        LPICMP6_ERROR_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE_PARAMETER_PROBLEM].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "parameter-problem packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ERROR_HEADER) pICMP6;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_POINTER].hProperty,
                       sizeof (pHdr->Parameter),
                       &pHdr->Parameter,
                       0, 1, IFLAG_SWAPPED);

        pRawData = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_ECHO_REQUEST:
    case ICMP6_ECHO_REPLY: {
        LPICMP6_ECHO_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "echo packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ECHO_HEADER) pICMP6;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_IDENTIFIER].hProperty,
                       sizeof (pHdr->Identifier),
                       &pHdr->Identifier,
                       0, 1, IFLAG_SWAPPED);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_SEQUENCE_NUMBER].hProperty,
                       sizeof (pHdr->SequenceNumber),
                       &pHdr->SequenceNumber,
                       0, 1, IFLAG_SWAPPED);

        pRawData = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_GROUP_MEMBERSHIP_QUERY:
    case ICMP6_GROUP_MEMBERSHIP_REPORT:
    case ICMP6_GROUP_MEMBERSHIP_REDUCTION: {
        LPICMP6_GROUP_MEMBERSHIP_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "group-membership packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_GROUP_MEMBERSHIP_HEADER) pICMP6;

        if ((pICMP6->Type != ICMP6_GROUP_MEMBERSHIP_QUERY) &&
            (pHdr->MaximumResponseDelay != 0))
            pErrorMsg = "delay field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_MAXIMUM_RESPONSE_DELAY].hProperty,
                       sizeof (pHdr->MaximumResponseDelay),
                       &pHdr->MaximumResponseDelay,
                       0, 1, IFLAG_SWAPPED | (pErrorMsg != NULL));

        if (pHdr->Unused != 0)
            pErrorMsg = "unused field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_UNUSED].hProperty,
                       sizeof (pHdr->Unused),
                       &pHdr->Unused,
                       0, 1, pErrorMsg != NULL);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_MULTICAST_ADDRESS].hProperty,
                       sizeof (pHdr->MulticastAddress),
                       &pHdr->MulticastAddress,
                       0, 1, 0);

        if (BytesLeft > sizeof *pHdr) {
            pErrorMsg = "group-membership packet too big";
            pRawData = ICMP6Frame + sizeof *pHdr;
        }
        break;
    }

    case ICMP6_ROUTER_SOLICITATION: {
        LPICMP6_ROUTER_SOLICITATION_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "router-solication packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ROUTER_SOLICITATION_HEADER) pICMP6;

        if (pHdr->Reserved != 0)
            pErrorMsg = "reserved field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_RESERVED].hProperty,
                       sizeof (pHdr->Reserved),
                       &pHdr->Reserved,
                       0, 1, pErrorMsg != NULL);

        pNDOptions = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_ROUTER_ADVERTISEMENT: {
        LPICMP6_ROUTER_ADVERTISEMENT_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);    // 1 is the level of indentation

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "router-advertisement packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_ROUTER_ADVERTISEMENT_HEADER) pICMP6;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CUR_HOP_LIMIT].hProperty,
                       sizeof (pHdr->CurHopLimit),
                       &pHdr->CurHopLimit,
                       0, 1, 0);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_ROUTER_ADVERTISEMENT_FLAGS].hProperty,
                       sizeof (pHdr->Flags),
                       &pHdr->Flags,
                       0, 1, 0);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_ROUTER_LIFETIME].hProperty,
                       sizeof (pHdr->RouterLifetime),
                       &pHdr->RouterLifetime,
                       0, 1, IFLAG_SWAPPED);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_REACHABLE_TIME].hProperty,
                       sizeof (pHdr->ReachableTime),
                       &pHdr->ReachableTime,
                       0, 1, IFLAG_SWAPPED);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_RETRANS_TIMER].hProperty,
                       sizeof (pHdr->RetransTimer),
                       &pHdr->RetransTimer,
                       0, 1, IFLAG_SWAPPED);

        pNDOptions = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_NEIGHBOR_SOLICITATION:
    case ICMP6_NEIGHBOR_ADVERTISEMENT: {
        LPICMP6_NEIGHBOR_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "neighbor packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_NEIGHBOR_HEADER) pICMP6;

        if (pICMP6->Type == ICMP6_NEIGHBOR_SOLICITATION) {
            if (pHdr->Flags != 0)
                pErrorMsg = "reserved field not zero";

            AttachPropertyInstance(hFrame,
                           ICMP6Database[ICMP6_RESERVED].hProperty,
                           sizeof (pHdr->Flags),
                           &pHdr->Flags,
                           0, 1, pErrorMsg != NULL);
        }
        else { // ICMP6_NEIGHBOR_ADVERTISEMENT
            AttachPropertyInstance(hFrame,
                           ICMP6Database[ICMP6_NEIGHBOR_FLAGS].hProperty,
                           sizeof (pHdr->Flags),
                           &pHdr->Flags,
                           0, 1, IFLAG_SWAPPED);
        }

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_TARGET_ADDRESS].hProperty,
                       sizeof (pHdr->TargetAddress),
                       &pHdr->TargetAddress,
                       0, 1, 0);

        pNDOptions = ICMP6Frame + sizeof *pHdr;
        break;
    }

    case ICMP6_REDIRECT: {
        LPICMP6_REDIRECT_HEADER pHdr;

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);

        if (BytesLeft < sizeof *pHdr) {
            pErrorMsg = "redirect packet too small";
            goto BadPacketData;
        }

        pHdr = (LPICMP6_REDIRECT_HEADER) pICMP6;

        if (pHdr->Reserved != 0)
            pErrorMsg = "reserved field not zero";

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_RESERVED].hProperty,
                       sizeof (pHdr->Reserved),
                       &pHdr->Reserved,
                       0, 1, pErrorMsg != NULL);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_TARGET_ADDRESS].hProperty,
                       sizeof (pHdr->TargetAddress),
                       &pHdr->TargetAddress,
                       0, 1, 0);

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_DEST_ADDRESS].hProperty,
                       sizeof (pHdr->DestAddress),
                       &pHdr->DestAddress,
                       0, 1, 0);

        pNDOptions = ICMP6Frame + sizeof *pHdr;
        break;
    }

    default:
        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_CODE].hProperty,
                       sizeof (pICMP6->Code),
                       &pICMP6->Code,
                       0, 1, 0);
        // fall-through...

    BadPacketData:
        pRawData = ICMP6Frame + sizeof(ICMP6_HEADER);
        break;
    }

    if (pNDOptions != NULL) {
        LPBYTE pNDOptionsEnd = ICMP6Frame + BytesLeft;
        LPICMP6_ND_OPTION_HEADER pOption;
        LPSTR pOptErrorMsg;
        DWORD OptLevel = 1;

#if 0
        // Put all options under a single property
        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_ND_OPTIONS].hProperty,
                       pNDOptionsEnd - pNDOptions,
                       pNDOptions,
                       0, 1, 0);    // 1 is the level of indentation
        OptLevel++;
#endif

        while (pNDOptions < pNDOptionsEnd) {
            pOption = (LPICMP6_ND_OPTION_HEADER) pNDOptions;

            if ((pNDOptions + sizeof *pOption > pNDOptionsEnd) ||
                (pOption->Length == 0) ||
                (pNDOptions + pOption->Length * 8 > pNDOptionsEnd)) {

                pOptErrorMsg = "malformed option";
                pNDOptions = pNDOptionsEnd;
                goto BadOption;
            }

            pNDOptions += pOption->Length * 8;

            switch (pOption->Type) {
            case ICMP6_ND_OPTION_SOURCE_LL_ADDR:
            case ICMP6_ND_OPTION_TARGET_LL_ADDR: {
                DWORD Prop;

                if (pOption->Type == ICMP6_ND_OPTION_SOURCE_LL_ADDR)
                    Prop = ICMP6_SOURCE_LL_ADDR;
                else
                    Prop = ICMP6_TARGET_LL_ADDR;

                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[Prop].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               pNDOptions - (LPBYTE)(pOption + 1),
                               (LPBYTE)(pOption + 1),
                               0, OptLevel, 0);

                ICMP6AttachNDOption(hFrame, pOption, OptLevel+1);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[Prop].hProperty,
                               pNDOptions - (LPBYTE)(pOption + 1),
                               (LPBYTE)(pOption + 1),
                               0, OptLevel+1, 0);
                break;
            }

            case ICMP6_ND_OPTION_PREFIX: {
                LPICMP6_ND_OPTION_PREFIX_HEADER pHdr =
                    (LPICMP6_ND_OPTION_PREFIX_HEADER) pOption;

                if (pNDOptions != (LPBYTE)(pHdr + 1)) {
                    pOptErrorMsg = "prefix option has bad length";
                    goto BadOption;
                }

                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[ICMP6_PREFIX].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               sizeof (pHdr->Prefix),
                               &pHdr->Prefix,
                               0, OptLevel, 0);

                ICMP6AttachNDOption(hFrame, pOption, OptLevel+1);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_PREFIX_LENGTH].hProperty,
                               sizeof (pHdr->PrefixLength),
                               &pHdr->PrefixLength,
                               0, OptLevel+1, 0);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_SITE_PREFIX_LENGTH].hProperty,
                               sizeof (pHdr->SitePrefixLength),
                               &pHdr->SitePrefixLength,
                               0, OptLevel+1,
                               ((pHdr->SitePrefixLength != 0) &&
                                ((pHdr->Flags & 0x10) == 0)));

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_PREFIX_FLAGS].hProperty,
                               sizeof (pHdr->Flags),
                               &pHdr->Flags,
                               0, OptLevel+1, 0);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_VALID_LIFETIME].hProperty,
                               sizeof (pHdr->ValidLifetime),
                               &pHdr->ValidLifetime,
                               0, OptLevel+1, IFLAG_SWAPPED);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_PREFERRED_LIFETIME].hProperty,
                               sizeof (pHdr->PreferredLifetime),
                               &pHdr->PreferredLifetime,
                               0, OptLevel+1, IFLAG_SWAPPED);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_RESERVED].hProperty,
                               sizeof (pHdr->Reserved),
                               &pHdr->Reserved,
                               0, OptLevel+1,
                               ((pHdr->Reserved[0] != 0) ||
                                (pHdr->Reserved[1] != 0) ||
                                (pHdr->Reserved[2] != 0)));

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_PREFIX].hProperty,
                               sizeof (pHdr->Prefix),
                               &pHdr->Prefix,
                               0, OptLevel+1, 0);
                break;
            }

            case ICMP6_ND_OPTION_REDIRECT:
                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[ICMP6_REDIRECT_DATA].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               pNDOptions - ((LPBYTE)pOption + 8),
                               (LPBYTE)pOption + 8,
                               0, OptLevel, 0);

                ICMP6AttachNDOption(hFrame, pOption, OptLevel+1);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_RESERVED].hProperty,
                               8 - sizeof *pOption,
                               (LPBYTE)(pOption + 1),
                               0, OptLevel+1, 0);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_REDIRECT_DATA].hProperty,
                               pNDOptions - ((LPBYTE)pOption + 8),
                               (LPBYTE)pOption + 8,
                               0, OptLevel+1, 0);
                break;

            case ICMP6_ND_OPTION_MTU: {
                LPICMP6_ND_OPTION_MTU_HEADER pHdr =
                    (LPICMP6_ND_OPTION_MTU_HEADER) pOption;

                if (pNDOptions != (LPBYTE)(pHdr + 1)) {
                    pOptErrorMsg = "MTU option has bad length";
                    goto BadOption;
                }

                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[ICMP6_MTU].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               sizeof (pHdr->MTU),
                               &pHdr->MTU,
                               0, OptLevel, IFLAG_SWAPPED);

                ICMP6AttachNDOption(hFrame, pOption, OptLevel+1);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_RESERVED].hProperty,
                               sizeof (pHdr->Reserved),
                               &pHdr->Reserved,
                               0, OptLevel+1, (pHdr->Reserved != 0));

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_MTU].hProperty,
                               sizeof (pHdr->MTU),
                               &pHdr->MTU,
                               0, OptLevel+1, IFLAG_SWAPPED);
                break;
            }

            default:
                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[ICMP6_UNRECOGNIZED_OPTION].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               sizeof (pOption->Type),
                               &pOption->Type,
                               0, OptLevel, 0);

                ICMP6AttachNDOption(hFrame, pOption, OptLevel+1);

                AttachPropertyInstance(hFrame,
                               ICMP6Database[ICMP6_OPTION_DATA].hProperty,
                               pNDOptions - (LPBYTE)(pOption + 1),
                               (LPBYTE)(pOption + 1),
                               0, OptLevel+1, 0);
                break;

            BadOption:
                AttachPropertyInstanceEx(hFrame,
                               ICMP6Database[ICMP6_ERROR].hProperty,
                               pNDOptions - (LPBYTE)pOption,
                               (LPBYTE)pOption,
                               strlen(pOptErrorMsg) + 1,
                               pOptErrorMsg,
                               0, OptLevel, 1);
                break;
            }
        }
    }

    if ((pRawData != NULL) && (pRawData < ICMP6Frame + BytesLeft)) {

        AttachPropertyInstance(hFrame,
                       ICMP6Database[ICMP6_DATA].hProperty,
                       ICMP6Frame + BytesLeft - pRawData,
                       pRawData,
                       0, 1, 0);    // 1 is the level of indentation
    }

    if (pErrorMsg != NULL) {
        // Attach a property to display the error message.

        AttachPropertyInstanceEx(hFrame,
                       ICMP6Database[ICMP6_ERROR].hProperty,
                       BytesLeft,
                       ICMP6Frame,
                       strlen(pErrorMsg) + 1,
                       pErrorMsg,
                       0, 1, 1);
    }

    return NULL;
}


VOID WINAPIV ICMP6AttachSummary(HFRAME hFrame, LPICMP6_HEADER pICMP6, DWORD BytesLeft)
{
    char Text[FORMAT_BUFFER_SIZE];
    char *p = Text;
    char *s;

    s = LookupByteSetString(&ICMP6TypesSet, pICMP6->Type);
    if (s != NULL)
        p += sprintf(p, "%s", s);
    else
        p += sprintf(p, "Type = %u", pICMP6->Type);

    switch (pICMP6->Type) {
    case ICMP6_DESTINATION_UNREACHABLE:
    case ICMP6_PACKET_TOO_BIG:
    case ICMP6_TIME_EXCEEDED:
    case ICMP6_PARAMETER_PROBLEM: {
        LPICMP6_ERROR_HEADER pHdr = (LPICMP6_ERROR_HEADER) pICMP6;
        DWORD nFrame;

        if (BytesLeft >= sizeof *pHdr) {

            // ICMP6_PACKET_TOO_BIG is "always" code 0

            if ((pICMP6->Type != ICMP6_PACKET_TOO_BIG) ||
                (pICMP6->Code != 0)) {
                s = LookupByteSetString(&ICMP6ErrorCodeSets[pICMP6->Type],
                                        pICMP6->Code);
                if (s != NULL)
                    p += sprintf(p, " (%s)", s);
                else
                    p += sprintf(p, " (Code = %u)", pICMP6->Code);
            }

            if (pICMP6->Type == ICMP6_PACKET_TOO_BIG)
                p += sprintf(p, " (MTU = %u)", ICMP6PacketTooBigMTU(pHdr));

            // Try to find the referenced frame.

            nFrame = ICMP6FindPreviousFrameNumber(hFrame,
                        (LPBYTE)(pHdr + 1), BytesLeft - sizeof *pHdr);
            if (nFrame != (DWORD) -1) {

                // Internal frame numbers are zero-based,
                // but they are one-based in the UI.
                p += sprintf(p, " - See frame %u", nFrame + 1);
            }
        }
        break;
    }

    case ICMP6_ECHO_REQUEST:
    case ICMP6_ECHO_REPLY: {
        LPICMP6_ECHO_HEADER pHdr = (LPICMP6_ECHO_HEADER) pICMP6;

        if (BytesLeft >= sizeof *pHdr) {

            p += sprintf(p, "; ID = %u, Seq = %u",
                         XCHG(pHdr->Identifier), XCHG(pHdr->SequenceNumber));
        }
        break;
    }

    case ICMP6_NEIGHBOR_SOLICITATION:
    case ICMP6_NEIGHBOR_ADVERTISEMENT: {
        LPICMP6_NEIGHBOR_HEADER pHdr = (LPICMP6_NEIGHBOR_HEADER) pICMP6;

        if (BytesLeft >= sizeof *pHdr) {

            p += sprintf(p, "; Target = ");
            IP6FormatAddressWorker(p, &pHdr->TargetAddress);
        }
        break;
    }
    }

    AttachPropertyInstanceEx(hFrame,
                ICMP6Database[ICMP6_SUMMARY].hProperty,
                BytesLeft,
                pICMP6,
                strlen(Text),
                Text,
                0, 0, 0);
}

VOID WINAPIV ICMP6AttachChecksum(HFRAME hFrame, LPICMP6_HEADER pICMP6, LPIP6_HEADER pIP6, DWORD InstData, DWORD BytesLeft)
{
    DWORD Length = GetLengthFromInstData(InstData);
    DWORD Fragged = GetInfoFromInstData(InstData);
    WORD Checksum;

    // We can only compute a checksum if we are looking
    // at a complete ICMP6 packet.

    if (Fragged || (Length > BytesLeft)) {
        // The property formatter will report
        // that the checksum could not be verified.
        Checksum = 0;
    }
    else {
        IP6_PSEUDO_HEADER PsuHdr;

        PsuHdr.SrcAddr = pIP6->SrcAddr;
        PsuHdr.DstAddr = pIP6->DstAddr;
        PsuHdr.PayloadLength = DXCHG(Length);
        PsuHdr.NextHeader = DXCHG(PROTO_ICMP6);

        Checksum = CalcChecksum(&PsuHdr, (LPBYTE)pICMP6, Length, offsetof(ICMP6_HEADER, Checksum));
    }

    AttachPropertyInstanceEx(hFrame,
                ICMP6Database[ICMP6_CHECKSUM].hProperty,
                sizeof (pICMP6->Checksum),
                &pICMP6->Checksum,
                sizeof Checksum,
                &Checksum,
                0, 1, IFLAG_SWAPPED);
}

//=============================================================================
//  FUNCTION: ICMP6AttachProperties()
//
//  We attach an error property when we notice something wrong,
//  because Bloodhound currently ignores IFLAG_ERROR.
//
//=============================================================================

VOID WINAPIV ICMP6AttachNDOption(HFRAME hFrame, LPICMP6_ND_OPTION_HEADER pOption, DWORD Level)
{
    AttachPropertyInstance(hFrame,
                   ICMP6Database[ICMP6_OPTION_TYPE].hProperty,
                   sizeof (pOption->Type),
                   &pOption->Type,
                   0, Level, 0);

    AttachPropertyInstance(hFrame,
                   ICMP6Database[ICMP6_OPTION_LENGTH].hProperty,
                   sizeof (pOption->Length),
                   &pOption->Length,
                   0, Level, 0);
}


//=============================================================================
//  FUNCTION: ICMP6FormatProperties()
//
//=============================================================================

DWORD WINAPI ICMP6FormatProperties(HFRAME         hFrame,
                                 LPBYTE         MacFrame,
                                 LPBYTE         FrameData,
                                 DWORD          nPropertyInsts,
                                 LPPROPERTYINST p)
{
    //=========================================================================
    //  Format each property in the property instance table.
    //
    //  The property-specific instance data was used to store the address of a
    //  property-specific formatting function so all we do here is call each
    //  function via the instance data pointer.
    //=========================================================================

    while (nPropertyInsts--)
    {
        ((FORMAT) p->lpPropertyInfo->InstanceData)(p);

        p++;
    }

    return BHERR_SUCCESS;
}

//=============================================================================
//  FUNCTION: ICMP6memcmp()
//
//  Just like the regular memcmp, EXCEPT that it works around
//  a bug in some ICMPv6 implementations. These buggy implementations
//  zero checksum fields in place, so then when they send an ICMP
//  containing the checksummed packet, the embedded data has a zero
//  checksum field instead of the original packet bytes.
//
//=============================================================================

int ICMP6memcmp(const void *buf1, const void *buf2, size_t count)
{
    const BYTE *pBuf1 = buf1;
    const BYTE *pBuf2 = buf2;
    BOOL SawZeroField = FALSE;

    while (count > 0) {
        BYTE Byte1 = *pBuf1++;
        BYTE Byte2 = *pBuf2++;
        count--;

        if (Byte1 != Byte2) {

            // Allow one 2-byte zero field in buf2.

            if (!SawZeroField && (Byte2 == 0) &&
                (count > 0) && (*pBuf2 == 0)) {

                SawZeroField = TRUE;
                pBuf1++;
                pBuf2++;
                count--;
                continue;
            }

            return (int)(Byte1 - Byte2);
        }
    }

    return 0;
}

//=============================================================================
//  FUNCTION: ICMP6FindPreviousFrameNumber()
//
//  Find the previous frame that contains the specified frame data.
//  Returns -1 if no frame found.
//
//=============================================================================

DWORD WINAPIV ICMP6FindPreviousFrameNumber(HFRAME hStart, LPBYTE Data, DWORD Length)
{
    DWORD Status;
    HPROTOCOL hNextProtocol;
    DWORD InstData = 0;

    // Check that we are looking at recognizable IP6 packet data.

    (void) IP6RecognizeFrame(hStart, Data, Data, 0, Length, NULL, 0,
                             &Status, &hNextProtocol, &InstData);

    if ((Status == PROTOCOL_STATUS_CLAIMED) ||
        (Status == PROTOCOL_STATUS_RECOGNIZED) ||
        (Status == PROTOCOL_STATUS_NEXT_PROTOCOL)) {
        DWORD nStart = GetFrameNumber(hStart);
        HFRAME hFrame = hStart;
        WORD Offset;

        while ((hFrame = FindPreviousFrame(hFrame,
                    "IP6", NULL, NULL, &Offset, nStart, 0)) != NULL) {
            LPBYTE Frame = ParserTemporaryLockFrame(hFrame);
            DWORD BytesAvail = GetFrameStoredLength(hFrame);

            Frame += Offset;
            BytesAvail -= Offset;

            if (BytesAvail > Length)
                BytesAvail = Length;

            if (ICMP6memcmp(Frame, Data, BytesAvail) == 0)
                return GetFrameNumber(hFrame);
        }
    }

    return (DWORD) -1;
}
