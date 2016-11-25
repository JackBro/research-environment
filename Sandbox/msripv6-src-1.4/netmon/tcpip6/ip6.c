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
// Based on RFCs 1883, 1884.
//
// Bugs/deficiencies:
//      Does not handle Jumbo payload lengths.
//      (NB: TCP & UDP limit packet lengths to 64K.)
//

#include "tcpip6.h"
#include "ip6.h"


//=============================================================================
//  IP6 property database.
//=============================================================================

enum IP6_PROP_IDS
{
    IP6_SUMMARY=0,
    IP6_VERSION,
    IP6_CLASS,
    IP6_FLOW_LABEL,
    IP6_PAYLOAD_LENGTH,
    IP6_NEXT_HEADER,
    IP6_HOP_LIMIT,
    IP6_SRCADDR,
    IP6_DSTADDR,
    IP6_PAYLOAD,
    IP6_PADDING,
    IP6_RESERVED,
    IP6_MALFORMED_EXTENSION,
    IP6_INCOMPLETE_EXTENSION,
    IP6_FRAGMENT,
    IP6_FRAGMENT_OFFSET,
    IP6_FRAGMENT_FLAGS,
    IP6_FRAGMENT_IDENT,
    IP6_HOPOPTS,
    IP6_DSTOPTS,
    IP6_ROUTING,
    IP6_SECURITY,
    IP6_SECURITY_TLR,
    IP6_SECURITY_PADDING,
    IP6_SECURITY_PAD_LEN,
    IP6_AUTH,
    IP6_SPI,
    IP6_SEQ_NUM,
    IP6_EXT_LENGTH,
    IP6_AUTH_DATA,
    IP6_ROUTING_TYPE,
    IP6_SEGMENTS_LEFT,
    IP6_ROUTING_DATA,
    IP6_ROUTE,
    IP6_ROUTE_ADDR,
    IP6_OPT_TYPE,
    IP6_OPT_TYPE_BITS,
    IP6_OPT_LENGTH,
    IP6_OPT_DATA,
    IP6_OPT_MALFORMED,
    IP6_OPT_UNRECOGNIZED,
    IP6_OPT_PADDING,
    IP6_OPT_JUMBO,
    IP6_OPT_JUMBO_VALUE,
    IP6_OPT_HOME_ADDR,
    IP6_OPT_HOME_ADDR_VALUE,
    IP6_OPT_BIND_UPDATE,
    IP6_OPT_BIND_UPDATE_FLAGS,
    IP6_OPT_BIND_UPDATE_FLAGS_BITS,
    IP6_OPT_BIND_UPDATE_PREFIX_LEN,
    IP6_OPT_BIND_SEQ_NO,
    IP6_OPT_BIND_LIFETIME,
    IP6_OPT_BIND_UPDATE_CARE_OF_ADDR,
    IP6_OPT_BIND_ACK,
    IP6_OPT_BIND_ACK_STATUS,
    IP6_OPT_BIND_ACK_REFRESH,
    IP6_OPT_BIND_REQUEST,
    IP6_OPT_ROUTER_ALERT,
    IP6_OPT_ROUTER_ALERT_VALUE,
    IP6_SUB_OPTIONS,
    IP6_SUB_OPT_TYPE,
    IP6_SUB_OPT_LENGTH,
    IP6_SUB_OPT_DATA,
    IP6_SUB_OPT_MALFORMED,
    IP6_SUB_OPT_UNRECOGNIZED,
    IP6_SUB_OPT_UNIQUE_ID,
    IP6_SUB_OPT_UNIQUE_ID_VALUE,
    IP6_SUB_OPT_HOME_AGENTS,
    IP6_SUB_OPT_HOME_AGENT_VALUE,
};

