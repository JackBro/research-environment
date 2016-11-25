// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Mobile IPv6.
// See Mobility Support in IPv6 draft (Mobile IP working group) for details. 
//


#ifndef MOBILE_INCLUDED
#define MOBILE_INCLUDED 1


//
// Combined structure used for inserting Binding Acknowledgements
// into mobile IPv6 messages.
//
typedef struct MobileAcknowledgementOption {
    IPv6OptionsHeader Header;
    IPv6BindingAcknowledgementOption Option;
    uchar Pad1;
} MobileAcknowledgementOption;


typedef struct MobileSubOptions {
    IPv6UniqueIdSubOption *UniqueId;
    IPv6HomeAgentsListSubOption *HomeAgentsList;
} MobileSubOptions;

//
// Mobile external functions.
//
extern int
IPv6RecvBindingUpdate(
    IPv6Packet *Packet,
    IPv6BindingUpdateOption *BindingUpdate,
    IPv6Addr *HomeAddr);

extern int
IPv6RecvHomeAddress(
    IPv6Packet *Packet,
    IPv6HomeAddressOption *HomeAddress);

#endif  // MOBILE_INCLUDED
