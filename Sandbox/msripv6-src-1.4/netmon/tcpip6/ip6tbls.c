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
// Tables for the IPv6 parser
//

#include "tcpip6.h"
#include "ip6.h"

LABELED_BYTE IP6Protocols[] =
{
   { PROTO_HOPOPTS,  "IPv6 - Hop-by-Hop Options Header" },
   { PROTO_ICMP,     "ICMP - Internet Control Message" },
   { PROTO_IGMP,     "IGMP - Internet Group Management" },
   { PROTO_GGP,      "GGP - Gateway to Gateway" },
   { PROTO_ST,       "Stream" },
   { PROTO_TCP,      "TCP - Transmission Control" },
   { PROTO_UCL,      "UCL" },
   { PROTO_EGP,      "Exterior Gateway" },
   { PROTO_IGP,      "IGP - any interior gateway protocol" },
   { PROTO_BBNRCC,   "BBN RCC Monitoring" },
   { PROTO_NVP,      "Network Voice" },
   { PROTO_PUP,      "PUP" },
   { PROTO_ARGUS,    "ARGUS" },
   { PROTO_EMCON,    "EMCON" },
   { PROTO_XNET,     "Cross Net Debugger" },
   { PROTO_CHAOS,    "CHAOS", },
   { PROTO_USRDGRAM, "UDP - User Datagram" },
   { PROTO_MUX,      "Multiplexing" },
   { PROTO_DCN,      "DCN Measurement Subsystems" },
   { PROTO_HMP,      "Host Monitoring" },
   { PROTO_PRM,      "Packet Radio Measurement" },
   { PROTO_XNS_IDP,  "Xerox NS IDP" },
   { PROTO_TRUNK_1,  "Trunk-1" },
   { PROTO_TRUNK_2,  "Trunk-2" },
   { PROTO_LEAF_1,   "Leaf-1" },
   { PROTO_LEAF_2,   "Leaf-2" },
   { PROTO_RDP,      "Reliable Data" },
   { PROTO_IRTP,     "Internet Reliable Transaction" },
   { PROTO_ISO_TP4,  "ISO Transport Protocol Class 4" },
   { PROTO_NETBLT,   "Bulk Data Transfer" },
   { PROTO_MFE_NSP,  "MFE Network Services" },
   { PROTO_MERIT_INP,"MERIT Internodal" },
   { PROTO_SEP,      "Sequential Exchange" },
   { PROTO_3PC,      "Third Party Connect" },
   { PROTO_ROUTING,  "IPv6 - Routing Header" },
   { PROTO_FRAGMENT, "IPv6 - Fragment Header" },
   { PROTO_GRE,      "Generic Routing Encapsulation" },
   { PROTO_ESP,      "Encapsulating Security Payload Header" },
   { PROTO_AUTH,     "Authentication Header" },
   { PROTO_ICMP6,    "ICMPv6 - Internet Control Message Protocol version 6" },
   { PROTO_NONE,     "IPv6 - No Next Header" },
   { PROTO_DSTOPTS,  "IPv6 - Destination Options Header" },
   { PROTO_ANYHOST,  "any host internal protocol" },
   { PROTO_CFTP,     "CFTP" },
   { PROTO_LOCAL,    "any local network" },
   { PROTO_SATEXPAK, "SATNET and Backroom EXPAK" },
   { PROTO_RVD,      "MIT Remote Virtual Disk" },
   { PROTO_IPPC,     "Internet Pluribus Packet Core" },
   { PROTO_DFILESYS, "any distributed file system" },
   { PROTO_SATMON,   "SATNET Monitoring" },
   { PROTO_VISA,     "VISA" },
   { PROTO_IPCV,     "Internet Packet Core Utility" },
   { PROTO_BACKSAT,  "Backroom SATNET Monitoring" },
   { PROTO_SUN_ND,   "SUN ND PROTOCOL (Temporary)" }, // Temporary assignment
   { PROTO_WIDEMON,  "WIDEBAND Monitoring" },
   { PROTO_WIDEPAK,  "WIDEBAND EXPAK" },
   { PROTO_ISO_IP,   "ISO Internet Protocol" },
   { PROTO_VMTP,     "VMTP" },
   { PROTO_SECUREVMTP,"Secure VMTP" },
   { PROTO_VINES,    "VINES" },
   { PROTO_TTP,      "TTP" },
   { PROTO_NSFNETIGP,"NSFNET IGP" },
   { PROTO_DGP,      "Dissimilar Gateway" },
   { PROTO_TCF,      "TCF" },
   { PROTO_IGRP,     "IGRP" },
   { PROTO_OSPF,     "Open Shortest Path First IGP" },
   { PROTO_SPRITE_RPC,"Sprite RPC" },
   { PROTO_LARP,     "Locus Address Resolution" },

   { 255, "Reserved" }
};