PROPERTYINFO IP6Database[] =
{
    {   //  IP6_SUMMARY
        0,0,
        "Summary",
        "IPv6 packet",
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        BIG_FORMAT_BUFFER_SIZE,
        IP6FormatString},

    {   // IP6_VERSION
        0,0, 
        "Version",     
        "Version Number.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_CLASS
        0,0, 
        "Traffic Class",     
        "Traffic Class.",
        PROP_TYPE_BYTE,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_FLOW_LABEL
        // Formatted as a DWORD, so it can be used for finding frames
        0,0,
        "Flow Label",
        "Flow Label.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_PAYLOAD_LENGTH
        // Formatted as a DWORD, to allow for jumbo-grams.
        0,0, 
        "Payload Length",     
        "Payload Length.", 
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_NEXT_HEADER
        0,0, 
        "Next Header",     
        "Type of protocol header following this one.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_LABELED_SET, 
        &IP6AbbrevProtocolsSet,
        FORMAT_BUFFER_SIZE, 
        IP6FormatNamedByte},

    {   // IP6_HOP_LIMIT
        0,0, 
        "Hop Limit",     
        "Hop Limit.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_SRCADDR
        0,0, 
        "Source Address",     
        "Source Address.", 
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // IP6_DSTADDR
        0,0, 
        "Destination Address",     
        "Destination Address.", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // IP6_PAYLOAD
        0,0, 
        "Payload",     
        "Payload.", 
        PROP_TYPE_RAW_DATA,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_PADDING
        0,0, 
        "Padding",
        "Data in the frame beyond the IP6 packet.", 
        PROP_TYPE_RAW_DATA,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_RESERVED
        0,0, 
        "Reserved",
        "Reserved bits.", 
        PROP_TYPE_COMMENT,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_MALFORMED_EXTENSION
        0,0, 
        "Malformed Extension",
        "Malformed Extension.",
        PROP_TYPE_COMMENT,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatString},

    {   // IP6_INCOMPLETE_EXTENSION
        0,0, 
        "Incomplete Extension Header",
        "Incomplete Extension Header.",
        PROP_TYPE_COMMENT,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatString},

    {   // IP6_FRAGMENT
        0,0, 
        "Fragment Header",
        "Fragment Header.", 
        PROP_TYPE_COMMENT,    
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_FRAGMENT_OFFSET
        0,0, 
        "Offset",
        "Fragment Offset (in bytes).", 
        PROP_TYPE_WORD,
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_FRAGMENT_FLAGS
        0,0, 
        "Flags",
        "Fragment Flags.", 
        PROP_TYPE_WORD,
        PROP_QUAL_FLAGS, 
        &IP6FragmentFlagsSet,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_FRAGMENT_IDENT
        0,0, 
        "Identifier",
        "Fragment Identifier.", 
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   // IP6_HOPOPTS
        0,0,
        "Hop-by-Hop Options Header",
        "Hop-by-Hop Options Header.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_DSTOPTS
        0,0,
        "Destination Options Header",
        "Destination Options Header.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_ROUTING
        0,0,
        "Routing Header",
        "Routing Header.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SECURITY
        0,0,
        "ESP Header",
        "Encapsulating Security Payload Header.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

   {   // IP6_SECURITY_TLR
        0,0,
        "ESP Trailer",
        "Encapsulating Security Payload Trailer.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

        {   // IP6_SECURITY_PADDING
        0,0,
        "ESP Padding",
        "Encapsulating Security Payload Padding.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance}, 

   {   // IP6_SECURITY_PAD_LEN
        0,0,
        "ESP Pad Length",
        "Encapsulating Security Payload Padding Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},        

    {   // IP6_AUTH
        0,0,
        "Authentication Header",
        "Authentication Header.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SPI
        0,0,
        "SPI",
        "Security Parameters Index.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

   {   // IP6_SEQ_NUM
            0,0,
                "Seq Num",
                "Sequence Number.",
                PROP_TYPE_DWORD,
                PROP_QUAL_NONE,
                NULL,
                FORMAT_BUFFER_SIZE,
                FormatPropertyInstance},

    {   // IP6_EXT_LENGTH
        0,0,
        "Length",
        "Extension Header Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_AUTH_DATA
        0,0,
        "Auth Data",
        "Authentication Data.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_ROUTING_TYPE
        0,0,
        "Type",
        "Routing Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SEGMENTS_LEFT
        0,0,
        "Segments Left",
        "Routing Segments Left.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_ROUTING_DATA
        0,0,
        "Data",
        "Routing Data.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_ROUTE
        0,0,
        "Route",
        "List of addresses traversed.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_ROUTE_ADDR
        0,0,
        "Address",
        "Address in route.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE,
        IP6FormatAddress},

    {   // IP6_OPT_TYPE
        0,0,
        "Type",
        "Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_SET,
        &IP6OptionsSet,
        FORMAT_BUFFER_SIZE,
        IP6FormatNamedByte},

    {   // IP6_OPT_TYPE_BITS
        0,0,
        "Option Type Bits",
        "Flags encoded in the Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE * 2,
        IP6FormatString},

    {   // IP6_OPT_LENGTH
        0,0,
        "Length",
        "Option Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_DATA
        0,0,
        "Data",
        "Option Data.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_MALFORMED
        0,0,
        "Malformed Option",
        "Malformed Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_UNRECOGNIZED
        0,0,
        "Unrecognized Option Type",
        "Unrecognized Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_PADDING
        0,0,
        "Padding",
        "Padding.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        IP6FormatString},

    {   // IP6_OPT_JUMBO
        0,0,
        "Jumbo Payload Option",
        "Jumbo Payload Option.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_JUMBO_VALUE
        0,0,
        "Jumbo Payload Length",
        "Jumbo Payload Length.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_HOME_ADDR
        0,0,
        "Home Address Option",
        "Home Address Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_HOME_ADDR_VALUE
        0,0,
        "Home Address",
        "Home Address.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

    {   // IP6_OPT_BIND_UPDATE
        0,0,
        "Binding Update Option",
        "Binding Update Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_BIND_UPDATE_FLAGS
        0,0,
        "Flags",
        "Flags.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_BIND_UPDATE_FLAGS_BITS
        0,0,
        "Flags Bits",
        "Flags Bits.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE * 2,
        IP6FormatString},

    {   // IP6_OPT_BIND_UPDATE_PREFIX_LEN
        0,0,
        "Prefix Length",
        "Prefix Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_BIND_SEQ_NO
        0,0,
        "Sequence Number",
        "Sequence Number.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},
   
    {   // IP6_OPT_BIND_LIFETIME
        0,0,
        "Lifetime",
        "Lifetime.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},
    
    {   // IP6_OPT_BIND_UPDATE_CARE_OF_ADDR
        0,0,
        "Care-of Address",
        "Care-of Address.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},


    {   // IP6_OPT_BIND_ACK
        0,0,
        "Binding Acknowledgement Option",
        "Binding Acknowledgement Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_BIND_ACK_STATUS
        0,0,
        "Status",
        "Status.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE * 2,
        IP6FormatString},

    {   // IP6_OPT_BIND_ACK_REFRESH
        0,0,
        "Refresh",
        "Refresh.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_BIND_REQUEST
        0,0,
        "Binding Request Option",
        "Binding Request Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_ROUTER_ALERT
        0,0,
        "Router Alert Option",
        "Router Alert Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_OPT_ROUTER_ALERT_VALUE
        0,0,
        "Router Alert Value",
        "Router Alert Value.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},
      
    {   // IP6_SUB_OPTIONS
        0,0,
        "Mobile Sub-Options",
        "Mobile Sub-Options.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_TYPE
        0,0,
        "Type",
        "Sub-Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_LENGTH
        0,0,
        "Length",
        "Sub-Option Length.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_DATA
        0,0,
        "Data",
        "Sub-Option Data.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_MALFORMED
        0,0,
        "Malformed Sub-Option",
        "Malformed Sub-Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_UNRECOGNIZED
        0,0,
        "Unrecognized Sub-Option Type",
        "Unrecognized Sub-Option Type.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_UNIQUE_ID
        0,0,
        "Unique Id Sub-Option",
        "Unique Id Sub-Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_UNIQUE_ID_VALUE
        0,0,
        "Unique ID",
        "Unique ID.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_HOME_AGENTS
        0,0,
        "Home Agents List Sub-Option",
        "Home Agents List Sub-Option.",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // IP6_SUB_OPT_HOME_AGENT_VALUE
        0,0,
        "Home Agent Address",
        "Home Agent Address.",
        PROP_TYPE_BYTE,
        PROP_QUAL_ARRAY, 
        NULL,
        FORMAT_BUFFER_SIZE, 
        IP6FormatAddress},

};

DWORD nIP6Properties = ((sizeof IP6Database) / PROPERTYINFO_SIZE);


LPHANDOFFTABLE lpIP6HandoffTable;

//=============================================================================
//  FUNCTION: IP6Register()
//
//=============================================================================

VOID WINAPI IP6Register(HPROTOCOL hIP6Protocol)
{
    DWORD nIP6HandoffSets;
    DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hIP6Protocol, nIP6Properties);

    for(i = 0; i < nIP6Properties; ++i)
    {
        AddProperty(hIP6Protocol, &IP6Database[i]);
    }

    //=========================================================================
    //  Create the protocol hand-off table.
    //=========================================================================

    nIP6HandoffSets = CreateHandoffTable(IP6_SECTION_NAME,
                IniFile, &lpIP6HandoffTable, IP6_TABLE_SIZE, 10);
}


//=============================================================================
//  FUNCTION: Deregister()
//
//=============================================================================

VOID WINAPI IP6Deregister(HPROTOCOL hIP6Protocol)
{
    DestroyPropertyDatabase(hIP6Protocol);
    DestroyHandoffTable(lpIP6HandoffTable);
}


//=============================================================================
//  FUNCTION: IP6RecognizeFrame()
//
//=============================================================================

LPBYTE WINAPI IP6RecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          IP6Frame,                   //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         pInstData)                   //... Next protocol instance data.
{
    LPIP6_HEADER pIP6 = (LPIP6_HEADER) IP6Frame;
    int ESPTlrCount = 0;  // Use to figure out how many nested ESP headers and trailers we have.

    // Check that we have a complete & consistent header;
    // IP6AttachProperties relies on this.

    if ((BytesLeft >= sizeof *pIP6) &&
        (IP6Version(pIP6) == 6)) {
        DWORD PayloadLength = IP6PayloadLength(pIP6);
        BYTE NextHeader = pIP6->NextHeader;
        LPBYTE pNextHeader = (LPBYTE)(pIP6 + 1);
        DWORD MoreFragments = 0;

        BytesLeft -= sizeof *pIP6;

        // Skip any extension headers.

        // We parse extension headers explicitly as opposed
        // to making them separate protocols in Bloodhound.
        // One reason is that we want the hPreviousProtocol
        // in TCP, UDP to be IP or IP6, without intervening
        // extension headers.

        for (;;) {
            DWORD ExtHdrLen;

            switch (NextHeader) {
            case PROTO_NONE:
                goto Claimed;

            case PROTO_HOPOPTS:
            case PROTO_DSTOPTS:
            case PROTO_ROUTING: {
                LPIP6_EXTENSION_HEADER pExt =
                    (LPIP6_EXTENSION_HEADER) pNextHeader;

                // If we don't have a complete extension header,
                // just claim the packet.

                if ((sizeof *pExt > BytesLeft) ||
                    ((ExtHdrLen = IP6ExtensionHeaderLength(pExt)) > BytesLeft) ||
                    (ExtHdrLen > PayloadLength))
                    goto Claimed;

                NextHeader = pExt->NextHeader;
                break;
            }

            case PROTO_FRAGMENT: {
                LPIP6_FRAGMENT_HEADER pFrag =
                    (LPIP6_FRAGMENT_HEADER) pNextHeader;

                // If we don't have a complete fragment header,
                // just claim the packet.

                ExtHdrLen = sizeof *pFrag;

                if ((ExtHdrLen > BytesLeft) ||
                    (ExtHdrLen > PayloadLength))
                    goto Claimed;

                // If this is not the first fragment,
                // we must claim the packet.

                if (IP6FragmentOffset(pFrag) > 0)
                    goto Claimed;

                MoreFragments = IP6MoreFragments(pFrag);
                NextHeader = pFrag->NextHeader;
                break;
            }

            case PROTO_ESP: {           
                LPIP6_SECURITY_HEADER pSec =
                    (LPIP6_SECURITY_HEADER) pNextHeader; 
                LPIP6_SECURITY_TRAILER pSecTrailer;

                ESPTlrCount++;
                
                // If we don't have a complete security header,
                // just claim the packet.
                if ((sizeof *pSec > BytesLeft) ||
                    ((ExtHdrLen = IP6SecurityHeaderLength(pSec)) > BytesLeft) ||
                    (ExtHdrLen > PayloadLength))
                    goto Claimed;              
                
                //
                // Need to go to the very end of the packet
                // and get the next header and pad length.
                // Since we only do ESP Authentication, the 
                // packet is visible (not encrypted).
                //
                pSecTrailer = (LPIP6_SECURITY_TRAILER) 
                    ((unsigned char*)pSec + BytesLeft - ((sizeof *pSecTrailer) * ESPTlrCount));
                NextHeader = pSecTrailer->NextHeader;
                break;
            }

            case PROTO_AUTH: {
                LPIP6_AUTH_HEADER pAuth =
                    (LPIP6_AUTH_HEADER) pNextHeader;

                // If we don't have a complete authentication header,
                // just claim the packet.

                if ((sizeof *pAuth > BytesLeft) ||
                    ((ExtHdrLen = IP6AuthHeaderLength(pAuth)) > BytesLeft) ||
                    (ExtHdrLen > PayloadLength))
                    goto Claimed;

                NextHeader = pAuth->NextHeader;
                break;
            }

            default:
                goto NextProtocol;
            }

            pNextHeader += ExtHdrLen;
            PayloadLength -= ExtHdrLen;
            BytesLeft -= ExtHdrLen;
        }

    NextProtocol:

        // If there is no higher-level data,
        // then claim the entire packet.

        if (PayloadLength == 0) {
    Claimed:
            *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
            return NULL;
        }

        // Check if we recognize the following protocol.

        if ((*hNextProtocol = GetProtocolFromTable(lpIP6HandoffTable,
                                        NextHeader, pInstData)) != NULL) {
            // Bloodhound will continue parsing with the next protocol.

            *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
        }
        else {
            // We don't recognize the following protocol.
            // Bloodhound will consult our follow set to continue parsing.

            *ProtocolStatusCode = PROTOCOL_STATUS_RECOGNIZED;
        }

        // Pass two pieces of information to the next protocol:
        //      - it's packet length
        //      - whether it's the first of several fragments

        *pInstData = MakeInstData(PayloadLength, MoreFragments);
        return pNextHeader;
    }
    else {
        // We do not have a consistent header.

        *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return IP6Frame;
    }
}


//=============================================================================
//  FUNCTION: IP6AttachProperties()
//
//=============================================================================

LPBYTE WINAPI IP6AttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    IP6Frame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{
    LPIP6_HEADER pIP6 = ( LPIP6_HEADER ) IP6Frame;
    BYTE Version;
    BYTE Class;
    DWORD FlowLabel;
    DWORD PayloadLength;
    BYTE NextHeader;
    LPBYTE pNextHeader;
    int ESPTlrCount = 0;

    IP6AttachSummary(hFrame, pIP6, BytesLeft);

    Version = IP6Version(pIP6);

    AttachPropertyInstanceEx(hFrame,
                   IP6Database[IP6_VERSION].hProperty,
                   sizeof pIP6->VersClassFlow,
                   &pIP6->VersClassFlow,
                   sizeof Version,
                   &Version,
                   0, 1, 0);    // 1 is the level of indentation

    Class = IP6Class(pIP6);

    AttachPropertyInstanceEx(hFrame,
                   IP6Database[IP6_CLASS].hProperty,
                   sizeof pIP6->VersClassFlow,
                   &pIP6->VersClassFlow,
                   sizeof Class,
                   &Class,
                   0, 1, 0);    // 1 is the level of indentation

    FlowLabel = IP6FlowLabel(pIP6);

    AttachPropertyInstanceEx(hFrame,
                   IP6Database[IP6_FLOW_LABEL].hProperty,
                   sizeof pIP6->VersClassFlow,
                   &pIP6->VersClassFlow,
                   sizeof FlowLabel,
                   &FlowLabel,
                   0, 1, 0);

    PayloadLength = IP6PayloadLength(pIP6);

    AttachPropertyInstanceEx(hFrame,
                   IP6Database[IP6_PAYLOAD_LENGTH].hProperty,
                   sizeof pIP6->PayloadLength,
                   &pIP6->PayloadLength,
                   sizeof PayloadLength,
                   &PayloadLength,
                   0, 1, 0);

    NextHeader = pIP6->NextHeader;

    AttachPropertyInstance(hFrame,
                   IP6Database[IP6_NEXT_HEADER].hProperty,
                   sizeof pIP6->NextHeader,
                   &pIP6->NextHeader,
                   0, 1, 0);

    AttachPropertyInstance(hFrame,
                   IP6Database[IP6_HOP_LIMIT].hProperty,
                   sizeof pIP6->HopLimit,
                   &pIP6->HopLimit,
                   0, 1, 0);

    AttachPropertyInstance(hFrame,
                   IP6Database[IP6_SRCADDR].hProperty,
                   sizeof pIP6->SrcAddr,
                   &pIP6->SrcAddr,
                   0, 1, 0);

    AttachPropertyInstance(hFrame,
                   IP6Database[IP6_DSTADDR].hProperty,
                   sizeof pIP6->DstAddr,
                   &pIP6->DstAddr,
                   0, 1, 0);

    // Process any extension headers.
    // Note that IP6RecognizeFrame does not guarantee
    // that we have complete or consistent extension headers.
    // We distinguish between incomplete extension headers
    // (when BytesLeft cuts off an extension header)
    // and malformed extension headers.

    BytesLeft -= sizeof *pIP6;
    pNextHeader = (LPBYTE)(pIP6 + 1);

    while (NextHeader != PROTO_NONE) {
        DWORD ExtHdrLen;

        switch (NextHeader) {
        case PROTO_HOPOPTS:
        case PROTO_DSTOPTS:
        case PROTO_ROUTING: {
            LPIP6_EXTENSION_HEADER pExt =
                (LPIP6_EXTENSION_HEADER) pNextHeader;

            ExtHdrLen = sizeof *pExt;
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            ExtHdrLen = IP6ExtensionHeaderLength(pExt);
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            switch (NextHeader) {
            case PROTO_HOPOPTS:
                IP6AttachOptions(hFrame, pExt, IP6_HOPOPTS);
                break;

            case PROTO_DSTOPTS:
                IP6AttachOptions(hFrame, pExt, IP6_DSTOPTS);
                break;

            case PROTO_ROUTING:
                IP6AttachRouting(hFrame, (LPIP6_ROUTING_HEADER)pExt);
                break;
            }

            NextHeader = pExt->NextHeader;
            break;
        }

        case PROTO_FRAGMENT: {
            LPIP6_FRAGMENT_HEADER pFrag =
                (LPIP6_FRAGMENT_HEADER) pNextHeader;

            ExtHdrLen = sizeof *pFrag;
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            IP6AttachFragment(hFrame, pFrag);

            // If this is not the first fragment,
            // show remaining bytes as payload.

            if (IP6FragmentOffset(pFrag) > 0)
                NextHeader = PROTO_NONE; // leave loop
            else
                NextHeader = pFrag->NextHeader;
            break;
        }

        case PROTO_ESP: {
            LPIP6_SECURITY_HEADER pSec =
                (LPIP6_SECURITY_HEADER) pNextHeader; 
            LPIP6_SECURITY_TRAILER pSecTrailer;

            ESPTlrCount++;

            ExtHdrLen = sizeof *pSec;
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            IP6AttachSecurity(hFrame, pSec);

            //
            // Need to go to the very end of the packet
            // and get the next header and pad length.
            // Since we only do ESP Authentication, the 
            // packet is visible (not encrypted).
            //
            pSecTrailer = (LPIP6_SECURITY_TRAILER) 
                ((unsigned char*)pSec + BytesLeft - ((sizeof *pSecTrailer) * ESPTlrCount));            

            IP6AttachSecurityTrailer(hFrame, pSecTrailer);

            NextHeader = pSecTrailer->NextHeader;

            break;
        }

        case PROTO_AUTH: {
            LPIP6_AUTH_HEADER pAuth =
                (LPIP6_AUTH_HEADER) pNextHeader;

            ExtHdrLen = sizeof *pAuth;
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            ExtHdrLen = IP6AuthHeaderLength(pAuth);
            if (ExtHdrLen > PayloadLength)
                goto MalformedExtension;
            if (ExtHdrLen > BytesLeft)
                goto IncompleteExtension;

            IP6AttachAuth(hFrame, pAuth);

            NextHeader = pAuth->NextHeader;
            break;
        }

        default:
            ExtHdrLen = 0;
            NextHeader = PROTO_NONE; // leave loop
            break;

        MalformedExtension:
            ExtHdrLen = PayloadLength;
            if (ExtHdrLen > BytesLeft)
                ExtHdrLen = BytesLeft;
            IP6AttachMalformedExtension(hFrame, NextHeader, pNextHeader, ExtHdrLen);
            NextHeader = PROTO_NONE; // leave loop
            break;

        IncompleteExtension:
            ExtHdrLen = BytesLeft;
            IP6AttachIncompleteExtension(hFrame, NextHeader, pNextHeader, ExtHdrLen);
            NextHeader = PROTO_NONE; // leave loop
            break;
        }

        pNextHeader += ExtHdrLen;
        PayloadLength -= ExtHdrLen;
        BytesLeft -= ExtHdrLen;
    }

    // Create payload & padding properties, if appropriate.
    // Note that the summary property only highlights
    // the header, and these properties highlight
    // data following the header.

    if (PayloadLength > BytesLeft)
        PayloadLength = BytesLeft;

    if (PayloadLength > 0) {

        AttachPropertyInstance(hFrame,
                       IP6Database[IP6_PAYLOAD].hProperty,
                       PayloadLength,
                       pNextHeader,
                       0, 1, 0);

        BytesLeft -= PayloadLength;
    }

    if (BytesLeft > 0) {

        AttachPropertyInstance(hFrame,
                       IP6Database[IP6_PADDING].hProperty,
                       BytesLeft,
                       pNextHeader + PayloadLength,
                       0, 1, 0);
    }

    return NULL;
}

VOID WINAPIV IP6AttachSummary(HFRAME hFrame, LPIP6_HEADER pIP6, DWORD BytesLeft)
{
    DWORD PayloadLength = IP6PayloadLength(pIP6);
    BYTE NextHeader = pIP6->NextHeader;
    LPBYTE pNextHeader = (LPBYTE)(pIP6 + 1);
    BOOL Stop = FALSE;
    int NumExts = 5;

    char Text[BIG_FORMAT_BUFFER_SIZE];
    LPSTR p = Text;
    LPSTR s;

    // Simplify checks for incomplete extension headers. (Here we don't
    // distinguish between incomplete headers that extend beyond BytesLeft
    // and malformed headers that extend beyond PayloadLength.)

    BytesLeft -= sizeof *pIP6;
    if (BytesLeft > PayloadLength)
        BytesLeft = PayloadLength;

    // Parse extension headers. We want the "true" payload size
    // and upper-layer protocol value. We let extension headers
    // add information to the summary. However we stop adding
    // to the summary after 5 extension headers.

    while (!Stop) {
        DWORD ExtHdrLen;

        switch (NextHeader) {
        case PROTO_NONE:
            goto Summarize;

        case PROTO_HOPOPTS:
        case PROTO_DSTOPTS:
        case PROTO_ROUTING: {
            LPIP6_EXTENSION_HEADER pExt =
                (LPIP6_EXTENSION_HEADER) pNextHeader;

            // If we don't have a complete extension header,
            // stop here.

            if ((sizeof *pExt > BytesLeft) ||
                ((ExtHdrLen = IP6ExtensionHeaderLength(pExt)) > BytesLeft))
                goto Summarize;

            if (NumExts-- > 0) {
                switch (NextHeader) {
                case PROTO_HOPOPTS:
                    p += sprintf(p, "Hop Opts; ");
                    break;

                case PROTO_DSTOPTS:
                    p += sprintf(p, "Dest Opts; ");
                    break;

                case PROTO_ROUTING: {
                    LPIP6_ROUTING_HEADER pRoute =
                        (LPIP6_ROUTING_HEADER) pExt;

                    if ((pRoute->Type == 0) &&
                        (pRoute->Ext.Length <= 46) &&
                        ((pRoute->Ext.Length & 1) == 0) &&
                        (pRoute->SegmentsLeft <= pRoute->Ext.Length/2))
                        p += sprintf(p, "Routing (%u left of %u); ",
                                     pRoute->SegmentsLeft,
                                     pRoute->Ext.Length/2);
                    else
                        p += sprintf(p, "Routing (type %u); ",
                                     pRoute->Type);
                    break;
                }
                }
            }

            NextHeader = pExt->NextHeader;
            break;
        }

        case PROTO_FRAGMENT: {
            LPIP6_FRAGMENT_HEADER pFrag =
                (LPIP6_FRAGMENT_HEADER) pNextHeader;

            // If we don't have a complete fragment header,
            // stop here.

            ExtHdrLen = sizeof *pFrag;

            if (ExtHdrLen > BytesLeft)
                goto Summarize;

            if (NumExts-- > 0) {
                p += sprintf(p, "Fragment (id 0x%X at 0x%x%s); ",
                             DXCHG(pFrag->Identifier),
                             IP6FragmentOffset(pFrag),
                             IP6MoreFragments(pFrag) ? "+" : "");
            }

            // If this is not the first fragment,
            // we must stop *after* this header.

            NextHeader = pFrag->NextHeader;
            if (IP6FragmentOffset(pFrag) > 0)
                Stop = TRUE;
            break;
        }

        case PROTO_ESP: {
            LPIP6_SECURITY_HEADER pSec =
                (LPIP6_SECURITY_HEADER) pNextHeader;

            // If we don't have a complete security header,
            // stop here.

            ExtHdrLen = sizeof *pSec;

            if (ExtHdrLen > BytesLeft)
                goto Summarize;

            if (NumExts-- > 0) {
                p += sprintf(p, "ESP SPI = 0x%x; ",
                             IP6SecuritySPI(pSec));
            }

            // The remainder of the packet is encrypted
            // and can not be parsed.

            Stop = TRUE;
            NextHeader = PROTO_NONE;
            break;
        }

        case PROTO_AUTH: {
            LPIP6_AUTH_HEADER pAuth =
                (LPIP6_AUTH_HEADER) pNextHeader;

            // If we don't have a complete authentication header,
            // stop here.

            if ((sizeof *pAuth > BytesLeft) ||
                ((ExtHdrLen = IP6AuthHeaderLength(pAuth)) > BytesLeft))
                goto Summarize;

            if (NumExts-- > 0) {
                p += sprintf(p, "Auth SPI = 0x%x; ",
                             IP6AuthSPI(pAuth));
            }

            NextHeader = pAuth->NextHeader;
            break;
        }

        default:
            goto Summarize;
        }

        pNextHeader += ExtHdrLen;
        PayloadLength -= ExtHdrLen;
        BytesLeft -= ExtHdrLen;
    }

Summarize:

    if (NumExts < 0)
        p += sprintf(p, "... ");

    if (NextHeader != PROTO_NONE) {
        p += sprintf(p, "Proto = ");

        s = LookupByteSetString(&IP6AbbrevProtocolsSet, NextHeader);

        if (s != NULL)
            p += sprintf(p, "%s; ", s);
        else
            p += sprintf(p, "0x%02X; ", NextHeader);
    }

    p += sprintf(p, "Len = %u", PayloadLength);

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_SUMMARY].hProperty,
                pNextHeader - (LPBYTE)pIP6,
                pIP6,
                strlen(Text),
                Text,
                0, 0, 0);
}

VOID WINAPIV IP6AttachMalformedExtension(HFRAME hFrame, BYTE NextHeader, LPBYTE pNextHeader, DWORD ExtHdrLen)
{
    char Text[FORMAT_BUFFER_SIZE];

    sprintf(Text, "Malformed %s",
            LookupByteSetString(&IP6AbbrevProtocolsSet, NextHeader));

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_MALFORMED_EXTENSION].hProperty,
                ExtHdrLen,
                pNextHeader,
                strlen(Text),
                Text,
                0, 1, 1);
}

VOID WINAPIV IP6AttachIncompleteExtension(HFRAME hFrame, BYTE NextHeader, LPBYTE pNextHeader, DWORD ExtHdrLen)
{
    char Text[FORMAT_BUFFER_SIZE];

    sprintf(Text, "Incomplete %s",
            LookupByteSetString(&IP6AbbrevProtocolsSet, NextHeader));

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_INCOMPLETE_EXTENSION].hProperty,
                ExtHdrLen,
                pNextHeader,
                strlen(Text),
                Text,
                0, 1, 1);
}

VOID WINAPIV IP6AttachFragment(HFRAME hFrame, LPIP6_FRAGMENT_HEADER pFrag)
{
    WORD FragmentOffset;
    DWORD Identifier;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_FRAGMENT].hProperty,
                sizeof *pFrag,
                pFrag,
                0, 1, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_NEXT_HEADER].hProperty,
                sizeof pFrag->NextHeader,
                &pFrag->NextHeader,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_RESERVED].hProperty,
                sizeof pFrag->Reserved,
                &pFrag->Reserved,
                0, 2, 0);

    FragmentOffset = IP6FragmentOffset(pFrag);

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_FRAGMENT_OFFSET].hProperty,
                sizeof pFrag->FragmentOffset,
                &pFrag->FragmentOffset,
                sizeof FragmentOffset,
                &FragmentOffset,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_FRAGMENT_FLAGS].hProperty,
                sizeof pFrag->FragmentOffset,
                &pFrag->FragmentOffset,
                0, 2, IFLAG_SWAPPED);

    // This is "opaque" data.
    // The NRL/BSD implementation puts a counter in this field
    // that is little-endian (not byte-swapped).
    Identifier = DXCHG(pFrag->Identifier);
    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_FRAGMENT_IDENT].hProperty,
                sizeof pFrag->Identifier,
                &pFrag->Identifier,
                sizeof Identifier,
                &Identifier,
                0, 2, 0);
}

