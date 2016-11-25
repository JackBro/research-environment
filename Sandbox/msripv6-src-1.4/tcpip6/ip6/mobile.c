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
// Mobility routines for Internet Protocol Version 6.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "mobile.h"
#include "route.h"
#include "security.h"
#include "ipsec.h"

int MobilitySecurity = MOBILITY_SECURITY_OFF;  // Default is off.


//* IPv6SendBindingAck
//
//  Send a BindingAcknowledgement indicating that we accepted/rejected
//  the mobile node's binding update.
//
void
IPv6SendBindingAck(
    IPv6Packet *V6Packet,
    IPv6BindingUpdateOption *BindingUpdate,
    IPv6Addr *DestAddr,
    uint DestScopeId,
    uchar StatusCode)
{
    Interface *IF;
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    uint Offset;
    uchar *Mem;
    uint MemLen;
    uint PayloadLength;
    IPv6Header *IP;
    MobileAcknowledgementOption *MA;
    IP_STATUS IPStatus;
    RouteCacheEntry *RCE;

    IPStatus = RouteToDestination(DestAddr, DestScopeId, NULL,
                                  RTD_FLAG_NORMAL, &RCE);
    if (IPStatus != IP_SUCCESS) {
        //
        // No route - drop the packet.
        //
        KdPrint(("IPv6SendBindingAck - no route: %x\n", IPStatus));
        return;
    }

    // Determine size of memory buffer needed.
    Offset = RCE->NCE->IF->LinkHeaderSize;
    PayloadLength = sizeof(MobileAcknowledgementOption);
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    // Allocate the packet.
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrint(("IPv6SendBindingAck: Couldn't allocate packet header!?!\n"));
        return;
    }

    // Prepare IP header of reply packet.
    IP = (IPv6Header *)(Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_DEST_OPTS;
    IP->HopLimit = RCE->NCE->IF->CurHopLimit;
    IP->Dest = *DestAddr;
    IP->Source = RCE->NTE->Address;

    // Prepare the binding acknowledgement option.
    MA = (MobileAcknowledgementOption UNALIGNED *)(IP + 1);
    MA->Header.NextHeader = IP_PROTOCOL_NONE;
    MA->Header.HeaderExtLength = 1;
    MA->Option.Type = OPT6_BINDING_ACK;
    MA->Option.Length = 11;
    MA->Option.Status = StatusCode;
    MA->Option.SeqNumber = BindingUpdate->SeqNumber;
    if (StatusCode == OPT6_BINDING_ACK) {
        MA->Option.Lifetime = BindingUpdate->Lifetime;
        MA->Option.Refresh = BindingUpdate->Lifetime;
    } else {
        MA->Option.Lifetime = 0;
        MA->Option.Refresh = 0;
    }
    MA->Pad1 = 0;

    IPv6Send2(Packet, Offset, IP, PayloadLength, RCE, 0, 0, 0);
    // IPv6Send1(Packet, Offset, IP, PayloadLength, RCE);

    //
    // Release the route.
    //
    ReleaseRCE(RCE);
}


//* SubOptionProcessing - Routine for mobile ip sub-option processsing.
//
//  Mobile IPv6 destination options may themselves have options, see
//  section 5.5 of the draft.  This routine parses these sub-options.
//
//  Returns TRUE if the sub-options were successfully processed.
//  Returns FALSE if the packet should be discarded.
//
int
SubOptionProcessing(
    uchar *SubOptPtr,           // Start of the sub-option data.
    uint SubOptSizeLeft,        // Length remaining in the parent option.
    MobileSubOptions *SubOpts)  // Return sub-option values to caller.
{
    SubOptionHeader *SubOptHdr;
    uint SubOptLen;

    //
    // Zero out the SubOpts struct that is returned.
    //
    RtlZeroMemory(SubOpts, sizeof *SubOpts);

    //
    // Note that if there are multiple sub-options
    // of the same type, we just use the last one encountered.
    //

    while (SubOptSizeLeft > 0) {
        //
        // First we check the option length and ensure that it fits.
        // We move OptPtr past this option while leaving OptHdr
        // for use by the option processing code below.
        //

        SubOptHdr = (SubOptionHeader *) SubOptPtr;

        if ((sizeof *SubOptHdr > SubOptSizeLeft) ||
            ((SubOptLen = sizeof *SubOptHdr + SubOptHdr->DataLength) >
             SubOptSizeLeft)) {
            //
            // Bad length.  REVIEW: Should we discard the packet or continue
            // processing it?  For now, discard it.
            //
            return FALSE;
        }

        SubOptPtr += SubOptLen;
        SubOptSizeLeft -= SubOptLen;

        switch (SubOptHdr->Type) {
        case SUBOPT6_UNIQUE_ID:
            SubOpts->UniqueId = (IPv6UniqueIdSubOption *)(SubOptHdr);
            break;

        case SUBOPT6_HOME_AGENTS_LIST:
            SubOpts->HomeAgentsList = (IPv6HomeAgentsListSubOption *)
                (SubOptHdr);
            break;

            //
            // Unrecognized sub-options are to be silently skipped.
            //
        }
    }

    return TRUE;
}