SET IP6ProtocolsSet =
{
   sizeof( IP6Protocols) / sizeof(LABELED_BYTE),
   IP6Protocols
};


LABELED_BYTE IP6AbbrevProtocols[] =
{
   { PROTO_HOPOPTS,  "Hop-by-Hop Options Header" },
   { PROTO_ICMP,     "ICMP" },
   { PROTO_IGMP,     "IGMP" },
   { PROTO_GGP,      "GGP" },
   { PROTO_ST,       "Stream" },
   { PROTO_TCP,      "TCP" },
   { PROTO_UCL,      "UCL" },
   { PROTO_EGP,      "EGP" },
   { PROTO_IGP,      "IGP" },
   { PROTO_BBNRCC,   "BBN RCC" },
   { PROTO_NVP,      "NVP" },
   { PROTO_PUP,      "PUP" },
   { PROTO_ARGUS,    "ARGUS" },
   { PROTO_EMCON,    "EMCON" },
   { PROTO_XNET,     "XNET" },
   { PROTO_CHAOS,    "CHAOS", },
   { PROTO_USRDGRAM, "UDP" },
   { PROTO_MUX,      "MUX" },
   { PROTO_DCN,      "DCN" },
   { PROTO_HMP,      "HMP" },
   { PROTO_PRM,      "PRM" },
   { PROTO_XNS_IDP,  "XNS IDP" },
   { PROTO_TRUNK_1,  "Trunk-1" },
   { PROTO_TRUNK_2,  "Trunk-2" },
   { PROTO_LEAF_1,   "Leaf-1" },
   { PROTO_LEAF_2,   "Leaf-2" },
   { PROTO_RDP,      "RD" },
   { PROTO_IRTP,     "IRT" },
   { PROTO_ISO_TP4,  "TP4" },
   { PROTO_NETBLT,   "NETBLT" },
   { PROTO_MFE_NSP,  "MFE" },
   { PROTO_MERIT_INP,"MERIT INP" },
   { PROTO_SEP,      "SEP" },
   { PROTO_3PC,      "3PC" },
   { PROTO_ROUTING,  "Routing Header" },
   { PROTO_FRAGMENT, "Fragment Header" },
   { PROTO_GRE,      "GRE" },
   { PROTO_ESP,      "Security Header" },
   { PROTO_AUTH,     "Authentication Header" },
   { PROTO_ICMP6,    "ICMP6" },
   { PROTO_NONE,     "None" },
   { PROTO_DSTOPTS,  "Destination Options Header" },
   { PROTO_ANYHOST,  "ANYHOST" },
   { PROTO_CFTP,     "CFTP" },
   { PROTO_LOCAL,    "LOCAL" },
   { PROTO_SATEXPAK, "SATNET/EXPAK" },
   { PROTO_RVD,      "MIT RVD" },
   { PROTO_IPPC,     "IPPC" },
   { PROTO_DFILESYS, "DFS" },
   { PROTO_SATMON,   "SATNET Mon." },
   { PROTO_VISA,     "VISA" },
   { PROTO_IPCV,     "IPCU" },
   { PROTO_BACKSAT,  "SATNET Mon." },
   { PROTO_SUN_ND,   "SUN NDP" }, // Temporary assignment
   { PROTO_WIDEMON,  "WIDEBAND Mon." },
   { PROTO_WIDEPAK,  "WIDEBAND EXPAK" },
   { PROTO_ISO_IP,   "ISO IP" },
   { PROTO_VMTP,     "VMTP" },
   { PROTO_SECUREVMTP,"Secure VMTP" },
   { PROTO_VINES,    "VINES" },
   { PROTO_TTP,      "TTP" },
   { PROTO_NSFNETIGP,"NSFNET IGP" },
   { PROTO_DGP,      "DGP" },
   { PROTO_TCF,      "TCF" },
   { PROTO_IGRP,     "IGRP" },
   { PROTO_OSPF,     "OSPF" },
   { PROTO_SPRITE_RPC,"Sprite RPC" },
   { PROTO_LARP,     "LARP" },

   { 255, "Reserved" }
};

SET IP6AbbrevProtocolsSet =
{
   sizeof( IP6AbbrevProtocols) / sizeof(LABELED_BYTE),
   IP6AbbrevProtocols
};


LABELED_BIT IP6FragmentFlags[] =
{
    { 0, "No more fragments", "More fragments" },
};

SET IP6FragmentFlagsSet =
{
   sizeof( IP6FragmentFlags) / sizeof(LABELED_BIT),
   IP6FragmentFlags
};


LABELED_BYTE IP6Options[] =
{
    { IP6_OPTION_PAD1, "Pad1" },
    { IP6_OPTION_PADN, "PadN" },
    { IP6_OPTION_JUMBO, "Jumbo Payload" },
};

SET IP6OptionsSet =
{
   sizeof( IP6Options) / sizeof(LABELED_BYTE),
   IP6Options
};