VOID WINAPIV IP6AttachOptions(HFRAME hFrame, LPIP6_EXTENSION_HEADER pExt, DWORD Prop)
{
    DWORD Length = IP6ExtensionHeaderLength(pExt);
    LPBYTE pOptions;
    LPBYTE pOptionsEnd;

    AttachPropertyInstance(hFrame,
                IP6Database[Prop].hProperty,
                Length,
                pExt,
                0, 1, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_NEXT_HEADER].hProperty,
                sizeof pExt->NextHeader,
                &pExt->NextHeader,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_EXT_LENGTH].hProperty,
                sizeof pExt->Length,
                &pExt->Length,
                0, 2, 0);

    pOptions = (LPBYTE)(pExt + 1);
    pOptionsEnd = (LPBYTE)pExt + Length;

    while (pOptions < pOptionsEnd) {
        BYTE OptionType = pOptions[0];
        DWORD OptionLength;
        LPBYTE pOptionsNext;

        if (OptionType == IP6_OPTION_PAD1) {
            // Single-octet option
            OptionLength = 1;
        }
        else if ((pOptions + 2 <= pOptionsEnd) &&
                 (pOptions + (OptionLength = pOptions[1]+2) <= pOptionsEnd)) {
            // Multi-octet option
        }
        else {
            // Malformed option
            IP6AttachOptionMalformed(hFrame, pOptions, pOptionsEnd);
            break;
        }

        pOptionsNext = pOptions + OptionLength;

        switch (OptionType) {
        case IP6_OPTION_PAD1:
            IP6AttachOptionPad1(hFrame, pOptions);
            break;

        case IP6_OPTION_PADN:
            IP6AttachOptionPadN(hFrame, pOptions);
            break;

        case IP6_OPTION_JUMBO:
            IP6AttachOptionJumbo(hFrame, pOptions);
            break;

        case IP6_OPTION_BIND_UPDATE:
            IP6AttachOptionBinding(hFrame, pOptions);
            break;
    
        case IP6_OPTION_HOME_ADDR:
            IP6AttachOptionHomeAddr(hFrame, pOptions);
            break;
        
        case IP6_OPTION_BIND_ACK:
            IP6AttachOptionBindAck(hFrame, pOptions);
            break;

        case IP6_OPTION_BIND_REQUEST:
            IP6AttachOptionBindRequest(hFrame, pOptions);
            break;

        case IP6_OPTION_ROUTER_ALERT:
            IP6AttachOptionRouterAlter(hFrame, pOptions);
            break;

        default:
            IP6AttachOptionUnrecognized(hFrame, pOptions);
            break;
        }

        pOptions = pOptionsNext;
    }
}