//* IPv6RecvBindingUpdate - handle an incoming binding update.
//
//  Process the receipt of a binding update destination option
//  from a mobile node.
//
//  BUGBUG: Right now, we're blindly trusting the home address we're given.
//  BUGBUG: Check these somehow?  For one thing, if it is a scoped address,
//  BUGBUG: we have no way to determine its scope id.  And for another,
//  BUGBUG: accepting some addresses (like say loopback) would be way bad.
//
int
IPv6RecvBindingUpdate(
    IPv6Packet *Packet,                      // Packet received.
    IPv6BindingUpdateOption *BindingUpdate,  // Binding update option.
    IPv6Addr *HomeAddr)                      // Mobile node's home address.
{
    IPv6Addr *CareOfAddr;
    uint CareOfScopeId;
    IP_STATUS Status;
    uint OptBytesLeft, OptsLen;
    MobileSubOptions SubOpts;

    //
    // While the mobility spec requires that packets containing binding
    // update options be authenticated, we currently allow this to be
    // turned off for interoperability testing with mobility implementations
    // that don't support IPSec yet.
    //
    if (MobilitySecurity == MOBILITY_SECURITY_ON) {
        //
        // Check if the packet went through some security.
        //
        // BUGBUG?: This doesn't check that use of this security association
        // BUGBUG?: actually falls within a security policy.
        //
        if (Packet->SAPerformed == NULL) {
            KdPrint(("IPv6RecvBindingUpdate: IPSec required "
                     "for binding update\n"));
            return 1;
        }
    }

    //
    // Examine binding update option to determine if the care-of address
    // is specified and if any sub-options exist.
    //

    OptsLen = BindingUpdate->Length + sizeof(OptionHeader);

    //
    // Check if the care-of address is in the binding update,
    // and extract it if so.
    //
    if (BindingUpdate->Flags & IPV6_BINDING_CARE_OF_ADDR) {
        // Care of address present.
        CareOfAddr = &BindingUpdate->CareOfAddr;
        OptBytesLeft = OptsLen - sizeof(IPv6BindingUpdateOption);
    } else {
        // No care-of address in binding update.
        CareOfAddr = &Packet->IP->Source;
        OptBytesLeft = OptsLen - (sizeof(IPv6BindingUpdateOption) -
            sizeof(IPv6Addr));
    }

    //
    // Determine scope id for care-of address.
    // REVIEW: If care-of address is scoped, and present in the binding update,
    // REVIEW: then this could potentially give us the wrong scope id if the
    // REVIEW: specified address differs from the packet's source.
    // REVIEW: Refuse specified care-of addresses that are scoped?
    //
    CareOfScopeId = DetermineScopeId(CareOfAddr, Packet->NTEorIF->IF);

    //
    // Check to make sure we have a reasonable care-of address.
    // Not required by spec but seems like a good idea.
    // REVIEW: Aren't link-local addresses o.k. as care-of addresses?
    //
    if (!IsUniqueAddress(CareOfAddr) || IsLoopback(CareOfAddr)
        || IsLinkLocal(CareOfAddr)) {
        IPv6SendBindingAck(Packet, BindingUpdate, CareOfAddr, CareOfScopeId,
            IPV6_BINDING_POORLY_FORMED);
        return 1;
    }

    if (OptBytesLeft) {
        //
        // Sub-options are present.  For now we don't do anything with
        // them, other than verify they are formed properly.
        // REVIEW: Send BindingAck on failure?
        //
        if (!SubOptionProcessing((uchar *) BindingUpdate + OptsLen
                                 - OptBytesLeft, OptBytesLeft, &SubOpts))
            return 1;
    }

    //
    // We don't support home agent functionality (yet).
    //
    if (BindingUpdate->Flags & IPV6_BINDING_HOME_REG) {
        IPv6SendBindingAck(Packet, BindingUpdate, CareOfAddr, CareOfScopeId,
                           IPV6_BINDING_HOME_REG_NOT_SUPPORTED);
        return 1;
    }

    //
    // Update our binding cache to reflect this binding update.
    //
    Status = CacheBindingUpdate(BindingUpdate, CareOfAddr, CareOfScopeId,
                                HomeAddr);
    if (Status != IP_SUCCESS) {
        // An RCE could not be created, so we'll reject this request.
        if (Status == IP_NO_RESOURCES)
            IPv6SendBindingAck(Packet, BindingUpdate, CareOfAddr,
                               CareOfScopeId, IPV6_BINDING_NO_RESOURCES);
        else
            IPv6SendBindingAck(Packet, BindingUpdate, CareOfAddr,
                               CareOfScopeId, IPV6_BINDING_REJECTED);
        return 1;
    }

    if (BindingUpdate->Flags & IPV6_BINDING_ACK) {
        //
        // The mobile node has requested an acknowledgement.  This ACK could be
        // delayed until the next packet is sent to the mobile node, but for
        // now we send one immediately.
        //
        IPv6SendBindingAck(Packet, BindingUpdate, HomeAddr, 0,
                           IPV6_BINDING_ACCEPTED);
    }

    // Done.
    return 0;
}


//* IPv6RecvHomeAddress - handle an incoming home address option.
//
//  Process the receipt of a Home Address destination option.  
//
int
IPv6RecvHomeAddress(
    IPv6Packet *Packet,                  // Packet received.
    IPv6HomeAddressOption *HomeAddress)  // Home address option.
{
    IP_STATUS Status;
    uint OptBytesLeft, OptsLen;
    MobileSubOptions SubOpts;

    //
    // If any mobile sub-options exist, then find out which ones.
    // For now we don't do anything with them.
    //
    OptsLen = HomeAddress->Length + sizeof(OptionHeader);
    OptBytesLeft = OptsLen - sizeof(IPv6HomeAddressOption);    

    if (OptBytesLeft) {
        if (!SubOptionProcessing((uchar *) HomeAddress + OptsLen
                                 - OptBytesLeft, OptBytesLeft, &SubOpts))
            return 1;
    }

    //
    // Save the home address for use by upper layers.
    //
    Packet->SrcAddr = &HomeAddress->HomeAddress;

    // Done.
    return 0;
}
