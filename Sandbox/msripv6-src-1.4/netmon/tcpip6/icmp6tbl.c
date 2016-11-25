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
// Tables for the ICMPv6 parser
//

#include "tcpip6.h"
#include "ip6.h"
#include "icmp6.h"

LABELED_BYTE ICMP6Types[] =
{
    // Error messages
    { ICMP6_DESTINATION_UNREACHABLE, "Destination Unreachable" },
    { ICMP6_PACKET_TOO_BIG, "Packet Too Big" },
    { ICMP6_TIME_EXCEEDED, "Time Exceeded" },
    { ICMP6_PARAMETER_PROBLEM, "Parameter Problem" },

    // Informational messages
    { ICMP6_ECHO_REQUEST, "Echo Request" },
    { ICMP6_ECHO_REPLY, "Echo Reply" },
    { ICMP6_GROUP_MEMBERSHIP_QUERY, "Group Membership Query" },
    { ICMP6_GROUP_MEMBERSHIP_REPORT, "Group Membership Report" },
    { ICMP6_GROUP_MEMBERSHIP_REDUCTION, "Group Membership Reduction" },

    // Neighbor discovery messages
    { ICMP6_ROUTER_SOLICITATION, "Router Solicitation" },
    { ICMP6_ROUTER_ADVERTISEMENT, "Router Advertisement" },
    { ICMP6_NEIGHBOR_SOLICITATION, "Neighbor Solicitation" },
    { ICMP6_NEIGHBOR_ADVERTISEMENT, "Neighbor Advertisement" },
    { ICMP6_REDIRECT, "Redirect" },
};

SET ICMP6TypesSet =
{
   sizeof( ICMP6Types) / sizeof(LABELED_BYTE),
   ICMP6Types
};


LABELED_BYTE ICMP6DestinationUnreachableCodes[] =
{
    { 0,        "No route" },
    { 1,        "Administratively prohibited" },
    { 2,        "Beyond scope of source address" },
    { 3,        "Address unreachable" },
    { 4,        "Port unreachable" },
};

LABELED_BYTE ICMP6PacketTooBigCodes[] =
{
    { 0,        NULL },
};

LABELED_BYTE ICMP6TimeExceededCodes[] =
{
    { 0,        "Hop limit exceeded" },
    { 1,        "Fragment reassembly time exceeded" },
};

LABELED_BYTE ICMP6ParameterProblemCodes[] =
{
    { 0,        "Erroneous header field" },
    { 1,        "Unrecognized Next Header type" },
    { 2,        "Unrecognized IPv6 option" },
};

SET ICMP6ErrorCodeSets[] =
{
    { 0, NULL },

    {   // ICMP6_DESTINATION_UNREACHABLE
        sizeof( ICMP6DestinationUnreachableCodes) / sizeof(LABELED_BYTE),
        ICMP6DestinationUnreachableCodes
    },
    {   // ICMP6_PACKET_TOO_BIG
        sizeof( ICMP6PacketTooBigCodes) / sizeof(LABELED_BYTE),
        ICMP6PacketTooBigCodes
    },
    {   // ICMP6_TIME_EXCEEDED
        sizeof( ICMP6TimeExceededCodes) / sizeof(LABELED_BYTE),
        ICMP6TimeExceededCodes
    },
    {   // ICMP6_PARAMETER_PROBLEM
        sizeof( ICMP6ParameterProblemCodes) / sizeof(LABELED_BYTE),
        ICMP6ParameterProblemCodes
    },
};

LABELED_BIT ICMP6NeighborFlags[] =
{
    { 31, "Not router", "Router" },
    { 30, "Not solicited", "Solicited" },
    { 29, "Not override", "Override" },
};

SET ICMP6NeighborFlagsSet =
{
   sizeof( ICMP6NeighborFlags) / sizeof(LABELED_BIT),
   ICMP6NeighborFlags
};


LABELED_BIT ICMP6RouterAdvertisementFlags[] =
{
    { 7, "Not managed address config", "Managed address configuration" },
    { 6, "Not other stateful config", "Other stateful configuration" },
};

SET ICMP6RouterAdvertisementFlagsSet =
{
   sizeof( ICMP6RouterAdvertisementFlags) / sizeof(LABELED_BIT),
   ICMP6RouterAdvertisementFlags
};


LABELED_BIT ICMP6PrefixFlags[] =
{
    { 7, "No on-link specification", "On-link determination allowed" },
    { 6, "Not autonomous address config", "Autonomous address configuration" },
    { 5, "No router address", "Router address provided" },
    { 4, "Not a site prefix", "Also a site prefix" },
    { 0, "Not a route prefix", "Route prefix provided" },
};

SET ICMP6PrefixFlagsSet =
{
   sizeof( ICMP6PrefixFlags) / sizeof(LABELED_BIT),
   ICMP6PrefixFlags
};