VOID WINAPIV IP6AttachOptionType(HFRAME hFrame, LPBYTE pOption)
{
    char Text[FORMAT_BUFFER_SIZE * 2];

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_TYPE].hProperty,
                sizeof pOption[0],
                &pOption[0],
                0,3,0);

    switch (pOption[0] & 0xC0) {
    case 0x00:
        sprintf(Text, "00...... = Skip option if not recognized");
        break;
    case 0x40:
        sprintf(Text, "01...... = Discard packet if not recognized");
        break;
    case 0x80:
        sprintf(Text, "10...... = Discard packet if not recognized, and send ICMP");
        break;
    case 0xC0:
        sprintf(Text, "11...... = Discard packet if not recognized, and send ICMP if not multicast");
        break;
    }

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_TYPE_BITS].hProperty,
                sizeof pOption[0],
                &pOption[0],
                strlen(Text),
                Text,
                0,4,0);

    if (pOption[0] & 0x20)
        sprintf(Text, "..1..... = Option data may change enroute");
    else
        sprintf(Text, "..0..... = Option data does not change enroute");

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_TYPE_BITS].hProperty,
                sizeof pOption[0],
                &pOption[0],
                strlen(Text),
                Text,
                0,4,0);
}

VOID WINAPIV IP6AttachOptionLength(HFRAME hFrame, LPBYTE pOption)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_LENGTH].hProperty,
                sizeof pOption[1],
                &pOption[1],
                0,3,0);
}

VOID WINAPIV IP6AttachOptionMalformed(HFRAME hFrame, LPBYTE pOption, LPBYTE pOptionEnd)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_MALFORMED].hProperty,
                pOptionEnd - pOption,
                pOption,
                0,2,0);

    IP6AttachOptionType(hFrame, pOption);

    if (pOption + 2 <= pOptionEnd) {

        IP6AttachOptionLength(hFrame, pOption);

        if (pOption + 2 < pOptionEnd) {

            AttachPropertyInstance(hFrame,
                        IP6Database[IP6_OPT_DATA].hProperty,
                        pOptionEnd - &pOption[2],
                        &pOption[2],
                        0,3,0);
        }
    }
}

VOID WINAPIV IP6AttachOptionUnrecognized(HFRAME hFrame, LPBYTE pOption)
{
    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_UNRECOGNIZED].hProperty,
                pOption[1] + 2,
                pOption,
                sizeof pOption[0],
                &pOption[0],
                0,2,0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    if (pOption[1] != 0) {

        AttachPropertyInstance(hFrame,
                    IP6Database[IP6_OPT_DATA].hProperty,
                    pOption[1],
                    &pOption[2],
                    0,3,0);
    }
}

VOID WINAPIV IP6AttachOptionPadding(HFRAME hFrame, LPBYTE pOption, DWORD Bytes)
{
    char Text[FORMAT_BUFFER_SIZE];

    sprintf(Text, "Padding (%u bytes)", Bytes);

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_PADDING].hProperty,
                Bytes,
                pOption,
                strlen(Text),
                Text,
                0,2,0);

}

VOID WINAPIV IP6AttachOptionPad1(HFRAME hFrame, LPBYTE pOption)
{
    IP6AttachOptionPadding(hFrame, pOption, 1);
    IP6AttachOptionType(hFrame, pOption);
}

VOID WINAPIV IP6AttachOptionPadN(HFRAME hFrame, LPBYTE pOption)
{
    IP6AttachOptionPadding(hFrame, pOption, pOption[1] + 2);
    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    if (pOption[1] != 0) {

        AttachPropertyInstance(hFrame,
                    IP6Database[IP6_OPT_DATA].hProperty,
                    pOption[1],
                    &pOption[2],
                    0,3,0);
    }
}

VOID WINAPIV IP6AttachOptionJumbo(HFRAME hFrame, LPBYTE pOption)
{
#pragma pack(1)
    struct _JUMBO {
        BYTE Type;
        BYTE Length;
        DWORD Jumbo;
    } UNALIGNED *pOpt = (struct _JUMBO UNALIGNED *) pOption;
#pragma pack()

    if (pOpt->Length + 2 != sizeof *pOpt) {
        IP6AttachOptionMalformed(hFrame, pOption, pOption + pOpt->Length + 2);
        return;
    }

    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_JUMBO].hProperty,
                pOption[1] + 2,
                pOption,
                sizeof pOpt->Jumbo,
                &pOpt->Jumbo,
                0,2,IFLAG_SWAPPED);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_JUMBO_VALUE].hProperty,
                sizeof pOpt->Jumbo,
                &pOpt->Jumbo,
                0,3,IFLAG_SWAPPED);
}


VOID WINAPIV IP6AttachSubOptionType(HFRAME hFrame, LPBYTE Type)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_TYPE].hProperty,
                sizeof Type[0],
                &Type[0],
                0,5,0);

}

VOID WINAPIV IP6AttachSubOptionLength(HFRAME hFrame, LPBYTE Length)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_LENGTH].hProperty,
                sizeof Length[0],
                &Length[0],
                0,5,0);
}

VOID WINAPIV IP6AttachSubOptUnrecognized(HFRAME hFrame, LPIP6_SUB_OPT_HDR pHdr)
{
    LPBYTE Data;
    
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_UNRECOGNIZED].hProperty,
                sizeof *pHdr + pHdr->Length,
                pHdr,
                0,4,0);

    IP6AttachSubOptionType(hFrame, &pHdr->Type);
    IP6AttachSubOptionLength(hFrame, &pHdr->Length);

    if (pHdr->Length > 0) {
        Data = ((LPBYTE) pHdr) + 2; 
        AttachPropertyInstance(hFrame,
                    IP6Database[IP6_SUB_OPT_DATA].hProperty,
                    pHdr->Length,
                    Data,
                    0,5,0);
    }
}

VOID WINAPIV IP6AttachSubOptionMalformed(HFRAME hFrame, LPIP6_SUB_OPT_HDR pHdr, 
            WORD BytesLeft)
{
    LPBYTE Data;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_MALFORMED].hProperty,
                BytesLeft,
                pHdr,
                0,4,0);

    IP6AttachSubOptionType(hFrame, &pHdr->Type);

    if (BytesLeft > 1)
        IP6AttachSubOptionLength(hFrame, &pHdr->Length);

    if (BytesLeft > 2) {
        Data = ((LPBYTE) pHdr) + 2;
        AttachPropertyInstance(hFrame,
            IP6Database[IP6_SUB_OPT_DATA].hProperty,
            BytesLeft-2,
            Data,
            0,5,0);
    }
}

VOID WINAPIV IP6AttachUniqueId(HFRAME hFrame, LPIP6_UNIQUE_ID_SUB_OPT pHdr)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_UNIQUE_ID].hProperty,
                sizeof *pHdr,
                pHdr,
                0,4,0);

    IP6AttachSubOptionType(hFrame, &pHdr->Type);
    IP6AttachSubOptionLength(hFrame, &pHdr->Length);

    AttachPropertyInstance(hFrame,
            IP6Database[IP6_SUB_OPT_UNIQUE_ID_VALUE].hProperty,
            sizeof pHdr->UniqueId,
            &pHdr->UniqueId,
            0,5,IFLAG_SWAPPED);
}

VOID WINAPIV IP6AttachHomeAgents(HFRAME hFrame, LPIP6_HOME_AGENTS_SUB_OPT pHdr)
{
    BYTE Length, i;

    Length = pHdr->Length;
    
    if (Length & sizeof (IP6_ADDRESS) != 0) {
        //
        // Length must be a multiple of the size of a V6 address.
        //
        IP6AttachSubOptionMalformed(hFrame, (LPIP6_SUB_OPT_HDR) pHdr, Length);
        return;
    }

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPT_HOME_AGENTS].hProperty,
                sizeof *pHdr + Length,
                pHdr,
                0,4,0);

    IP6AttachSubOptionType(hFrame, &pHdr->Type);
    IP6AttachSubOptionLength(hFrame, &pHdr->Length);

    i=0;
    while (Length > 0) {
        AttachPropertyInstance(hFrame,
                        IP6Database[IP6_SUB_OPT_HOME_AGENT_VALUE].hProperty,
                        sizeof (IP6_ADDRESS),
                        &pHdr->Address[i++],
                        0, 5, IFLAG_ERROR);
        Length -= sizeof (IP6_ADDRESS);
    }
}

VOID WINAPIV IP6AttachMobileSubOpts(HFRAME hFrame, LPIP6_SUB_OPT_HDR pHdr,
         WORD BytesLeft)
{
    BYTE OptionType, OptionLength;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SUB_OPTIONS].hProperty,
                BytesLeft,
                pHdr,
                0, 3, 0);

    while (BytesLeft > 0) {
        OptionType = pHdr->Type;
        if (BytesLeft > 1)
            OptionLength = pHdr->Length;
        else {
            // Malformed sub-option
            IP6AttachSubOptionMalformed(hFrame, pHdr, BytesLeft);
            break;
        }

        if (OptionLength > BytesLeft) {
            // Malformed sub-option
            IP6AttachSubOptionMalformed(hFrame, pHdr, BytesLeft);
            break;
        }

        switch (OptionType) {
        case IP6_SUB_OPTION_UNIQUE_ID:
            IP6AttachUniqueId(hFrame, (LPIP6_UNIQUE_ID_SUB_OPT) pHdr);
            break;

        case IP6_SUB_OPTION_HOME_AGENTS:
            IP6AttachHomeAgents(hFrame, (LPIP6_HOME_AGENTS_SUB_OPT) pHdr);
            break;

        default:
            IP6AttachSubOptUnrecognized(hFrame, pHdr);
            break;
        }

        BytesLeft -= (OptionLength + 2) ;
        pHdr = (LPIP6_SUB_OPT_HDR) ((LPBYTE) pHdr + OptionLength + 2);
    }
}

VOID WINAPIV IP6AttachOptionHomeAddr(HFRAME hFrame, LPBYTE pOption)
{
    LPIP6_HOME_ADDR_OPT pOpt = (LPIP6_HOME_ADDR_OPT) pOption;
    BYTE BytesLeft;
    LPIP6_SUB_OPT_HDR pHdr;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_HOME_ADDR].hProperty,
                pOpt->Length + 2,
                pOpt,
                0, 2, 0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_HOME_ADDR_VALUE].hProperty,
                sizeof pOpt->HomeAddress,
                &pOpt->HomeAddress,
                0, 3, 0);

    BytesLeft = pOpt->Length - sizeof *pOpt + 2;
    if (BytesLeft > 0) {
        pHdr = (LPIP6_SUB_OPT_HDR) (pOption + sizeof *pOpt);
        IP6AttachMobileSubOpts(hFrame, pHdr, BytesLeft);
    }

}

VOID WINAPIV IP6AttachOptionBinding(HFRAME hFrame, LPBYTE pOption)
{
    LPIP6_BINDING_UPDATE_OPT pOpt = (LPIP6_BINDING_UPDATE_OPT) pOption;
    char Text[FORMAT_BUFFER_SIZE * 2];
    BYTE BytesLeft;
    LPIP6_SUB_OPT_HDR pHdr;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE].hProperty,
                pOpt->Length + 2,
                pOpt,
                0, 2, 0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_FLAGS].hProperty,
                sizeof pOpt->Flags,
                &pOpt->Flags,
                0, 3, 0);

    if (pOpt->Flags & 128) {
        sprintf(Text,"1....... = Request a binding acknowledgement response.");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_FLAGS_BITS].hProperty,
                sizeof pOpt->Flags,
                &pOpt->Flags,
                strlen(Text),
                Text,
                0,4,0);
    }

    if (pOpt->Flags & 64) {
        sprintf(Text,".1...... = Request host to act as home agent.");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_FLAGS_BITS].hProperty,
                sizeof pOpt->Flags,
                &pOpt->Flags,
                strlen(Text),
                Text,
                0,4,0);
    }

    if (pOpt->Flags & 32) {
        sprintf(Text,"..1..... = Binding update includes a care-of address.");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_FLAGS_BITS].hProperty,
                sizeof pOpt->Flags,
                &pOpt->Flags,
                strlen(Text),
                Text,
                0,4,0);
    }

    if (pOpt->Flags == 0) {
        sprintf(Text,"........ = No binding update flags enabled.");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_FLAGS_BITS].hProperty,
                sizeof pOpt->Flags,
                &pOpt->Flags,
                strlen(Text),
                Text,
                0,4,0);
    }

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_PREFIX_LEN].hProperty,
                sizeof pOpt->PrefixLength,
                &pOpt->PrefixLength,
                0, 3, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_SEQ_NO].hProperty,
                sizeof pOpt->SeqNumber,
                &pOpt->SeqNumber,
                0, 3, IFLAG_SWAPPED);
    
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_LIFETIME].hProperty,
                sizeof pOpt->Lifetime,
                &pOpt->Lifetime,
                0, 3, IFLAG_SWAPPED);
    
    if (pOpt->Flags & 32) {
        AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_UPDATE_CARE_OF_ADDR].hProperty,
                sizeof pOpt->CareOfAddr,
                &pOpt->CareOfAddr,
                0, 3, 0);
    }


    BytesLeft = pOpt->Length - sizeof *pOpt + 2;
    if (!pOpt->Flags & 32) 
        BytesLeft += sizeof (IP6_ADDRESS);
    if (BytesLeft > 0) {
        pHdr = (LPIP6_SUB_OPT_HDR) (pOption + pOpt->Length + 2 - BytesLeft);
        IP6AttachMobileSubOpts(hFrame, pHdr, BytesLeft);
    }

}

VOID WINAPIV IP6AttachOptionBindAckStatus(HFRAME hFrame, BYTE *Status)
{
    char Text[FORMAT_BUFFER_SIZE * 2];

    if (*Status == 0) {
        sprintf(Text, "Status: 0 = Binding accepted.");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 128) {
        sprintf(Text, "Status: 128 = Binding rejected for an unspecified reason..");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 129) {
        sprintf(Text, "Status: 129 = Binding rejected (poorly formed binding update).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 130) {
        sprintf(Text, "Status: 130 = Binding rejected (administratively prohibited).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 131) {
        sprintf(Text, "Status: 131 = Binding rejected (no resources).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 132) {
        sprintf(Text, "Status: 132 = Binding rejected (home registration not supported).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 133) {
        sprintf(Text, "Status: 133 = Binding rejected (not home subnet).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 134) {
        sprintf(Text, "Status: 134 = Binding rejected (sequence number too small).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 135) {
        sprintf(Text, "Status: 135 = Binding rejected (dynamic home agent discovery response).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 136) {
        sprintf(Text, "Status: 136 = Binding rejected (incorrect interface identifier length).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    if (*Status == 137) {
        sprintf(Text, "Status: 137 = Binding rejected (not the home agent for this mobile node).");
        AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);
        return;
    }

    sprintf(Text, "Status: %d = Unrecognized status.",*Status);
    AttachPropertyInstanceEx(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_STATUS].hProperty,
                sizeof *Status,
                Status,
                strlen(Text),
                Text,
                0,3,0);

}

VOID WINAPIV IP6AttachOptionBindAck(HFRAME hFrame, LPBYTE pOption)
{
    LPIP6_BINDING_ACK_OPT pOpt = (LPIP6_BINDING_ACK_OPT) pOption;
    BYTE BytesLeft;
    LPIP6_SUB_OPT_HDR pHdr;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_ACK].hProperty,
                pOpt->Length + 2,
                pOpt,
                0, 2, 0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    IP6AttachOptionBindAckStatus(hFrame, &pOpt->Status);
       
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_SEQ_NO].hProperty,
                sizeof pOpt->SeqNumber,
                &pOpt->SeqNumber,
                0, 3, IFLAG_SWAPPED);
    
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_LIFETIME].hProperty,
                sizeof pOpt->Lifetime,
                &pOpt->Lifetime,
                0, 3, IFLAG_SWAPPED);
    
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_ACK_REFRESH].hProperty,
                sizeof pOpt->Refresh,
                &pOpt->Refresh,
                0, 3, IFLAG_SWAPPED);

    BytesLeft = pOpt->Length - sizeof *pOpt + 2;
    if (BytesLeft > 0) {
        pHdr = (LPIP6_SUB_OPT_HDR) (pOption + sizeof *pOpt);
        IP6AttachMobileSubOpts(hFrame, pHdr, BytesLeft);
    }
}

VOID WINAPIV IP6AttachOptionBindRequest(HFRAME hFrame, LPBYTE pOption)
{
    LPIP6_BINDING_REQ_OPT pOpt = (LPIP6_BINDING_REQ_OPT) pOption;
    BYTE BytesLeft;
    LPIP6_SUB_OPT_HDR pHdr;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_BIND_REQUEST].hProperty,
                0,
                pOption,
                0, 2, 0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    BytesLeft = pOpt->Length - sizeof *pOpt + 2;
    if (BytesLeft > 0) {
        pHdr = (LPIP6_SUB_OPT_HDR) (pOption + sizeof *pOpt);
        IP6AttachMobileSubOpts(hFrame, pHdr, BytesLeft);
    }
}


VOID WINAPIV IP6AttachOptionRouterAlter(HFRAME hFrame, LPBYTE pOption)
{
    LPIP6_ROUTER_ALERT_OPT pOpt = (LPIP6_ROUTER_ALERT_OPT) pOption;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_ROUTER_ALERT].hProperty,
                pOpt->Length + 2,
                pOpt,
                0, 2, 0);

    IP6AttachOptionType(hFrame, pOption);
    IP6AttachOptionLength(hFrame, pOption);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_OPT_ROUTER_ALERT_VALUE].hProperty,
                sizeof pOpt->Value,
                &pOpt->Value,
                0, 3, IFLAG_SWAPPED);
}


VOID WINAPIV IP6AttachRouting(HFRAME hFrame, LPIP6_ROUTING_HEADER pRoute)
{
    DWORD Length = IP6ExtensionHeaderLength(&pRoute->Ext);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_ROUTING].hProperty,
                Length,
                pRoute,
                0, 1, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_NEXT_HEADER].hProperty,
                sizeof pRoute->Ext.NextHeader,
                &pRoute->Ext.NextHeader,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_EXT_LENGTH].hProperty,
                sizeof pRoute->Ext.Length,
                &pRoute->Ext.Length,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_ROUTING_TYPE].hProperty,
                sizeof pRoute->Type,
                &pRoute->Type,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SEGMENTS_LEFT].hProperty,
                sizeof pRoute->SegmentsLeft,
                &pRoute->SegmentsLeft,
                0, 2, 0);

    if (pRoute->Type == 0) {
        LPIP6_ROUTING0_HEADER pRoute0 =
            (LPIP6_ROUTING0_HEADER) pRoute;

        // Type 0 routing header.

        AttachPropertyInstance(hFrame,
                IP6Database[IP6_RESERVED].hProperty,
                sizeof pRoute0->Reserved,
                &pRoute0->Reserved,
                0, 2, 0);

        if (Length > sizeof *pRoute0) {

            if ((pRoute->Ext.Length & 1) != 0) {
                // Malformed header

                AttachPropertyInstance(hFrame,
                        IP6Database[IP6_ROUTING_DATA].hProperty,
                        Length - sizeof *pRoute0,
                        (LPBYTE)(pRoute0 + 1),
                        0, 2, IFLAG_ERROR);
            }
            else {
                int i;

                AttachPropertyInstance(hFrame,
                        IP6Database[IP6_ROUTE].hProperty,
                        Length - sizeof *pRoute0,
                        pRoute0->Address,
                        0, 2, 0);

                for (i = 0; i < pRoute->Ext.Length/2; i++) {

                    AttachPropertyInstance(hFrame,
                            IP6Database[IP6_ROUTE_ADDR].hProperty,
                            sizeof pRoute0->Address[i],
                            &pRoute0->Address[i],
                            0, 3, 0);
                }
            }
        }
    }
    else {
        // Unrecognized/malformed routing header.

        AttachPropertyInstance(hFrame,
                IP6Database[IP6_ROUTING_DATA].hProperty,
                Length - sizeof *pRoute,
                (LPBYTE)(pRoute + 1),
                0, 2, 0);
    }
}

VOID WINAPIV IP6AttachSecurity(HFRAME hFrame, LPIP6_SECURITY_HEADER pSec)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SECURITY].hProperty,
                sizeof *pSec,
                pSec,
                0, 1, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SPI].hProperty,
                sizeof pSec->SPI,
                &pSec->SPI,
                0, 2, IFLAG_SWAPPED);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SEQ_NUM].hProperty,
                sizeof pSec->SeqNum,
                &pSec->SeqNum,
                0, 2, IFLAG_SWAPPED);
}


VOID WINAPIV IP6AttachSecurityTrailer(HFRAME hFrame, LPIP6_SECURITY_TRAILER pSecTrailer)
{
    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SECURITY_TLR].hProperty,
                sizeof *pSecTrailer,
                pSecTrailer,
                0, 1, 0);  
    
    if(pSecTrailer->PadLen > 0)
    {
        AttachPropertyInstance(hFrame,
        IP6Database[IP6_SECURITY_PADDING].hProperty,
        pSecTrailer->PadLen,
        (unsigned char*)pSecTrailer - pSecTrailer->PadLen,
        0, 2, 0); 
    }    
    
    AttachPropertyInstance(hFrame,
        IP6Database[IP6_SECURITY_PAD_LEN].hProperty,
        sizeof pSecTrailer->PadLen,
        &pSecTrailer->PadLen,
        0, 2, 0);      

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_NEXT_HEADER].hProperty,
                sizeof pSecTrailer->NextHeader,
                &pSecTrailer->NextHeader,
                0, 2, 0);   

    AttachPropertyInstance(hFrame,
        IP6Database[IP6_AUTH_DATA].hProperty,
        sizeof pSecTrailer->AuthData,
        pSecTrailer->AuthData,
        0, 2, 0);
}


VOID WINAPIV IP6AttachAuth(HFRAME hFrame, LPIP6_AUTH_HEADER pAuth)
{
    DWORD Length;

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_AUTH].hProperty,
                IP6AuthHeaderLength(pAuth),
                pAuth,
                0, 1, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_NEXT_HEADER].hProperty,
                sizeof pAuth->NextHeader,
                &pAuth->NextHeader,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_EXT_LENGTH].hProperty,
                sizeof pAuth->Length,
                &pAuth->Length,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_RESERVED].hProperty,
                sizeof pAuth->Reserved,
                &pAuth->Reserved,
                0, 2, 0);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SPI].hProperty,
                sizeof pAuth->SPI,
                &pAuth->SPI,
                0, 2, IFLAG_SWAPPED);

    AttachPropertyInstance(hFrame,
                IP6Database[IP6_SEQ_NUM].hProperty,
                sizeof pAuth->SeqNum,
                &pAuth->SeqNum,
                0, 2, IFLAG_SWAPPED);

    Length = IP6AuthHeaderLength(pAuth);

    if (Length > sizeof *pAuth) {

        AttachPropertyInstance(hFrame,
                    IP6Database[IP6_AUTH_DATA].hProperty,
                    Length - sizeof *pAuth,
                    (LPBYTE)(pAuth + 1),
                    0, 2, 0);
    }
}


//=============================================================================
//  FUNCTION: IP6FormatString()
//
//=============================================================================

VOID WINAPIV IP6FormatString(LPPROPERTYINST lpPropertyInst)
{
    strcpy(lpPropertyInst->szPropertyText,
           lpPropertyInst->lpPropertyInstEx->Byte);
}


//=============================================================================
//  FUNCTION: IP6FormatNamedByte()
//
//=============================================================================

VOID WINAPIV IP6FormatNamedByte(LPPROPERTYINST lpPropertyInst)
{
    BYTE Byte;
    LPSET pSet;
    LPSTR pName;
    LPSTR p;

    if (lpPropertyInst->DataLength == (WORD)-1)
        Byte = lpPropertyInst->lpPropertyInstEx->Byte[0];
    else
        Byte = lpPropertyInst->lpByte[0];

    pSet = lpPropertyInst->lpPropertyInfo->lpSet;

    pName = LookupByteSetString(pSet, Byte);

    p = lpPropertyInst->szPropertyText;

    p += sprintf(p, "%s = %u", lpPropertyInst->lpPropertyInfo->Label, Byte);

    if (pName != NULL)
        p += sprintf(p, " (%s)", pName);
}


//=============================================================================
//  FUNCTION: IP6FormatAddress()
//
//=============================================================================

VOID WINAPIV IP6FormatAddress(LPPROPERTYINST lpPropertyInst)
{
    LPSTR sz = lpPropertyInst->szPropertyText;
    LPBYTE lpData;

    if (lpPropertyInst->DataLength == (WORD) -1)
        lpData = lpPropertyInst->lpPropertyInstEx->Byte;
    else
        lpData = lpPropertyInst->lpData;

    sz += sprintf(sz, "%s = ", lpPropertyInst->lpPropertyInfo->Label);

    IP6FormatAddressWorker(sz, (LPIP6_ADDRESS)lpData);
}


//=============================================================================
//  FUNCTION: IP6FormatAddressWorker ()
//
//=============================================================================

LPSTR WINAPIV IP6FormatAddressWorker(LPSTR sz, LPIP6_ADDRESS pAddr)
{
    // Check for IPv6-compatible and IPv4-mapped addresses

    if ((pAddr->Word[0] == 0) && (pAddr->Word[1] == 0) &&
        (pAddr->Word[2] == 0) && (pAddr->Word[3] == 0) &&
        (pAddr->Word[4] == 0) &&
        ((pAddr->Word[5] == 0) || (pAddr->Word[5] == 0xffff)) &&
        (pAddr->Word[6] != 0)) {

        sz += sprintf(sz, "::%s%u.%u.%u.%u",
                      pAddr->Word[5] == 0 ? "" : "ffff:",
                      pAddr->Byte[12], pAddr->Byte[13],
                      pAddr->Byte[14], pAddr->Byte[15]);
    }
    else {
        int maxFirst, maxLast;
        int curFirst, curLast;
        int i;

        // Find largest contiguous substring of zeroes
        // A substring is [First, Last), so it's empty if First == Last.

        maxFirst = maxLast = 0;
        curFirst = curLast = 0;

        for (i = 0; i < 8; i++) {

            if (pAddr->Word[i] == 0) {
                // Extend current substring
                curLast = i+1;

                // Check if current is now largest
                if (curLast - curFirst > maxLast - maxFirst) {

                    maxFirst = curFirst;
                    maxLast = curLast;
                }
            }
            else {
                // Start a new substring
                curFirst = curLast = i+1;
            }
        }

        // Ignore a substring of length 1.

        if (maxLast - maxFirst <= 1)
            maxFirst = maxLast = 0;

        // Write colon-separated words.
        // A double-colon takes the place of the longest string of zeroes.
        // All zeroes is just "::".

        for (i = 0; i < 8; i++) {

            // Skip over string of zeroes
            if ((maxFirst <= i) && (i < maxLast)) {

                sz += sprintf(sz, "::");
                i = maxLast-1;
                continue;
            }

            // Need colon separator if not at beginning
            if ((i != 0) && (i != maxLast))
                sz += sprintf(sz, ":");

            sz += sprintf(sz, "%x", XCHG(pAddr->Word[i]));
        }
    }

    return sz;
}


//=============================================================================
//  FUNCTION: IP6FormatProperties()
//
//=============================================================================

DWORD WINAPI IP6FormatProperties(HFRAME         hFrame,
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

VOID WINAPIV IP6FormatChecksum(LPPROPERTYINST lpPropertyInst)
{
    LPSTR Label = lpPropertyInst->lpPropertyInfo->Label;
    LPSTR s = lpPropertyInst->szPropertyText;
    LPPROPERTYINSTEX pInstEx = lpPropertyInst->lpPropertyInstEx;
    WORD ActualChecksum;
    WORD FrameChecksum;

    // We assume InstEx and WORD data.

    ActualChecksum = *(ULPWORD)pInstEx->Word;
    FrameChecksum = *(ULPWORD)pInstEx->lpData;

    // Note CalcChecksum never returns zero, so we can use that value
    // as a flag to indicate. However the checksum in the frame
    // might be zero, and zero and 0xffff are equivalent
    // in ones-complement arithmetic.

    if (ActualChecksum == 0) {

        s += sprintf(s, "%s = 0x%.4X (truncated packet, unable to verify)",
                     Label, XCHG(FrameChecksum));
    }
    else if (ActualChecksum ==
             ((FrameChecksum == 0) ? 0xffff : FrameChecksum)) {

        s += sprintf(s, "%s = 0x%.4X",
                     Label, XCHG(FrameChecksum));
    }
    else {
        s += sprintf(s, "%s = 0x%.4X (ERROR: should be 0x%.4X)",
                     Label, XCHG(FrameChecksum), XCHG(ActualChecksum));
        lpPropertyInst->IFlags |= IFLAG_ERROR;
    }
}

WORD WINAPIV CalcChecksum(LPIP6_PSEUDO_HEADER pPsuHdr, LPBYTE Packet, DWORD Length, DWORD ChecksumOffset)
{
    DWORD Checksum = 0;
    DWORD i;

    // Add in the pseudo-header.

    for (i = 0; i < sizeof *pPsuHdr / sizeof(WORD); i++)
        Checksum += ((ULPWORD)pPsuHdr)[i];

    // Add in the packet (except for the checksum field).

    for (i = 0; i+1 < Length; i += 2) {

        if (i != ChecksumOffset)
            Checksum += *(ULPWORD)Packet;

        Packet += 2;
    }

    // Add in the odd byte at the end.

    if (i < Length)
        Checksum += (WORD)*Packet;

    // Reduce to 16 bits and take complement.

    while (Checksum >> 16)
        Checksum = (Checksum & 0xffff) + (Checksum >> 16);

    Checksum = ~Checksum;

    // Unless zero results - replace with ones.

    if (Checksum == 0)
        Checksum = 0xffff;

    return (WORD)Checksum;
}
