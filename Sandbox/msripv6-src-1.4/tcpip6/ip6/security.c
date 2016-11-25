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
// IP security routines for Internet Protocol Version 6.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "ipsec.h"
#include "security.h"
#include "alloca.h"

//#define IPSEC_DEBUG

//
// Global Variables.
//
SecurityPolicy *DefaultSP = NULL, *FirstGlobalSP = NULL, *FirstCommonSP = NULL;
SecurityAssociation *FirstGlobalSA = NULL;

KSPIN_LOCK IPSecLock;

SecurityAlgorithm AlgorithmTable[MAX_ALGORITHM_ID + 1];

#ifdef IPSEC_DEBUG

void dump_encoded_mesg(uchar *buff, uint len)
{
    uint i, cnt = 0;
    uint bytes = 0;
    uint wrds = 0;
    uchar *buf = buff;

    for (i = 0; i < len; i++) {
        if (wrds == 0) {
            DbgPrint("&%02x:     ", cnt);
        }

        DbgPrint("%02x", *buf);
        buf++;
        bytes++;
        if (!(bytes % 4)) {
            DbgPrint(" ");
            bytes = 0;
        }
        wrds++;
        if (!(wrds % 16)) {
            DbgPrint("\n");
            wrds = 0;
            cnt += 16;
        }
    }

    DbgPrint("\n");
}

void DumpKey(uchar *buff, uint len)
{
    uint i;

    for (i = 0; i < len; i++) {
        DbgPrint("%c", (char *)buff[i]);
    }
    DbgPrint("\n");
}
#endif


//* GetDefaultSP
// MEF
void
DefaultSPforIF(Interface *IF)
{
    KIRQL OldIrql;
    KeAcquireSpinLock(&IPSecLock, &OldIrql);
    DefaultSP->RefCount++;
    KeReleaseSpinLock(&IPSecLock, OldIrql);
    IF->FirstSP = DefaultSP;
}

//* SPCheckAddr - Compare IP address in packet to IP address in policy.
//
uint
SPCheckAddr(
    IPv6Addr *PacketAddr,
    uint SPAddrField,
    IPv6Addr *SPAddr,
    IPv6Addr *SPAddrData)
{
    uint i, Result = FALSE;

    switch (SPAddrField) {

    case WILDCARD_VALUE:
        // Always a match since the address is don't care.
        Result = TRUE;
        break;

    case SINGLE_VALUE:
        // Check if the address of the packet matches the SP selector.
        if (IP6_ADDR_EQUAL(PacketAddr, SPAddr)) {
            Result = TRUE;
        }
        break;

    case RANGE_VALUE:
        Result = TRUE;
        //
        // Check the start of the range.
        //
        for (i = 0; i < 4; i++) {
            if (net_long(PacketAddr->s6_dwords[i]) >
                net_long(SPAddr->s6_dwords[i])) {
                break;
            } else {
                if (net_long(PacketAddr->s6_dwords[i]) <
                    net_long(SPAddr->s6_dwords[i])) {
                    Result = FALSE;
                    break;
                }
            }
        }

        if (Result == TRUE) {
            //
            // Check the end of the range.
            //
            for (i = 0; i < 4; i++) {
                if (net_long(PacketAddr->s6_dwords[i]) <
                    net_long(SPAddrData->s6_dwords[i])) {
                    break;
                } else {
                    if (net_long(PacketAddr->s6_dwords[i]) >
                         net_long(SPAddrData->s6_dwords[i])) {
                        Result = FALSE;
                        break;
                    }
                }
            }
        }

        //
        // Gets here if the start and end of the range are equal to each other
        // and the address being checked equals them.
        //
        break;

    default:
        //
        // Should never happen.
        //
        KdPrint(("SPCheckAddr: invalid value for SPAddrField (%u)\n",
                 SPAddrField));
    }

    return Result;
}


//* SPCheckPort - Compare port in packet to port in policy.
//
uint
SPCheckPort(
    ushort PacketPort,
    uint SPPortField,
    ushort SPPort,
    ushort SPPortData)
{
    uint Result = FALSE;

    switch (SPPortField) {

    case WILDCARD_VALUE:
        // Always a match since the port is don't care.
        Result = TRUE;
        break;

    case SINGLE_VALUE:
        // Check if the port of the packet matches the SP selector.
        if (PacketPort == SPPort) {
            Result = TRUE;
        }
        break;

    case RANGE_VALUE:
        // Check if port is between range.
        if (PacketPort >= SPPort && PacketPort <= SPPortData) {
            Result = TRUE;
        }
        break;

    default:
        //
        // Should never happen.
        //
        KdPrint(("SPCheckPort: invalid value for SPPortField (%u)\n",
                 SPPortField));
    }

    return Result;
}


//* CleanUpGlobalSAList
//
void
CleanUpGlobalSAList(void)
{
    SecurityAssociation *SA, *NextSA;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Start at the first SA entry.
    SA = FirstGlobalSA;

    while (SA != NULL) {
        NextSA = SA->Next;

        if (SA->RefCount == 0) {
            FreeSA(SA);
        }

        SA = NextSA;
    }

    KeReleaseSpinLock(&IPSecLock, OldIrql);
}


//* DeleteSA - Check if the SA is valid.
//
//  The SA is removed from the SA chain.  All pointers from the SA entry are
//  removed and the related reference counts decremented.  The SP pointers to
//  the SA can be removed; however, there could be references from the temp SA
//  holders used during IPSec traffic processing.
//
//  The temp SA references (IPSecProc and SALinkage) remove the references
//  when traffic processing is done.  The case can occur where the SA is
//  deleted but the temp SA holder still has a reference.  In that case,
//  the SA is not removed from the global list.
//
//  The global list clean up is handled by a periodically called function that
//  searches the global SA list looking for SA->RefCount equal to zero and
//  frees the memory.
//
int
DeleteSA(
    SecurityAssociation *SA)
{
    SecurityAssociation *FirstSA, *PrevSA = NULL;
    uint Direction;

    //
    // Get the start of the SA Chain.
    //
    Direction = SA->DirectionFlag;

    if (Direction == INBOUND) {
        FirstSA = SA->SecPolicy->InboundSA;
    } else {
        FirstSA = SA->SecPolicy->OutboundSA;
    }

    //
    // Find the invalid SA and keep track of the SA before it.
    //
    while (FirstSA != SA) {
        PrevSA = FirstSA;
        if (PrevSA == NULL) {
            // This is a problem it should never happen.
            // REVIEW: Can we change this to an ASSERT?
            KdPrint(("DeleteSA: SA was not found\n"));
            return FALSE;
        }
        FirstSA = FirstSA->ChainedSecAssoc;
    }

    //
    // Remove the SA from the SA Chain.
    //
    // Check if the invalid SA is the First SA of the chain.
    if (PrevSA == NULL) {
        // The invalid SA is the first SA so the SP needs to be adjusted.
        if (Direction == INBOUND) {
            SA->SecPolicy->InboundSA = FirstSA->ChainedSecAssoc;
        } else {
            SA->SecPolicy->OutboundSA = FirstSA->ChainedSecAssoc;
        }
    } else {
        // Just a entry in the Chain.
        PrevSA->ChainedSecAssoc = FirstSA->ChainedSecAssoc;
    }

    // Decrement the reference count of the SP.
    SA->SecPolicy->RefCount--;

    // Remove the reference to the SP.
    SA->SecPolicy = NULL;

    // Decrement the reference count of the SA.
    SA->RefCount--;

    if (SA->RefCount == 0) {
        //
        // Remove from the global list.
        //
        FreeSA(SA);
    } else {
        SA->Valid = SA_REMOVED;
    }

    return TRUE;
}


//* FixIFSPPointers - Fix the SP pointers in the interface's SP list.
//
//  Searches the interface's SP list starting at IF->FirstSP until
//  the first common security policy.  Fixes the SP->NextSecPolicy
//  pointers to remove the invalid SP.
//
int
FixIFSPPointers(
    Interface *IF,
    SecurityPolicy *SP)
{
    SecurityPolicy *IFSP, *PrevSP = NULL;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    IFSP = IF->FirstSP;

    while (IFSP != SP) {
        PrevSP = IFSP;
        if (PrevSP == FirstCommonSP) {
            KdPrint(("FixIFSPPointer: SP not found in interface list\n"));
            KeReleaseSpinLock(&IF->Lock, OldIrql);
            return FALSE;
        }
        IFSP = IFSP->NextSecPolicy;
    }

    // Check if the invalid SP is the first SP of the interface.
    if (PrevSP == NULL) {
        // The invalid SP is the first.
        IF->FirstSP = SP->NextSecPolicy;
    } else {
        // Just in the interface list.
        PrevSP->NextSecPolicy = SP->NextSecPolicy;
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    // Decrement the reference count of the SP.
    SP->RefCount--;

    return TRUE;
}


//* DeleteSP - Removes an SP entry from the kernel.
//
//  The removal of an SP makes all the SAs belonging to the SP invalid.
//  Unlike the SA removal, this removes every reference to the invalid SP.
//  Therefore, a check does not need to be made to ensure the SP is valid.
//
//  Called with the security lock held.
//
int
DeleteSP(
    SecurityPolicy *SP)
{
    SecurityPolicy *IFSP, *PrevSP = NULL;

    //
    // Remove the SP's SAs.
    //
    while (SP->InboundSA != NULL) {
        if (!(DeleteSA(SP->InboundSA))) {
            return FALSE;
        }
    }
    while (SP->OutboundSA != NULL) {
        if (!(DeleteSA(SP->OutboundSA))) {
            return FALSE;
        }
    }

    //
    // Find the invalid SP and keep track of the SP before it.
    //
    // Check if this is a common policy.
    if (SP->IFIndex == 0) {
        IFSP = FirstCommonSP;
        //
        // Walk the NextSecPolicy list to find the SP before.
        //
        while (IFSP != SP) {
            PrevSP = IFSP;
            if (PrevSP == NULL) {
                KdPrint(("DeleteSP: SP was not found in the common list\n"));
                return FALSE;
            }
            IFSP = IFSP->NextSecPolicy;
        }

        // Check if the invalid SP is the first common SP.
        if (PrevSP == NULL) {
            Interface *IF;
            uint i = 0;
            //
            // This is the first.  Need to walk every interface list
            // to fix the pointers.
            //
            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            for (IF = IFList; IF != NULL; IF = IF->Next) {
                AddRefIF(IF);
                KeReleaseSpinLockFromDpcLevel(&IFListLock);

                if (!FixIFSPPointers(IF, SP)) {
                    ReleaseIF(IF);
                    return FALSE;
                }
                i++;

                KeAcquireSpinLockAtDpcLevel(&IFListLock);
                ReleaseIF(IF);
            }
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            // Fix the FirstCommonSP pointer.
            FirstCommonSP = FirstCommonSP->NextSecPolicy;
            if (FirstCommonSP != NULL) {
                // Fix the FirstCommonSP reference count.
                FirstCommonSP->RefCount += (i-1);
            }
        } else {
            // Just in the common list.
            PrevSP->NextSecPolicy = SP->NextSecPolicy;
            SP->RefCount--;
        }
    } else {
        Interface *IF;
        int RetCode;

        // Specific policy.
        // Get the interface.
        IF = FindInterfaceFromIndex(SP->IFIndex);
        if (IF == NULL) {
            KdPrint(("DeleteSP: SP->IFIndex (%d) is an invalid interface "
                     "index\n", SP->IFIndex));
            return FALSE;
        }

        RetCode = FixIFSPPointers(IF, SP);
        ReleaseIF(IF);
        if (!RetCode)
            return FALSE;
    }

    //
    // Remove from the global list.
    //
    // Check if this is the only entry left.
    if (SP->Prev == NULL && SP->Next == NULL) {
        FirstGlobalSP = NULL;
    } else {
        // Check if this is the first entry.
        if (SP->Prev == NULL) {
            SP->Next->Prev = NULL;
            // Reset First pointer.
            FirstGlobalSP = SP->Next;
        } else {
            // Check if this is the last entry.
            if (SP->Next == NULL) {
                SP->Prev->Next = NULL;
            } else {
                // Delete this entry.
                SP->Next->Prev = SP->Prev;
                SP->Prev->Next = SP->Next;
            }
        }
    }

    // Check if this is part of an SA bundle.
    if (SP->SABundle != NULL) {
        SecurityPolicy *PrevSABundle, *NextSABundle;

        //
        // The SP pointer being removed is a middle or first SABundle pointer.
        //
        PrevSABundle = SP->PrevSABundle;
        NextSABundle = SP->SABundle;
        NextSABundle->PrevSABundle = PrevSABundle;

        if (PrevSABundle == NULL) {
            // First SABundle pointer.
            NextSABundle->RefCount--;
        } else {
            //
            // Clean up the SABundle deletion affects on other SP pointers.
            //
            while (PrevSABundle != NULL) {
                PrevSABundle->NestCount--;
                PrevSABundle->SABundle = NextSABundle;
                NextSABundle = PrevSABundle;
                PrevSABundle = PrevSABundle->PrevSABundle;
            }

            SP->RefCount--;
        }

        SP->RefCount--;
    }

    //
    // Check if anything else is referencing the invalid SP.
    // All the interfaces and SA references have been removed.
    // The only thing left are SABundle pointers.
    //
    if (SP->RefCount != 0) {
        SecurityPolicy *PrevSABundle, *NextSABundle;

        //
        // The SP pointer being removed is the last of the bundle pointers.
        //
        PrevSABundle = SP->PrevSABundle;
        NextSABundle = SP->SABundle;

        ASSERT(PrevSABundle != NULL);
        ASSERT(NextSABundle == NULL);

        PrevSABundle->RefCount--;

        //
        // Cleanup the SABundle deletion affects on other SP pointers.
        //
        while (PrevSABundle != NULL) {
            PrevSABundle->NestCount--;
            PrevSABundle->SABundle = NextSABundle;
            NextSABundle = PrevSABundle;
            PrevSABundle = PrevSABundle->PrevSABundle;
        }

        SP->RefCount--;

        // Now the reference count better be zero.
        if (SP->RefCount != 0) {
            KdPrint(("DeleteSP: The SP list is corrupt!\n"));
            return FALSE;
        }
    }

    // Free the memory.
    ExFreePool(SP);

    return TRUE;
}

//* FreeSA
//
void
FreeSA(
    SecurityAssociation *SA)
{
    //
    // Remove from the global list.
    //
    // Check if this is the only entry left.
    if (SA->Prev == NULL && SA->Next == NULL) {
        FirstGlobalSA = NULL;
    } else {
        // Check if this is the first entry.
        if (SA->Prev == NULL) {
            SA->Next->Prev = NULL;
            // Reset First pointer.
            FirstGlobalSA = SA->Next;
        } else {
            // Check if this is the last entry.
            if (SA->Next == NULL) {
                SA->Prev->Next = NULL;
            } else {
                // Delete this entry.
                SA->Next->Prev = SA->Prev;
                SA->Prev->Next = SA->Next;
            }
        }
    }

    // Free memory.
    ExFreePool(SA);
    SA = NULL;
}


//* FreeIPSecToDo
//
void
FreeIPSecToDo(
    IPSecProc *IPSecToDo,
    uint Number)
{
    uint i;

    for (i = 0; i < Number; i++) {
        if (--IPSecToDo[i].SA->RefCount == 0) {
            FreeSA(IPSecToDo[i].SA);
        }
    }

    ExFreePool(IPSecToDo);
    IPSecToDo = NULL;
}


//* InboundSAFind - find a SA in the Security Association Database.
//
//  A Security Association on a receiving machine is uniquely identified
//  by the tuple of SPI, IP Destination, and security protocol.
//
//  REVIEW: Since we can choose our SPI's to be system-wide unique, we
//  REVIEW: could do the lookup solely via SPI and just verify the others.
//
//  REVIEW: Should we do our IP Destination lookup via ADE?  Faster.
//
SecurityAssociation *
InboundSAFind(
    ulong SPI,       // Security Parameter Index.
    IPv6Addr *Dest,  // Destination address.
    uint Protocol)   // Security protocol in use (e.g. AH or ESP).
{
    SecurityAssociation *SA;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Start at the first SA entry.
    SA = FirstGlobalSA;

    while (SA != NULL) {
        // Check SPI.
        if (SPI == SA->SPI) {
            // Check destination IP address and IPSec protocol.
            if (IP6_ADDR_EQUAL(Dest, &SA->SADestAddr) &&
                (Protocol == SA->IPSecProto)) {

                // Check direction.
                if (SA->DirectionFlag == INBOUND) {
                    // Check if the SA entry is valid.
                    if (SA->Valid == SA_VALID) {
                        SA->RefCount++;
                        break;
                    }
                    // Not valid so continue checking.
                    continue;
                }
            }
        }

        SA = SA->Next;
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return SA;
}


//* InboundSALookup - Check the matched SP for a matching SA.
//
//  In the SABundle case, this function is called recursively to compare all
//  the SA entries.  Note, the selectors are not compared for SABundles.
//
uint
InboundSALookup(
    SecurityPolicy *SP,
    SALinkage *SAPerformed)
{
    SecurityAssociation *SA;
    uint Result = FALSE;

    for (SA = SP->InboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
        if (SA == SAPerformed->This && SA->DirectionFlag == INBOUND) {
            //
            // Check if the SP entry is a bundle.
            //
            if (SP->SABundle != NULL && SAPerformed->Next != NULL) {
                // Recursive call.
                if (InboundSALookup(SP->SABundle, SAPerformed->Next)) {
                    Result = TRUE;
                    break;
                }

            } else if (SP->SABundle == NULL && SAPerformed->Next == NULL) {
                // Found a match and no bundle to check.
                if (SA->Valid == SA_VALID) {
                    Result = TRUE;
                } else {
                    KdPrint(("InboundSALookup: Invalid SA\n"));
                }
                break;

            } else {
                // SAs in packet disagree with SABundle so no match.
                KdPrint(("InboundSALookup: SA seen disagrees with SA "
                         "in SABundle\n"));
                break;
            }
        }
    }

    return Result;
}


//* InboundSPLookup - IPsec processing verification.
//
//  This function is called from the transport layer.  The policy selectors
//  are compared with the packet to find a match.  The search continues
//  until there is a match.
//
//  The RFC says that the inbound SPD does not need to be ordered.
//  However if that is the case, then Bypass and discard mode couldn't
//  be used to quickly handle a packet.  Also since most of the SPs are
//  bidirectional, the SP entries are order.  We require the administrator
//  to order the policies.
//
int
InboundSPLookup(
    IPv6Packet *Packet,
    ushort TransportProtocol,
    ushort SourcePort,
    ushort DestPort,
    Interface *IF)
{
    SecurityPolicy *SP;
    int Result = FALSE;
    KIRQL OldIrql;

    // Get IPSec lock then get interface lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (SP = IF->FirstSP; SP != NULL; SP = SP->NextSecPolicy) {
        // Check Direction of SP.
        if (!(SP->DirectionFlag == INBOUND ||
              SP->DirectionFlag == BIDIRECTIONAL)) {
            continue;
        }

        // Check Remote Address.
        if (!SPCheckAddr(&Packet->IP->Source, SP->RemoteAddrField,
                         &SP->RemoteAddr, &SP->RemoteAddrData)) {
            continue;
        }

        // Check Local Address.
        if (!SPCheckAddr(&Packet->IP->Dest, SP->LocalAddrField,
                         &SP->LocalAddr, &SP->LocalAddrData)) {
            continue;
        }

        // Check Transport Protocol.
        if (SP->TransportProto == NONE) {
            // None so protocol passed.

        } else {
            if (SP->TransportProto != TransportProtocol) {
                continue;
            } else {
                // Check Remote Port.
                if (!SPCheckPort(SourcePort, SP->RemotePortField,
                                 SP->RemotePort, SP->RemotePortData)) {
                    continue;
                }

                // Check Local Port.
                if (!SPCheckPort(DestPort, SP->LocalPortField,
                                 SP->LocalPort, SP->LocalPortData)) {
                    continue;
                }
            }
        }

        // Check if the packet should be dropped.
        if (SP->SecPolicyFlag == IPSEC_DISCARD) {
            //
            // Packet is dropped by transport layer.
            // This essentially denies traffic.
            //
            break;
        }

        // Check if packet bypassed IPsec processing.
        if (Packet->SAPerformed == NULL) {
            //
            // Check if this is bypass mode.
            //
            if (SP->SecPolicyFlag == IPSEC_BYPASS) {
                // Packet is okay to be processed by transport layer.
                Result = TRUE;
                break;
            }
            // Check other policies, may change this to dropping later.
            continue;
        }

        //
        // Getting here means the packet saw an SA.
        //

        // Check IPsec mode.
        if (SP->IPSecSpec.Mode != Packet->SAPerformed->Mode) {
            //
            // Wrong mode for this traffic drop packet.
            //
            KdPrint(("InboundSPLookup: Wrong IPSec mode for traffic "
                     "Policy #%d\n", SP->Index));
            break;
        }

        // Check SA pointer.
        if (!InboundSALookup(SP, Packet->SAPerformed)) {
            //
            // SA lookup failed.
            //
            KdPrint(("InboundSPLookup: SA lookup failed Policy #%d\n",
                     SP->Index));
            break;
        }

        // Successful verification of IPsec.
        Result = TRUE;
        break;
    } // end of for (SP = IF->FirstSP; ...)

    // Release locks.
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* OutboundSALookup - Find a SA for the matching SP.
//
//  This function is called after an SP match is found.  The SAs associated
//  with the SP are searched for a match.  If match is found, a check to see
//  if the SP contains a bundle is done.  A bundle causes a similar lookup.
//  If any of the bundle SAs are not found, the lookup is a failure.
//
IPSecProc *
OutboundSALookup(
    IPv6Addr *SourceAddr,
    IPv6Addr *DestAddr,
    ushort TransportProtocol,
    ushort DestPort,
    ushort SourcePort,
    SecurityPolicy *SP,
    uint *Action)
{
    SecurityAssociation *SA;
    uint i;
    uint BundleCount = 0;
    IPSecProc *IPSecToDo = NULL;

    *Action = LOOKUP_DROP;

    //
    // Find the SA entry associated with the found SP entry.
    // If there is a bundle, a subsequent search finds the bundled SAs.
    //
    for (SA = SP->OutboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
        if (SP->RemoteAddrSelector == PACKET_SELECTOR) {
            // Check Remote Address.
            if (!IP6_ADDR_EQUAL(DestAddr, &SA->DestAddr)) {
                continue;
            }
        }

        if (SP->LocalAddrSelector == PACKET_SELECTOR) {
            // Check Remote Address.
            if (!IP6_ADDR_EQUAL(SourceAddr, &SA->SrcAddr)) {
                continue;
            }
        }

        if (SP->RemotePortSelector == PACKET_SELECTOR) {
            if (DestPort != SA->DestPort) {
                continue;
            }
        }

        if (SP->LocalPortSelector == PACKET_SELECTOR) {
            if (SourcePort != SA->SrcPort) {
                continue;
            }
        }

        if (SP->TransportProtoSelector == PACKET_SELECTOR) {
            if (TransportProtocol != SA->TransportProto) {
                continue;
            }
        }

        // Check if the SA is valid.
        if (SA->Valid != SA_VALID) {
            // SA is invalid continue checking.
            continue;
        }

        //
        // Match found.
        //
#ifdef IPSEC_DEBUG
        DbgPrint("Send using SP->Index=%d, SA->Index=%d\n",
                 SP->Index, SA->Index);
#endif
        BundleCount = SP->NestCount;

        // Allocate the IPSecToDo array.
        IPSecToDo = ExAllocatePool(NonPagedPool,
                                   (sizeof *IPSecToDo) * BundleCount);
        if (IPSecToDo == NULL) {
            KdPrint(("OutboundSALookup: "
                     "Couldn't allocate memory for IPSecToDo!?!\n"));
            return NULL;
        }

        //
        // Fill in IPSecToDo first entry.
        //
        IPSecToDo[0].SA = SA;
        SA->RefCount++;
        IPSecToDo[0].Mode = SP->IPSecSpec.Mode;
        IPSecToDo[0].BundleSize = SP->NestCount;
        *Action = LOOKUP_CONT;
        break;
    } // end of for (SA = SP->OutboundSA; ...)

    // Check if there is a bundled SA.
    for (i = 1; i < BundleCount; i++) {
        *Action = LOOKUP_DROP;

        // Check to make sure the bundle pointer is not null (safety check).
        if (SP->SABundle == NULL) {
            // This is bad so exit loop.
            // Free IPSecToDo memory.
            KdPrint(("OutboundSALookup: SP entry %d SABundle pointer is "
                     "NULL\n", SP->Index));
            FreeIPSecToDo(IPSecToDo, i);
            break;
        }

        SP = SP->SABundle;

        // Search through the SAs for this SP.
        for (SA = SP->OutboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
            if (SP->RemoteAddrSelector == PACKET_SELECTOR) {
                // Check Remote Address.
                if (!IP6_ADDR_EQUAL(DestAddr, &SA->DestAddr)) {
                    continue;
                }
            }

            if (SP->LocalAddrSelector == PACKET_SELECTOR) {
                // Check Remote Address.
                if (!IP6_ADDR_EQUAL(SourceAddr, &SA->SrcAddr)) {
                    continue;
                }
            }

            if (SP->RemotePortSelector == PACKET_SELECTOR) {
                if (DestPort != SA->DestPort) {
                    continue;
                }
            }

            if (SP->LocalPortSelector == PACKET_SELECTOR) {
                if (SourcePort != SA->SrcPort) {
                    continue;
                }
            }

            if (SP->TransportProtoSelector == PACKET_SELECTOR) {
                if (TransportProtocol != SA->TransportProto) {
                    continue;
                }
            }

            // Check if the SA is valid.
            if (SA->Valid != SA_VALID) {
                // SA is invalid continue checking.
                continue;
            }

            //
            // Match found.
            //

#ifdef IPSEC_DEBUG
            DbgPrint("Send using SP->Index=%d, SA->Index=%d\n",
                     SP->Index, SA->Index);
#endif
            //
            // Fill in IPSecToDo entry.
            //
            IPSecToDo[i].SA = SA;
            SA->RefCount++;
            IPSecToDo[i].Mode = SP->IPSecSpec.Mode;
            IPSecToDo[i].BundleSize = SP->NestCount;
            *Action = LOOKUP_CONT;
            break;
        } // end of for (SA = SP->OutboundSA; ...)

        // Check if the match was found.
        if (*Action == LOOKUP_DROP) {
            // No match so free IPSecToDo memory.
            FreeIPSecToDo(IPSecToDo, i);
            break;
        }
    } // end of for (i = 1; ...)

    return IPSecToDo;
}


//* OutboundSPLookup - Do the IPsec processing associated with an outbound SP.
//
//  This function is called from the transport layer to find an appropriate
//  SA or SABundle associated with the traffic.  The Outbound SPD is sorted
//  so the first SP found is for this traffic.  If the SP has an appropriate
//  SA, the pointer SAToPerform is completed.  If the SP does not have an SA,
//  the SAToPerform pointer is NULL.
//
IPSecProc *
OutboundSPLookup(
    IPv6Addr *SourceAddr,
    IPv6Addr *DestAddr,         // Pointer to IP Header.
    ushort TransportProtocol,   // Transport Protocol.
    ushort SourcePort,          // Source Port.
    ushort DestPort,            // Destination Port.
    Interface *IF,              // Interface Pointer.
    uint *Action)               // Action to do.
{
    SecurityPolicy *SP;
    KIRQL OldIrql;
    IPSecProc *IPSecToDo;

    IPSecToDo = NULL;
    *Action = LOOKUP_DROP;

    // Get IPSec lock then get interface lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (SP = IF->FirstSP; SP != NULL; SP = SP->NextSecPolicy) {
        // Check Direction of SP.
        if (!(SP->DirectionFlag == OUTBOUND ||
            SP->DirectionFlag == BIDIRECTIONAL)) {
            continue;
        }

        // Check Remote Address.
        if (!SPCheckAddr(DestAddr, SP->RemoteAddrField,
                         &SP->RemoteAddr, &SP->RemoteAddrData)) {
            continue;
        }

        // Check Local Address.
        if (!SPCheckAddr(SourceAddr, SP->LocalAddrField,
                         &SP->LocalAddr, &SP->LocalAddrData)) {
            continue;
        }

        // Check Transport Protocol.
        if (SP->TransportProto == NONE) {
            // None so protocol passed.

        } else {
            if (SP->TransportProto != TransportProtocol) {
                continue;
            } else {
                // Check Remote Port.
                if (!SPCheckPort(DestPort, SP->RemotePortField,
                                 SP->RemotePort, SP->RemotePortData)) {
                    continue;
                }

                // Check Local Port.
                if (!SPCheckPort(SourcePort, SP->LocalPortField,
                                 SP->LocalPort, SP->LocalPortData)) {
                    continue;
                }
            }
        }

        //
        // Check IPsec Action.
        //
        if (SP->SecPolicyFlag == IPSEC_APPLY) {
            // Search for an SA entry that matches.
            IPSecToDo = OutboundSALookup(SourceAddr, DestAddr,
                                         TransportProtocol, DestPort,
                                         SourcePort, SP, Action);
            if (IPSecToDo == NULL) {
                // No SA was found for the outgoing traffic.
                KdPrint(("OutboundSPLookup: No SA found for SP entry %d\n",
                    SP->Index));
                *Action = LOOKUP_DROP;
            }
        } else {
            if (SP->SecPolicyFlag == IPSEC_DISCARD) {
                // Packet is dropped.
                IPSecToDo = NULL;
                *Action = LOOKUP_DROP;
            } else {
                //
                // This is Bypass or "App determines" mode.
                // REVIEW: What is the app determine mode?
                //
                IPSecToDo = NULL;
                *Action = LOOKUP_BYPASS;
            }
        }
        break;
    } // end of for (SP = IF->FirstSP; ...)

    // Release locks.
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return IPSecToDo;
}


//* AuthenticationHeaderReceive - Handle an IPv6 AH header.
//
//  This is the routine called to process an Authentication Header,
//  next header value of 51.
//
uchar *
AuthenticationHeaderReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    AHHeader *AH;
    SecurityAssociation *SA;
    SecurityAlgorithm *Alg;
    uint ResultSize, AHHeaderLen;
    void *Context;
    uchar *Result, *AuthData;
    SALinkage *SAPerformed;
    uint SavePosition;
    void *SaveData;
    uint SaveContigSize;
    uint SaveTotalSize;
    void *SaveAuxList;
    uchar NextHeader;
    uint Dummy, Done;

    //
    // Verify that we have enough contiguous data to overlay an Authentication
    // Header structure on the incoming packet.  Then do so and skip over it.
    //
    if (Packet->ContigSize < sizeof(AHHeader)) {
        if (PacketPullup(Packet, sizeof(AHHeader)) == 0) {
            // Pullup failed.
            KdPrint(("AuthenticationHeaderReceive: Incoming packet too small"
                     " to contain authentication header\n"));
            return NULL;  // Drop packet.
        }
    }
    AH = (AHHeader *)Packet->Data;
    AdjustPacketParams(Packet, sizeof(AHHeader));

    //
    // Lookup Security Association in the Security Association Database.
    //
    SA = InboundSAFind(net_long(AH->SPI), &Packet->IP->Dest, IP_PROTOCOL_AH);
    if (SA == NULL){
        // No SA exists for this packet.
        // Drop packet.  NOTE: This is an auditable event.
        KdPrint(("AuthenticationHeaderReceive: No matching SA in database\n"));
        return NULL;
    }

    //
    // Verify the Sequence Number if required to do so by the SA.
    // BUGBUG: How are we going to store this requirement in the SA?
    //


    //
    // Perform Integrity check.
    //
    // First ensure that the amount of Authentication Data claimed to exist
    // in this packet by the AH header's PayloadLen field is large enough to
    // contain the amount that is required by the algorithm specified in the
    // SA.  Note that the former may contain padding to make it a multiple
    // of 32 bits.  Then check the packet size to ensure that it is big
    // enough to hold what the header claims is present.
    //
    AHHeaderLen = (AH->PayloadLen + 2) * 4;
    if (AHHeaderLen < sizeof (AHHeader)) {
        KdPrint(("AuthenticationHeaderReceive: Bogus AH header length\n"));
        return NULL;  // Drop packet.
    }
    AHHeaderLen -= sizeof(AHHeader);  // May include padding.
    Alg = &AlgorithmTable[SA->AlgorithmId];
    ResultSize = Alg->ResultSize;  // Does not include padding.
    if (ResultSize  > AHHeaderLen) {
        KdPrint(("AuthenticationHeaderReceive: Incoming packet's AHHeader"
                 " length inconsistent with algorithm's AuthData size\n"));
        return NULL;  // Drop packet.
    }
    if (Packet->ContigSize < AHHeaderLen) {
        if (PacketPullup(Packet, AHHeaderLen) == 0) {
            // Pullup failed.
            KdPrint(("AuthenticationHeaderReceive: Incoming packet too small"
                     " to contain authentication data\n"));
            return NULL;  // Drop packet.
        }
    }
    AuthData = (uchar *)Packet->Data;
    AdjustPacketParams(Packet, AHHeaderLen);

    //
    // AH authenticates everything (expect mutable fields) starting from
    // the previous IPv6 header.  Stash away our current position (so we can
    // restore it later) and backup to the previous IPv6 header.
    //
    SavePosition = Packet->Position;
    SaveData = Packet->Data;
    SaveContigSize = Packet->ContigSize;
    SaveTotalSize = Packet->TotalSize;
    SaveAuxList = Packet->AuxList;
    PositionPacketAt(Packet, Packet->IPPosition);
    Packet->AuxList = NULL;

    //
    // Initialize this particular algorithm.
    //
    Context = alloca(Alg->ContextSize);
    (*Alg->Initialize)(Context, SA->Key);

    //
    // Run algorithm over packet data.  We go to the end of the packet.
    //
    // BUGBUG: This isn't a general solution for mutable fields.
    // BUGBUG: This would be really simple if it were not for that.
    //
    // We know the IPv6 header is the first thing, so we can special-case it.
    //
    if (Packet->ContigSize < sizeof(IPv6Header)) {
        if (PacketPullup(Packet, sizeof(IPv6Header)) == 0) {
            KdPrint(("AuthenticationHeaderReceive: Out of memory!?!\n"));
            return NULL;
        }
    }
    //
    // REVIEW: Cache IPv6 header so we can give it to Operate as a single
    // REVIEW: chunk and avoid all these individual calls?  More efficient?
    //
    Dummy = IP_VERSION;  // Class and Flow Label are mutable.
#ifdef IPSEC_DEBUG
    DbgPrint("\nAH Receive Data:\n");
    dump_encoded_mesg((uchar *)&Dummy, 4);
    dump_encoded_mesg((uchar *)&Packet->IP->PayloadLength, 2);
    dump_encoded_mesg((uchar *)&Packet->IP->NextHeader, 1);
#endif
    (*Alg->Operate)(Context, SA->Key, (uchar *)&Dummy, 4);
    (*Alg->Operate)(Context, SA->Key, (uchar *)&Packet->IP->PayloadLength, 2);
    (*Alg->Operate)(Context, SA->Key, (uchar *)&Packet->IP->NextHeader, 1);
    NextHeader = Packet->IP->NextHeader;
    Dummy = 0;  // Hop Limit is mutable.
#ifdef IPSEC_DEBUG
    dump_encoded_mesg((uchar *)&Dummy, 1);
    dump_encoded_mesg((uchar *)&Packet->IP->Source, 2 * sizeof(IPv6Addr));
#endif
    (*Alg->Operate)(Context, SA->Key, (uchar *)&Dummy, 1);
    (*Alg->Operate)(Context, SA->Key, (uchar *)&Packet->IP->Source,
                    2 * sizeof(IPv6Addr));
    AdjustPacketParams(Packet, sizeof(IPv6Header));
    //
    // Continue over various extension headers until we reach the AH header.
    // We've parsed this far before, so we know these headers are legit.
    //
    for (Done = FALSE;!Done;) {
        switch (NextHeader) {

        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS: {
            IPv6OptionsHeader *Ext;
            uint HdrLen, Amount, SavedHdrLen;
            uchar *Start, *Current, *Zero;

            //
            // Pullup the extension header and all options into one
            // contiguous chunk.
            // REVIEW: We could pullup things separately if it's a problem.
            // REVIEW: Instead, this code is written to minimize Operate calls.
            //
            if (Packet->ContigSize < sizeof(ExtensionHeader)) {
                if (PacketPullup(Packet, sizeof(ExtensionHeader)) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
            Ext = (IPv6OptionsHeader *)Packet->Data;
            NextHeader = Ext->NextHeader;
            HdrLen = (Ext->HeaderExtLength + 1) * EXT_LEN_UNIT;
            SavedHdrLen = HdrLen; // Needed due to HdrLen--.
            if (Packet->ContigSize < HdrLen) {
                if (PacketPullup(Packet, HdrLen) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }

            //
            // Run through options to see if any are mutable.
            //
            Start = Packet->Data;
            Current = (uchar *)Packet->Data + sizeof(IPv6OptionsHeader);
            HdrLen -= sizeof(IPv6OptionsHeader);
            while (HdrLen) {

                if (*Current == OPT6_PAD_1) {
                    //
                    // This is the special one byte pad option.  Immutable.
                    //
                    Current++;
                    HdrLen--;
                    continue;
                }

                if (OPT6_ISMUTABLE(*Current)) {
                    //
                    // This option's data is mutable.  Everything preceeding
                    // the option data is not.
                    //
                    Amount = (Current - Start) + 2;  // Immutable amount.
#ifdef IPSEC_DEBUG
                    DbgPrint("DestOpts: \n");
                    dump_encoded_mesg(Start, Amount);
#endif
                    (*Alg->Operate)(Context, SA->Key, Start, Amount);

                    Current++;  // Now on option data length byte.
                    Amount = *Current;  // Mutable amount.
                    Zero = alloca(Amount);
                    RtlZeroMemory(Zero, Amount);
#ifdef IPSEC_DEBUG
                    dump_encoded_mesg(Zero, Amount);
#endif
                    (*Alg->Operate)(Context, SA->Key, Zero, Amount);

                    HdrLen -= Amount + 2;
                    Current += Amount + 1;
                    Start = Current;

                } else {

                    //
                    // This option's data is not mutable.
                    // Just skip over it.
                    //
                    Current++;  // Now on option data length byte.
                    Amount = *Current;
                    HdrLen -= Amount + 2;
                    Current += Amount + 1;
                }
            }
            if (Start != Current) {
                //
                // Option block ends with an immutable region.
                //
                Amount = Current - Start;
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Start, Amount);
#endif
                (*Alg->Operate)(Context, SA->Key, Start, Amount);
            }

            AdjustPacketParams(Packet, SavedHdrLen);
            break;
        }

        case IP_PROTOCOL_ROUTING: {
            //
            // Nothing is mutable.  The sender needed to predict this.
            //
            IPv6RoutingHeader *Route;
            uint HdrLen;

            if (Packet->ContigSize < sizeof(IPv6RoutingHeader)) {
                if (PacketPullup(Packet, sizeof(IPv6RoutingHeader)) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
            Route = (IPv6RoutingHeader *)Packet->Data;
            NextHeader = Route->NextHeader;
            HdrLen = (Route->HeaderExtLength + 1) * EXT_LEN_UNIT;
            if (Packet->ContigSize < HdrLen) {
                if (PacketPullup(Packet, HdrLen) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Packet->Data, HdrLen);
#endif
            (*Alg->Operate)(Context, SA->Key, Packet->Data, HdrLen);
            AdjustPacketParams(Packet, HdrLen);
            break;
        }

        case IP_PROTOCOL_AH: {
            AHHeader *ThisAH;
            uint ThisHdrLen;
            uint Padding;
            uchar *Zero;

            //
            // Skip over this AH header.
            //
            if (Packet->ContigSize < sizeof(AHHeader)) {
                if (PacketPullup(Packet, sizeof(AHHeader)) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
            ThisAH = (AHHeader *)Packet->Data;
            AdjustPacketParams(Packet, sizeof(AHHeader));
            ThisHdrLen = ((ThisAH->PayloadLen + 2) * 4) - sizeof(AHHeader);
            //
            // REVIEW: If we ever make AdjustPacketParams know how to skip
            // REVIEW: non-contiguous regions, we can remove this check.
            //
            if (Packet->ContigSize < ThisHdrLen) {
                if (PacketPullup(Packet, ThisHdrLen) == 0) {
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
            AdjustPacketParams(Packet, ThisHdrLen);

            //
            // If this is another AH header encapsulating the one we are
            // currently processing, then don't include it in the integrity
            // check as per AH spec section 3.3.
            //
            if (Packet->Position != SavePosition) {
                NextHeader = ThisAH->NextHeader;
                break;
            }

            //
            // Otherwise this is the AH header that we're currently processing,
            // so we include it in its own integrity check.  The Authentication
            // Data is mutable, the rest of the AH header is not.
            //
            ASSERT(Packet->TotalSize == SaveTotalSize);
#ifdef IPSEC_DEBUG
            dump_encoded_mesg((uchar *)AH, sizeof(AHHeader));
#endif
            (*Alg->Operate)(Context, SA->Key, (uchar *)AH, sizeof(AHHeader));

            //
            // REVIEW: Does NT have a guaranteed zero page I can use instead?
            //
            Zero = alloca(ResultSize);
            RtlZeroMemory(Zero, ResultSize);
#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Zero, ResultSize);
#endif
            (*Alg->Operate)(Context, SA->Key, Zero, ResultSize);

            //
            // The Authentication Data may be padded.
            //
            Padding = AHHeaderLen - ResultSize;
            if (Padding != 0) {
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Zero, Padding);
#endif
                (*Alg->Operate)(Context, SA->Key, Zero, Padding);
            }

            Done = TRUE;
            break;
        }

        case IP_PROTOCOL_ESP: {
            //
            // We don't include other IPSec headers in the integrity check
            // as per AH spec section 3.3.  So just skip over this.  Tricky
            // part is that the NextHeader was in the ESP trailer which we've
            // already thrown away at this point.
            //
            ESPHeader *ThisESP;
            ulong ThisSPI;
            SALinkage *ThisSAL;

            //
            // Get the SPI out of the ESP header so we can identify its
            // SALinkage entry on the SAPerformed chain.  Skip over the
            // ESP header while we're at it.
            //
            if (Packet->ContigSize < sizeof(ESPHeader)) {
                if (PacketPullup(Packet, sizeof(ESPHeader)) == 0) {
                    // Pullup failed.
                    KdPrint(("AuthenticationHeaderReceive: Out of mem!?!\n"));
                    return NULL;
                }
            }
            ThisESP = (ESPHeader *)Packet->Data;
            AdjustPacketParams(Packet, sizeof(ESPHeader));
            ThisSPI = net_long(ThisESP->SPI);

            //
            // Find the SALinkage entry on the SAPerformed chain with the
            // matching SPI.  It must be present.
            // REVIEW: This code assumes we made SPIs system-wide unique.
            //
            for (ThisSAL = Packet->SAPerformed;
                 ThisSAL->This->SPI != ThisSPI; ThisSAL = ThisSAL->Next)
                ASSERT(ThisSAL->Next != NULL);

            //
            // Pull NextHeader value out of the SALinkage (where we stashed
            // it back in EncapsulatingSecurityPayloadReceive).
            //
            NextHeader = ThisSAL->NextHeader;

            break;
        }

        default:
            // Unrecognized header.
            KdPrint(("AuthenticationHeaderReceive: Unsupported header = %d\n",
                     NextHeader));
            return NULL;
        }
    }

    //
    // Everything inside this AH header is treated as immutable.
    //
    // REVIEW: For performance reasons, the ContigSize check could be moved
    // REVIEW: before the loop for the additional code space cost of
    // REVIEW: duplicating the PacketPullup call.
    //
    while (Packet->TotalSize != 0) {

        if (Packet->ContigSize == 0) {
            //
            // Ran out of contiguous data.
            // Get next buffer in packet.
            //
            PacketPullup(Packet, 0);  // Pulls up entire next buffer.
        }
#ifdef IPSEC_DEBUG
        dump_encoded_mesg(Packet->Data, Packet->ContigSize);
#endif
        (*Alg->Operate)(Context, SA->Key, Packet->Data, Packet->ContigSize);
        AdjustPacketParams(Packet, Packet->ContigSize);
    }

    //
    // Get final result from the algorithm.
    //
    Result = alloca(ResultSize);
    (*Alg->Finalize)(Context, SA->Key, Result);
#ifdef IPSEC_DEBUG
        DbgPrint("Recv Key: ");
        DumpKey(SA->RawKey, SA->RawKeyLength);
        DbgPrint("Recv AuthData:\n");
        dump_encoded_mesg(Result, ResultSize);

        DbgPrint("Sent AuthData:\n");
        dump_encoded_mesg(AuthData, ResultSize);
#endif

    //
    // Compare result to authentication data in packet.  They should match.
    //
    if (RtlCompareMemory(Result, AuthData, ResultSize) != ResultSize) {
        KdPrint(("AuthenticationHeaderReceive: Failed integrity check\n"));
        return NULL;
    }

    //
    // Restore our packet position (to just after AH Header).
    //
    PacketPullupCleanup(Packet);
    Packet->Position = SavePosition;
    Packet->Data = SaveData;
    Packet->ContigSize = SaveContigSize;
    Packet->TotalSize = SaveTotalSize;
    Packet->AuxList = SaveAuxList;

    //
    // Add this SA to the list of those that this packet has passed.
    //
    SAPerformed = ExAllocatePool(NonPagedPool, sizeof *SAPerformed);
    if (SAPerformed == NULL) {
        return NULL;  // Drop packet.
    }
    SAPerformed->This = SA;
    SAPerformed->Next = Packet->SAPerformed;  // This SA is now first on list.
    SAPerformed->Mode = TRANSPORT;  // Assume trans until we see an IPv6Header.
    Packet->SAPerformed = SAPerformed;

    return &AH->NextHeader;
}


//* EncapsulatingSecurityPayloadReceive - Handle an IPv6 ESP header.
//
//  This is the routine called to process an Encapsulating Security Payload,
//  next header value of 50.
//
uchar *
EncapsulatingSecurityPayloadReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    ESPHeader *ESP;
    ESPTrailer *ESPT, TrailerBuffer;
    SecurityAssociation *SA;
    PNDIS_BUFFER NdisBuffer;
    SALinkage *SAPerformed;

    //
    // Verify that we have enough contiguous data to overlay an Encapsulating
    // Security Payload Header structure on the incoming packet.  Since the
    // authentication check covers the ESP header, we don't skip over it yet.
    //
    if (Packet->ContigSize < sizeof(ESPHeader)) {
        if (PacketPullup(Packet, sizeof(ESPHeader)) == 0) {
            // Pullup failed.
            KdPrint(("EncapsulatingSecurityPayloadReceive: Incoming packet too"
                     " small to contain ESP header\n"));
            return NULL;  // Drop packet.
        }
    }
    ESP = (ESPHeader *)Packet->Data;

    //
    // Lookup Security Association in the Security Association Database.
    //
    SA = InboundSAFind(net_long(ESP->SPI), &Packet->IP->Dest, IP_PROTOCOL_ESP);
    if (SA == NULL){
        // No SA exists for this packet.
        // Drop packet.  NOTE: This is an auditable event.
        KdPrint(("EncapsulatingSecurityPayloadReceive: "
                 "No SA found for the packet\n"));
        return NULL;
    }

    //
    // Verify the Sequence Number if required to do so by the SA.
    // BUGBUG: How are we going to store this requirement in the SA?
    //

    //
    // Perform integrity check if authentication has been selected.
    // BUGBUG: How are we going to store this requirement in the SA?
    //
    if (1) {  // BUGBUG: insert authentication selected check here.
        SecurityAlgorithm *Alg;
        uint ResultSize;
        uint PayloadLength;
        void *Context;
        IPv6Packet Clone;
        uint DoNow;
        uchar *AuthData;
        uchar *Result;

        //
        // First ensure that the incoming packet is large enough to hold the
        // Authentication Data required by the algorithm specified in the SA.
        // Then calculate the amount of data covered by authentication.
        //
        Alg = &AlgorithmTable[SA->AlgorithmId];
        ResultSize = Alg->ResultSize;
        if (Packet->TotalSize < ResultSize + sizeof(ESPTrailer)) {
            KdPrint(("EncapsulatingSecurityPaylofadReceive: "
                     "Packet too short to hold Authentication Data\n"));
            return NULL;
        }
        PayloadLength = Packet->TotalSize - ResultSize;

        //
        // Clone the packet positioning information so we can step through
        // the packet without losing our current place.  Start clone with
        // a fresh pullup history, however.
        //
        Clone = *Packet;
        Clone.AuxList = NULL;

#ifdef IPSEC_DEBUG
        DbgPrint("\nESP Receive Data:\n");
#endif
        //
        // Initialize this particular algorithm.
        //
        Context = alloca(Alg->ContextSize);
        (*Alg->Initialize)(Context, SA->Key);

        //
        // Run algorithm over packet data.
        // ESP authenticates everything beginning with the ESP Header and
        // ending just prior to the Authentication Data.
        //
        while (PayloadLength != 0) {
            DoNow = MIN(PayloadLength, Clone.ContigSize);

#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Clone.Data, DoNow);
#endif
            (*Alg->Operate)(Context, SA->Key, Clone.Data, DoNow);
            if (DoNow < PayloadLength) {
                //
                // Not done yet, must have run out of contiguous data.
                // Get next buffer in packet.
                //
                AdjustPacketParams(&Clone, DoNow);
                PacketPullup(&Clone, 0);  // Pulls up entire next buffer.
            }
            PayloadLength -= DoNow;
        }

        AdjustPacketParams(&Clone, DoNow);

        //
        // Get final result from the algorithm.
        //
        Result = alloca(ResultSize);
        (*Alg->Finalize)(Context, SA->Key, Result);

#ifdef IPSEC_DEBUG
        DbgPrint("Recv AuthData:\n");
        dump_encoded_mesg(Result, ResultSize);
#endif

        //
        // The Authentication Data immediately follows the region of
        // authentication coverage.  So our clone should be positioned
        // at the beginning of it.  Ensure that it's contiguous.
        //
        if (Clone.ContigSize < ResultSize) {
            if (PacketPullup(&Clone, ResultSize) == 0) {
                // Pullup failed.  Should never happen due to earlier check.
                KdPrint(("EncapsulatingSecurityPayloadReceive: Incoming "
                         "packet too small to contain Authenticaton Data\n"));
                return NULL;  // Drop packet.
            }
        }

        // Point to Authentication data.
        AuthData = Clone.Data;

#ifdef IPSEC_DEBUG
        DbgPrint("Sent AuthData:\n");
        dump_encoded_mesg(AuthData, ResultSize);
#endif
        //
        // Compare our result to the Authentication Data.  They should match.
        //
        if (RtlCompareMemory(Result, AuthData, ResultSize) != ResultSize) {
            //
            // Integrity check failed.  NOTE: This is an auditable event.
            //
            KdPrint(("EncapsulatingSecurityPayloadReceive: "
                     "Failed integrity check\n"));
            PacketPullupCleanup(&Clone);
            return NULL;
        }

        //
        // Done with the clone, clean up after it.
        //
        PacketPullupCleanup(&Clone);

        //
        // Truncate our packet to no longer include the Authentication Data.
        //
        Packet->TotalSize -= ResultSize;
        if (Packet->ContigSize > Packet->TotalSize)
            Packet->ContigSize = Packet->TotalSize;
    }

    //
    // We can consume the ESP Header now since it isn't
    // covered by confidentiality.
    //
    AdjustPacketParams(Packet, sizeof(ESPHeader));

    //
    // Decrypt Packet if confidentiality has been selected.
    // BUGBUG: How are we going to store this requirement in the SA?
    // Since we don't support encryption, we just make sure it isn't requested.
    //
    if (0) {  // BUGBUG: insert confidentiality selected check here.
        KdPrint(("EncapsulatingSecurityPaylofadReceive: "
                 "SA requested confidentiality -- unsupported feature\n"));
        return NULL;
    }

    //
    // Remove trailer and padding (if any).  Note that padding may appear
    // even in the no-encryption case in order to align the Authentication
    // Data on a four byte boundary.
    //
    if (Packet->NdisPacket == NULL) {
        //
        // This packet must be just a single contiguous region.
        // Finding the trailer is a simple matter of arithmetic.
        //
        ESPT = (ESPTrailer *)((uchar *)Packet->Data + Packet->TotalSize -
                              sizeof(ESPTrailer));
    } else {
        //
        // Need to find the trailer in the Ndis buffer chain.
        //
        // BUGBUG: Using GetDataFromNdis here results in the ESPTrailer mem
        // BUGBUG: being allocated off the stack rather than in Auxiliary
        // BUGBUG: data regions that GetPacketPositionFromPointer can find.
        // BUGBUG: Problematic since we return &ESPT->NextHeader;
        //
        NdisQueryPacket(Packet->NdisPacket, NULL, NULL, &NdisBuffer, NULL);
        ESPT = (ESPTrailer *)GetDataFromNdis(NdisBuffer, Packet->Position +
                                             Packet->TotalSize -
                                             sizeof(ESPTrailer),
                                             sizeof(ESPTrailer),
                                             (uchar *)&TrailerBuffer);
    }
    Packet->TotalSize -= sizeof(ESPTrailer);
    if (ESPT->PadLength > Packet->TotalSize) {
        KdPrint(("EncapsulatingSecurityPayloadReceive: "
                 "PadLength impossibly large (%u of %u bytes)\n",
                 ESPT->PadLength, Packet->TotalSize));
        return NULL;
    }
    Packet->TotalSize -= ESPT->PadLength;
    if (Packet->ContigSize > Packet->TotalSize)
        Packet->ContigSize = Packet->TotalSize;

    //
    // Add this SA to the list of those that this packet has passed.
    //
    SAPerformed = ExAllocatePool(NonPagedPool, sizeof *SAPerformed);
    if (SAPerformed == NULL) {
        return NULL;  // Drop packet.
    }
    SAPerformed->This = SA;
    SAPerformed->Next = Packet->SAPerformed;  // This SA is now first on list.
    SAPerformed->Mode = TRANSPORT;  // Assume trans until we see an IPv6Header.
    SAPerformed->NextHeader = ESPT->NextHeader;
    Packet->SAPerformed = SAPerformed;

    return &ESPT->NextHeader;
}


//* SetSPIndexValue - Set the SP Index and SA Bundle pointer of the SP entry.
//
uint
SetSPIndexValues(
    SecurityPolicy *SP,
    ulong SABundleIndex)
{
    uint Result = CREATE_SUCCESS;
    SecurityPolicy *MatchSP;
    ulong Index;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Verify this is not a common policy being entered after a
    // specific policy.
    //
    if (SP->IFIndex == 0) {
        Interface *IF;

        KeAcquireSpinLockAtDpcLevel(&IFListLock);
        for (IF = IFList; IF != NULL; IF = IF->Next) {
            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            if ((IF->FirstSP != NULL) &&
                (IF->FirstSP->IFIndex != 0)) {
                KeReleaseSpinLockFromDpcLevel(&IF->Lock);
                KeReleaseSpinLock(&IPSecLock, OldIrql);
                ReleaseIF(IF);
                return CREATE_INVALID_INTERFACE;
            }
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);

            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            ReleaseIF(IF);
        }
        KeReleaseSpinLockFromDpcLevel(&IFListLock);
    }

    // Check if this is the first SP to be entered.
    if (FirstGlobalSP == NULL) {
        Index = 0;
    } else {
        Index = FirstGlobalSP->Index;
    }

    // Check SP index value.
    if (SP->Index > Index) {

        //
        // Check SA Bundle Index value.
        //
        if (SABundleIndex == 0) {
            SP->SABundle = NULL;
        } else {
            // Search the SP List starting at the first SP.
            MatchSP = FindSecurityPolicyMatch(SABundleIndex);

            if (MatchSP == NULL) {
                Result = CREATE_INVALID_SABUNDLE;
            } else {
                SP->SABundle = MatchSP;
                SP->SABundle->RefCount++;
                SP->NestCount = SP->SABundle->NestCount + 1;
                //
                // Point from the SABundle to the SP referencing it.
                // This is done so one SP of the  SABundle can be deleted.
                // Increment the reference count of the SP also.
                //
                SP->SABundle->PrevSABundle = SP;
                SP->RefCount++;
            }
        }
    } else {
        Result = CREATE_INVALID_INDEX;
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* SetSPInitialInterface - Set the Interface SP pointer and the
//  NextSecPolicy pointer of the SP entry.
//
uint
SetSPInitialInterface(
    SecurityPolicy *SP)
{
    uint Result = CREATE_SUCCESS;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    if (SP->IFIndex == 0) {
        Interface *IF;

        // Set all the interfaces' FirstSP pointers to this policy.
        KeAcquireSpinLockAtDpcLevel(&IFListLock);
        for (IF = IFList; IF != NULL; IF = IF->Next) {
            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            SP->NextSecPolicy = IF->FirstSP;
            IF->FirstSP = SP;
            // Increment the reference count of the new first SP.
            SP->RefCount++;
            if (SP->NextSecPolicy != NULL) {
                // Decrement the reference count of the old first SP.
                SP->NextSecPolicy->RefCount--;
            }
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);

            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            ReleaseIF(IF);
        }
        KeReleaseSpinLockFromDpcLevel(&IFListLock);

        if (SP->NextSecPolicy != NULL) {
            //
            // Increment the former FirstCommonSP to account for the SecPolicy
            // pointer from the new FirstCommonSP.
            //
            SP->NextSecPolicy->RefCount++;
        }
    } else {
        Interface *IF;

        // Find the Interface.
        IF = FindInterfaceFromIndex(SP->IFIndex);
        if (IF == NULL) {
            Result = CREATE_INVALID_INTERFACE;
        } else {
            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            SP->NextSecPolicy = IF->FirstSP;
            IF->FirstSP = SP;            
            SP->RefCount++;
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);
            ReleaseIF(IF);
        }
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* AddSecurityPolicy - Add the SP entry onto the global list.
//
void
AddSecurityPolicy(
    SecurityPolicy *SP)  // New SP entry.
{
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Check if the SP list is empty.
    if (FirstGlobalSP == NULL) {
        SP->Next = NULL;
    } else {
        FirstGlobalSP->Prev = SP;
        SP->Next = FirstGlobalSP;
    }

    SP->Prev = NULL;
    FirstGlobalSP = SP;

    // Check if this is a common policy.
    if (SP->IFIndex == 0) {
        // Change the FirstCommonSP pointer.
        FirstCommonSP = SP;
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);
}


//* InsertSecurityPolicy
//
uint
InsertSecurityPolicy(
    SecurityPolicy *SP,
    ulong InsertIndex,
    ulong SPIndex,
    ulong SABundleIndex,
    uint SPInterface)
{
    uint Result = CREATE_SUCCESS;
    KIRQL OldIrql;
    SecurityPolicy *InsertSP;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Search the SP List starting at the first SP.
    InsertSP = FindSecurityPolicyMatch(InsertIndex);
    if (InsertSP == NULL) {
        // Release lock.
        KeReleaseSpinLock(&IPSecLock, OldIrql);
        return(CREATE_INVALID_INDEX);
    }

    // Check if this is the last SP.
    if (InsertSP->NextSecPolicy == NULL) {
        // Release lock.
        KeReleaseSpinLock(&IPSecLock, OldIrql);
        return(CREATE_INVALID_INDEX);
    }

    // Check that the SP being inserted is in the correct range.
    if (SPIndex < InsertSP->Index &&
        SPIndex > InsertSP->NextSecPolicy->Index) {
        SP->Index = SPIndex;

        // Check SABundle Index;
        if (SABundleIndex == 0) {
            SP->SABundle = NULL;
        } else {
            SecurityPolicy *NextSP;

            //
            // Search the SPs below InsertIndex.
            //
            NextSP = InsertSP;

            while (NextSP != NULL) {
                if (NextSP->Index == SABundleIndex) {
                    break;
                }

                // Set the pointer to the next SP.
                NextSP = NextSP->Next;
            }
            if (NextSP != NULL) {
                SP->SABundle = NextSP;
                SP->SABundle->RefCount++;
                SP->NestCount = SP->SABundle->NestCount + 1;
            } else {
                // Release lock.
                KeReleaseSpinLock(&IPSecLock, OldIrql);
                return (CREATE_INVALID_SABUNDLE);
            }
        }
    } else {
        // Release lock.
        KeReleaseSpinLock(&IPSecLock, OldIrql);
        return (CREATE_INVALID_INDEX);
    }

    SP->IFIndex = SPInterface;

    //
    // REVIEW: Interfaces can only share a common policy (all interfaces).
    //
    if (SPInterface == 0) {
        //
        // The new SP is for all interfaces (common) so the entry after
        // better be for all.
        //
        if (!(SPInterface == InsertSP->NextSecPolicy->IFIndex)) {
            Result = CREATE_INVALID_INDEX;
        }
    } else {
        //
        // The new SP is for a specific interface so the entry after better
        // be for the same interface or common and the SP before better be
        // the same interface.
        //
        if (!(SPInterface == InsertSP->IFIndex &&
            (SPInterface == InsertSP->NextSecPolicy->IFIndex ||
             InsertSP->NextSecPolicy->IFIndex == 0))) {
            Result = CREATE_INVALID_INDEX;
        }
    }
    if (Result != CREATE_SUCCESS) {
        // Release lock.
        KeReleaseSpinLock(&IPSecLock, OldIrql);
        return (CREATE_INVALID_INTERFACE);
    }

    //
    // Now insert the new SP in the global and NextSecPolicy lists.
    //

    // Set the new SP's NextSecPolicy pointer to the next entry.
    SP->NextSecPolicy = InsertSP->NextSecPolicy;

    //
    // Check if this is the being inserted before the first common SP.
    //
    if (InsertSP->NextSecPolicy == FirstCommonSP) {
        SecurityPolicy *IFSP;
        Interface *IF;
        uint i;

        //
        // This is the first common SP.
        // Have to walk every interface and set the last
        // specific SP's NextSecPolicy pointer to the new SP
        // being inserted.
        //

        KeAcquireSpinLockAtDpcLevel(&IFListLock);
        for (IF = IFList; IF != NULL; IF = IF->Next) {
            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            IFSP = IF->FirstSP;

            // Walk the interface SPs.
            while (IFSP->NextSecPolicy != NULL) {
                // Check if this is the last specific SP for this interface.
                if (IFSP->NextSecPolicy->IFIndex != IF->Index) {
                    // Check if this entry is a common.
                    if (IFSP->IFIndex != IF->Index) {
                        // Check if this the first SP of the interface.
                        if (IFSP == IF->FirstSP) {
                            // Change the first SP.
                            IF->FirstSP = SP;
                        } else {
                            IFSP->NextSecPolicy = SP;
                        }
                    } else {
                        IFSP->NextSecPolicy = SP;
                    }

                    // Increment the new FirstCommonSP.
                    SP->RefCount++;
                    // Decrement the old FirstCommonSP.
                    SP->NextSecPolicy->RefCount--;

                    break;
                }
                IFSP = IFSP->NextSecPolicy;
            } // end of while (IFSP->...)

            KeReleaseSpinLockFromDpcLevel(&IF->Lock);

            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            ReleaseIF(IF);
        } // end of for (IF = IFList...)
        KeReleaseSpinLockFromDpcLevel(&IFListLock);

        //
        // Increment the former FirstCommonSP to account for the SecPolicy
        // pointer from the new FirstCommonSP.
        //
        SP->NextSecPolicy->RefCount++;

    } else {
        InsertSP->NextSecPolicy = SP;
        SP->RefCount++;
    }

    //
    // Change global pointers.
    //
    SP->Prev = InsertSP;
    SP->Next = InsertSP->Next;
    if (SP->Next != NULL) {
        SP->Next->Prev = SP;
    }
    InsertSP->Next = SP;

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* SetSPSAPointer - Set the SP Outbound or Inbound SA pointer.
//
uint
SetSPSAPointer(
    SecurityPolicy *SP,
    SecurityAssociation *SA)
{
    uint Result = CREATE_SUCCESS;

    // Check the direction.
    if (SA->DirectionFlag == INBOUND) {
        if (SP->DirectionFlag == INBOUND ||
            SP->DirectionFlag == BIDIRECTIONAL) {
            // Add the SA to the Policy pointer.
            if (SP->InboundSA == NULL) {
                // Initialize ChainedSecAssoc pointer.
                SA->ChainedSecAssoc = NULL;
            } else {
                // Set the new SA ChainedSecAssoc pointer to the
                // current SP InboundSA pointer.
                SA->ChainedSecAssoc = SP->InboundSA;
            }

            // Add the new SA to the SP InboundSA pointer.
            SP->InboundSA = SA;
            SP->InboundSA->RefCount++;

            // Add the SP to the SA SecPolicy pointer.
            SA->SecPolicy = SP;
            SA->SecPolicy->RefCount++;

            // Set the IPSecProto in the SA.
            SA->IPSecProto = SP->IPSecSpec.Protocol;
        } else {
            Result = CREATE_INVALID_DIRECTION;
        }
    } else if (SA->DirectionFlag == OUTBOUND) {
        if (SP->DirectionFlag == OUTBOUND ||
            SP->DirectionFlag == BIDIRECTIONAL) {
            // Add the SA to the Policy pointer.
            if (SP->OutboundSA == NULL) {
                // Initialize ChainedSecAssoc pointer.
                SA->ChainedSecAssoc = NULL;
            } else {
                // Set the new SA ChainedSecAssoc pointer to the
                // current SP OutboundSA pointer.
                SA->ChainedSecAssoc = SP->OutboundSA;
            }

            // Add the new SA to the SP InboundSA pointer.
            SP->OutboundSA = SA;
            SP->OutboundSA->RefCount++;

            // Add the SP to the SA SecPolicy pointer.
            SA->SecPolicy = SP;
            SA->SecPolicy->RefCount++;

            // Set the IPSecProto in the SA.
            SA->IPSecProto = SP->IPSecSpec.Protocol;
        } else {
            Result = CREATE_INVALID_DIRECTION;
        }
    } else {
        Result = CREATE_INVALID_DIRECTION;
    }

    return Result;
}


//* SetSAIndexValues - Set the SA Index of the SA entry.
//
uint
SetSAIndexValues(
    SecurityAssociation *SA,
    ulong SAIndex,
    ulong SecPolicyIndex)
{
    uint Result = CREATE_SUCCESS;
    SecurityPolicy *MatchSP;
    ulong Index;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Check if this is the first SA to be entered.
    if (FirstGlobalSA == NULL) {
        Index = 0;
    } else {
        Index = FirstGlobalSA->Index;
    }

    // Check SP index value.
    if (SAIndex > Index) {
        SA->Index = SAIndex;

        // Search the SP List starting at the first SP.
        MatchSP = FindSecurityPolicyMatch(SecPolicyIndex);

        if (MatchSP == NULL) {
            Result = CREATE_INVALID_SEC_POLICY;
        } else {
            Result = SetSPSAPointer(MatchSP, SA);
        }
    } else {
        Result = CREATE_INVALID_INDEX;
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* InsertSecurityAssociation - Insert SA entry on global list.
//
void
InsertSecurityAssociation(
    SecurityAssociation *SA)  // New SA entry.
{
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Add the SA to the global list.
    //
    if (FirstGlobalSA == NULL) {
        SA->Next = NULL;
    } else {
        FirstGlobalSA->Prev = SA;
        SA->Next = FirstGlobalSA;
    }

    SA->Prev = NULL;
    FirstGlobalSA = SA;

    SA->Valid = SA_VALID;

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);
} // CreateSecurityAssociation


//* FindSecurityPolicyMatch - Find SP Entry corresponding to index value.
//
SecurityPolicy *
FindSecurityPolicyMatch(
    ulong Index)
{
    SecurityPolicy *NextSP;

    //
    // Search the SP List starting at the first SP.
    //
    NextSP = FirstGlobalSP;

    while (NextSP != NULL) {
        if (NextSP->Index == Index) {
            break;
        }

        // Set the pointer to the next SP.
        NextSP = NextSP->Next;
    }

    return NextSP;
}


//* FindSecurityAssociationMatch - Find SA Entry corresponding to index value.
//
SecurityAssociation *
FindSecurityAssociationMatch(
    ulong Index)
{
    SecurityAssociation *NextSA;

    //
    // Search the SA List starting at the first SA.
    //
    NextSA = FirstGlobalSA;

    while (NextSA != NULL) {
        if (NextSA->Index == Index) {
           break;
        }

        // Set the pointer to the next SA.
        NextSA = NextSA->Next;
    }

    return(NextSA);
}


//* GetFirstSecPolicy - Return first SP entry.
//
SecurityPolicy *
GetFirstSecPolicy(void)
{
    return FirstGlobalSP;
}


//* GetFirstSecAssociation - Return first SA entry.
//
SecurityAssociation *
GetFirstSecAssociation(void)
{
    return FirstGlobalSA;
}


//* GetSecPolicyIndex - Return SP Index or NONE.
//
ulong
GetSecPolicyIndex(
    SecurityPolicy *SP)
{
    ulong Index = NONE;

    // Get Index from SP.
    if (SP != NULL) {
        Index = SP->Index;
    }

    return Index;
}


//* GetSecAssocIndex - Return SA Index or NONE.
//
ulong
GetSecAssocIndex(
    SecurityAssociation *SA)
{
    ulong Index = NONE;

    // Get Index from SA.
    if (SA != NULL) {
            Index = SA->Index;
    }

    return Index;
}


//* IPsecInit - Initialize the Common SPD.
//
int
IPSecInit(void)
{
    SecurityPolicy *SP;
    Interface *IF;

    // Allocate memory for Security Policy.
    SP = ExAllocatePool(NonPagedPool, sizeof *SP);
    if (SP == NULL) {
        KdPrint(("IPsecInit - couldn't allocate pool for SP!?!\n"));
        return FALSE;
    }

    //
    // Initialize a default common policy that allows everything.
    //
    SP->Next = NULL;
    SP->Prev = NULL;

    SP->RemoteAddrField = WILDCARD_VALUE;
    SP->RemoteAddr = UnspecifiedAddr;
    SP->RemoteAddrData = UnspecifiedAddr;
    SP->RemoteAddrSelector = POLICY_SELECTOR;

    SP->LocalAddrField = WILDCARD_VALUE;
    SP->LocalAddr = UnspecifiedAddr;
    SP->LocalAddrData = UnspecifiedAddr;
    SP->LocalAddrSelector = POLICY_SELECTOR;

    SP->TransportProto = NONE;
    SP->TransportProtoSelector = POLICY_SELECTOR;

    SP->RemotePortField = WILDCARD_VALUE;
    SP->RemotePort = NONE;
    SP->RemotePortData = NONE;
    SP->RemotePortSelector = POLICY_SELECTOR;

    SP->LocalPortField = WILDCARD_VALUE;
    SP->LocalPort = NONE;
    SP->LocalPortData = NONE;
    SP->LocalPortSelector = POLICY_SELECTOR;

    SP->SecPolicyFlag = IPSEC_BYPASS;

    SP->IPSecSpec.Protocol = NONE;
    SP->IPSecSpec.Mode = NONE;
    SP->IPSecSpec.RemoteSecGWIPAddr = UnspecifiedAddr;

    SP->DirectionFlag = BIDIRECTIONAL;
    SP->OutboundSA = NULL;
    SP->InboundSA = NULL;
    SP->NextSecPolicy = NULL;
    SP->SABundle = NULL;
    SP->Index = 1;
    SP->RefCount = 0;
    SP->IFIndex = 0;

    //
    // The last Security Policy entered.
    // This is the global Security Policy list.
    //
    FirstGlobalSP = SP;
    FirstCommonSP = SP;

    // First Default Policy (most specific).
    DefaultSP = SP;

    KeInitializeSpinLock(&IPSecLock);

    //
    // Initialize the security algorithms table.
    //
    AlgorithmsInit();

    return(TRUE);
}


//* IPsecBytesToInsert
//
uint
IPSecBytesToInsert(
    IPSecProc *IPSecToDo,
    int *TunnelStart)
{
    uint i, Padding;
    uint BytesInHeader, BytesToInsert = 0;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;
    uint IPSEC_TUNNEL = FALSE;

    for (i = 0; i < IPSecToDo->BundleSize; i++) {
        SA = IPSecToDo[i].SA;
        Alg = &AlgorithmTable[SA->AlgorithmId];

        BytesInHeader = 0;
        //
        // Calculate bytes to insert for each IPSec header..
        //

        // Check if this is tunnel or transport mode.
        if (IPSecToDo[i].Mode == TUNNEL) {
            // Outer IPv6 header.
            BytesToInsert += sizeof(IPv6Header);

            if (!IPSEC_TUNNEL) {
                // Set the tunnel start location.
                *TunnelStart = i;
                IPSEC_TUNNEL = TRUE;
            }
        }

        // Check which IPSec protocol.
        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            // Check if padding needs to be added to AH Data.
            Padding = Alg->ResultSize % 4;
            if (Padding != 0) {
                BytesInHeader += (4 - Padding);
            }

            BytesInHeader += (sizeof(AHHeader) + Alg->ResultSize);
        } else {
            // ESP.
            BytesInHeader += sizeof(ESPHeader);
        }

        // Store the byte size of the IPSec header.
        IPSecToDo[i].ByteSize = BytesInHeader;

        // Add the IPSec header size to the total bytes to insert.
        BytesToInsert += BytesInHeader;
    }

    return BytesToInsert;
}


//* IPSecInsertHeaders
//
uint
IPSecInsertHeaders(
    uint Mode,              // Transport or Tunnel.
    IPSecProc *IPSecToDo,
    uchar **InsertPoint,
    uchar *NewMemory,
    PNDIS_PACKET Packet,
    uint *TotalPacketSize,
    uchar *PrevNextHdr,
    uint TunnelStart,
    uint *BytesInserted,
    uint *NumESPTrailers,
    uint *JUST_ESP)
{
    uint i, NumHeaders = 0;
    AHHeader *AH;
    ESPHeader *ESP;
    uchar NextHeader;
    uint Padding, j;
    ESPTrailer *ESPTlr;
    PNDIS_BUFFER ESPTlrBuffer, Buffer, NextBuffer;
    uchar *ESPTlrMemory;
    uint ESPTlrBufSize;
    NDIS_STATUS Status;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;
    uint Action = LOOKUP_CONT;
    uint BufferCount;

    NextHeader = *PrevNextHdr;

    if (Mode == TRANSPORT) {
        i = 0;
        if (TunnelStart != NO_TUNNEL) {
            NumHeaders = TunnelStart;
        } else {
            NumHeaders = IPSecToDo->BundleSize;
        }
    } else {
        // Tunnel.
        i = TunnelStart;
        // Get the end of the tunnels.
        for (j = TunnelStart + 1; j < IPSecToDo->BundleSize; j++) {
            if (IPSecToDo[j].Mode == TUNNEL) {
                // Another tunnel.
                NumHeaders = j;
                break;
            }
        }
        if (NumHeaders == 0) {
            // No other tunnels.
            NumHeaders = IPSecToDo->BundleSize;
        }
    }

    *JUST_ESP = TRUE;

    for (i; i < NumHeaders; i++) {
        SA = IPSecToDo[i].SA;

        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            *JUST_ESP = FALSE;

            // Move insert point up to start of AH Header.
            *InsertPoint -= IPSecToDo[i].ByteSize;

            *BytesInserted += IPSecToDo[i].ByteSize;

            AH = (AHHeader *)(*InsertPoint);

            //
            // Insert AH Header.
            //
            AH->NextHeader = NextHeader;
            // Set previous header's next header field to AH.
            NextHeader = IP_PROTOCOL_AH;
            // Payload length is in 32 bit counts.
            AH->PayloadLen = (IPSecToDo[i].ByteSize/4) - 2;
            AH->Reserved = 0;
            AH->SPI = net_long(SA->SPI);
            // BUGBUG: Need to handle cycle when IKE is used.
            // BUGBUG: Concurrent calls issue -- should atomically increment.
            AH->Seq = net_long(++SA->SequenceNum);

            //
            // Store point where to put AH Data after authentication.
            //
            IPSecToDo[i].AuthData = (*InsertPoint) + sizeof(AHHeader);

            // Set AH Data and padding to 0s (MUTABLE).
            RtlZeroMemory(IPSecToDo[i].AuthData,
                          IPSecToDo[i].ByteSize - sizeof(AHHeader));

        } else {
            // ESP.
            Alg = &AlgorithmTable[SA->AlgorithmId];

            // Move insert point up to start of ESP Header.
            *InsertPoint -= IPSecToDo[i].ByteSize;

            *BytesInserted += IPSecToDo[i].ByteSize;

            ESP = (ESPHeader *)(*InsertPoint);

            //
            // Insert ESP Header.
            //
            ESP->SPI = net_long(SA->SPI);
            // BUGBUG: Need to handle cycle when IKE is used.
            ESP->Seq = net_long(++SA->SequenceNum);

            //
            // Compute padding that needs to be added in ESP Trailer.
            // The PadLength and Next header must end of a 4-byte boundary
            // with respect to the start of the IPv6 header.
            // TotalPacketSize is the length of everything in the original
            // packet from the start of the IPv6 header.
            //
            Padding = *TotalPacketSize % 4;

            if (Padding == 0) {
                Padding = 2;
            } else if (Padding == 2) {
                Padding = 0;
            }

            // Adjust total packet size to account for padding.
            *TotalPacketSize += Padding;

            // Where to start the Authentication for this ESP header.
            IPSecToDo[i].Offset = (*InsertPoint) - NewMemory;

            ESPTlrBufSize = Padding + sizeof(ESPTrailer) + Alg->ResultSize;

            *BytesInserted += ESPTlrBufSize;

            //
            // Allocate ESP Trailer.
            //
            ESPTlrMemory = ExAllocatePool(NonPagedPool, ESPTlrBufSize);
            if (ESPTlrMemory == NULL) {
                KdPrint(("InsertTransportIPSec: "
                         "Couldn't allocate ESPTlrMemory!?!\n"));
                Action = LOOKUP_DROP;
                break;
            }

            NdisAllocateBuffer(&Status, &ESPTlrBuffer, IPv6BufferPool,
                               ESPTlrMemory, ESPTlrBufSize);
            if (Status != NDIS_STATUS_SUCCESS) {
                KdPrint(("InsertTransportIPSec: "
                         "Couldn't allocate ESP Trailer buffer!?!\n"));
                ExFreePool(ESPTlrMemory);
                Action = LOOKUP_DROP;
                break;
            }

            // Format Padding.
            for (j = 0; j < Padding; j++) {
                // Add padding.
                *(ESPTlrMemory + j) = j + 1;
            }

            ESPTlr = (ESPTrailer *)(ESPTlrMemory + Padding);

            //
            // Format ESP Trailer.
            //
            ESPTlr->PadLength = j;
            ESPTlr->NextHeader = NextHeader;
            // Set previous header's next header field to ESP.
            NextHeader = IP_PROTOCOL_ESP;

            //
            // Store pointer of where to put ESP Authentication Data.
            //
            IPSecToDo[i].AuthData = ESPTlrMemory + Padding +
                sizeof(ESPTrailer);

            // Set Authentication Data to 0s (MUTABLE).
            RtlZeroMemory(IPSecToDo[i].AuthData, Alg->ResultSize);

            // Link the ESP trailer to the back of the buffer chain.
            NdisChainBufferAtBack(Packet, ESPTlrBuffer);

            // Increment the number of ESP trailers present.
            *NumESPTrailers += 1;

        } // end of else
    } // end of for (i; i < NumHeaders; i++)

    *PrevNextHdr = NextHeader;

    return Action;
}


//* IPsecAdjustMutableFields
//
uint
IPSecAdjustMutableFields(
    uchar *Data,
    IPv6RoutingHeader *SavedRtHdr)
{
    uint i;
    uchar NextHeader;
    IPv6Header *IP;

    //
    // Walk original buffer starting at IP header to end of mutable headers
    // and change the mutable fields when copying to new buffer.
    //

    IP = (IPv6Header *)Data;

    // Only the IP version is immutable.
    IP->VersClassFlow = IP_VERSION;
    // Hop limit is mutable.
    IP->HopLimit = 0;
    NextHeader = IP->NextHeader;

    Data = (uchar *)(IP + 1);

    //
    // Loop through the original headers.  Copy the fields
    // changing the mutable fields.
    //
    for (;;) {
        switch (NextHeader) {

        case IP_PROTOCOL_AH:
        case IP_PROTOCOL_ESP:
            // done.
            return LOOKUP_CONT;

        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS: {
            IPv6OptionsHeader *NewOptHdr;
            uint HdrLen, Amount;
            uchar *Start, *Current;

            NewOptHdr = (IPv6OptionsHeader *)Data;
            HdrLen = (NewOptHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;
            Data += HdrLen;

            //
            // Run through options to see if any are mutable.
            //
            Start = (uchar *)NewOptHdr;
            Current = Start + sizeof(IPv6OptionsHeader);
            HdrLen -= sizeof(IPv6OptionsHeader);
            while (HdrLen) {
                if (*Current == OPT6_PAD_1) {
                    //
                    // This is the special one byte pad option.  Immutable.
                    //
                    Current++;
                    HdrLen--;
                    continue;
                }

                if (OPT6_ISMUTABLE(*Current)) {
                    //
                    // This option's data is mutable.  Everything preceeding
                    // the option data is not.
                    //
                    Current++;  // Now on option data length byte.
                    Amount = *Current;  // Mutable amount.
                    RtlZeroMemory(Current, Amount);

                    HdrLen -= Amount + 2;
                    Current += Amount + 1;
                    Start = Current;

                } else {
                    //
                    // This option's data is not mutable.
                    // Just skip over it.
                    //
                    Current++;  // Now on option data length byte.
                    Amount = *Current;
                    HdrLen -= Amount + 2;
                    Current += Amount + 1;
                }
            }
            if (Start != Current) {
                //
                // Option block ends with an immutable region.
                //
                Amount = Current - Start;
#ifdef IPSEC_DEBUG
                DbgPrint("DestOpts: \n");
                dump_encoded_mesg(Start, Amount);
#endif
            }

            NextHeader = NewOptHdr->NextHeader;

            break;
        }
        case IP_PROTOCOL_ROUTING: {
            IPv6RoutingHeader *NewRtHdr;
            IPv6Addr *RecvRtAddr, *SendRtAddr;

            // Verify there is a SavedRtHdr.
            if (SavedRtHdr == NULL) {
                KdPrint(("IPSecAdjustMutableFields: No Saved routing header"));
                return LOOKUP_DROP;
            }

            // Point to the saved first routing address.
            SendRtAddr = (IPv6Addr *)(SavedRtHdr + 1);

            // New buffer routing header.
            NewRtHdr = (IPv6RoutingHeader UNALIGNED *)Data;
            // Point to the first routing address.
            RecvRtAddr = (IPv6Addr *)(NewRtHdr + 1);

            NewRtHdr->SegmentsLeft = 0;

            // Copy the IP dest addr to first routing address.
            RtlCopyMemory(RecvRtAddr, &IP->Dest, sizeof(IPv6Addr));

            for (i = 0; i < (uint)(SavedRtHdr->SegmentsLeft - 1); i++) {
                //
                // Copy the routing addresses as they will look
                // at receive.
                //
                RtlCopyMemory(&RecvRtAddr[i+1], &SendRtAddr[i],
                              sizeof(IPv6Addr));
            }

            // Copy the last routing address to the IP dest address.
            RtlCopyMemory(&IP->Dest, &SendRtAddr[i], sizeof(IPv6Addr));

            Data += (NewRtHdr->HeaderExtLength + 1) * 8;
            NextHeader = NewRtHdr->NextHeader;
            break;
        }
        default:
            KdPrint(("IPSecAdjustMutableFields: Don't support header %d\n",
                     NextHeader));
            return LOOKUP_DROP;
        }// end of switch(NextHeader);
    } // end of for (;;)

    return LOOKUP_CONT;
}


//* IPsecAuthenticatePacket
//
void
IPSecAuthenticatePacket(
    uint Mode,
    IPSecProc *IPSecToDo,
    uchar *InsertPoint,
    uint *TunnelStart,
    uchar *NewMemory,
    uchar *EndNewMemory,
    PNDIS_BUFFER NewBuffer1)
{
    uchar *Data;
    uint i, NumHeaders = 0, DataLen, j;
    void *Context;
    void *VirtualAddr;
    PNDIS_BUFFER NextBuffer;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;

    if (Mode == TRANSPORT) {
        i = 0;
        if (*TunnelStart != NO_TUNNEL) {
            NumHeaders = *TunnelStart;
        } else {
            NumHeaders = IPSecToDo->BundleSize;
        }
    } else {
        // Tunnel.
        i = *TunnelStart;
        // Get the end of the tunnels.
        for (j = *TunnelStart + 1; j < IPSecToDo->BundleSize; j++) {
            if (IPSecToDo[j].Mode == TUNNEL) {
                // Another tunnel.
                NumHeaders = j;
                break;
            }
        }
        if (NumHeaders == 0) {
            // No other tunnels.
            NumHeaders = IPSecToDo->BundleSize;
        }

        // Set TunnelStart for loop in IPv6Send2().
        *TunnelStart = NumHeaders;
    }

    for (i; i < NumHeaders; i++) {
        SA = IPSecToDo[i].SA;
        Alg = &AlgorithmTable[SA->AlgorithmId];

        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            uint FirstIPSecHeader = NumHeaders - 1;

            // AH starts at the IP header.
            Data = InsertPoint;

            //
            // Check if there are other IPSec headers after this in the 
            // same IP area (IP_"after"<IP Area>_Data).  Other IPSec headers
            // need to be ignored in the authentication calculation.
            // NOTE: This is not a required IPSec header combination.
            //
            if (i < FirstIPSecHeader) {
                uchar *StopPoint;
                uint n;
                ESPTrailer *ESPT;

                n = i + 1;

                //
                // There are some other IPSec headers.
                // Need to authenticate from the IP header to
                // the last header before the first IPSec header hit.
                // Then if there is no ESP, just authenticate from the
                // current IPSec header to the end of the packet.
                // If there is ESP, need to ignore the ESP trailers.
                //

                //
                // Calculate start of first IPSec header.
                //
                if (IPSecToDo[FirstIPSecHeader].SA->IPSecProto ==
                    IP_PROTOCOL_AH) {
                    StopPoint = (IPSecToDo[FirstIPSecHeader].AuthData -
                                 sizeof(AHHeader));
                } else {
                    StopPoint = NewMemory + IPSecToDo[FirstIPSecHeader].Offset;
                }

                // Length from IP to first IPSec header.
                DataLen = StopPoint - Data;

                // Initialize Context.
                Context = alloca(Alg->ContextSize);
                (*Alg->Initialize)(Context, SA->Key);

                // Authentication to the first IPSec header.
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                // Set the data start to the current IPSec header.
                Data = IPSecToDo[i].AuthData - sizeof(AHHeader);
                DataLen = EndNewMemory - Data;

                //
                // Authenticate from current IPSec header to the
                // end of the new allocated buffer.
                //
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                //
                // Need to authenticate other buffers if there are any.
                // Ignore the ESP trailers.
                //

                // Check for an ESP header closest to the current IPSec header.
                while (n < NumHeaders) {
                    if (IPSecToDo[n].SA->IPSecProto == IP_PROTOCOL_ESP) {
                        break;
                    }
                    n++;
                }

                // Get the next buffer in the packet.
                NdisGetNextBuffer(NewBuffer1, &NextBuffer);

                while (NextBuffer != NULL) {
                    // Get the start of the data and the data length.
                    NdisQueryBuffer(NextBuffer, &VirtualAddr, &DataLen);
                    Data = (uchar *)VirtualAddr;

                    //
                    // Check if this is the ESP Trailer by seeing if the
                    // AuthData storage is in the buffer.
                    //
                    if (n < NumHeaders)
                    if (IPSecToDo[n].AuthData > Data &&
                        IPSecToDo[n].AuthData < (Data + DataLen)) {

                        // Stop here this is the ESP trailer.
                        break;
                    }

                    // Feed the buffer to the Authentication function.
                    (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                    // Get the next buffer in the packet.
                    NdisGetNextBuffer(NextBuffer, &NextBuffer)
                } // end of while (NextBuffer != NULL)

                // Get the Authentication Data.
                (*Alg->Finalize)(Context, SA->Key, IPSecToDo[i].AuthData);

                // Resume loop for other IPSec headers.
                continue;
            }
        } else { // ESP.
            // ESP starts the authentication at the ESP header.
            Data = NewMemory + IPSecToDo[i].Offset;
        }

        DataLen = EndNewMemory - Data;

        // Initialize Context.
        Context = alloca(Alg->ContextSize);
        (*Alg->Initialize)(Context, SA->Key);

#ifdef IPSEC_DEBUG
        DbgPrint("\nSend Data[%d]:\n", i);
        dump_encoded_mesg(Data, DataLen);
#endif

        // Feed the new buffer to the Authentication function.
        (*Alg->Operate)(Context, SA->Key, Data, DataLen);

        // Get the next buffer in the packet.
        NdisGetNextBuffer(NewBuffer1, &NextBuffer);

        while (NextBuffer != NULL) {
            // Get the start of the data and the data length.
            NdisQueryBuffer(NextBuffer, &VirtualAddr, &DataLen);
            Data = (uchar *)VirtualAddr;

            //
            // Check if this is the ESP Trailer by seeing if the
            // AuthData storage is in the buffer.
            //
            if (SA->IPSecProto == IP_PROTOCOL_ESP &&
                IPSecToDo[i].AuthData > Data &&
                IPSecToDo[i].AuthData < (Data + DataLen)) {
                // Don't include the Authentication Data
                // in the ICV calculation.
                DataLen = IPSecToDo[i].AuthData - Data;
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Data, DataLen);
#endif
                // Feed the buffer to the Authentication function.
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);
                break;
            }
#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Data, DataLen);
#endif
            // Feed the buffer to the Authentication function.
            (*Alg->Operate)(Context, SA->Key, Data, DataLen);

            // Get the next buffer in the packet.
            NdisGetNextBuffer(NextBuffer, &NextBuffer)
        } // end of while (NextBuffer != NULL)

        // Get the Authentication Data.
        (*Alg->Finalize)(Context, SA->Key, IPSecToDo[i].AuthData);

#ifdef IPSEC_DEBUG
        DbgPrint("Send Key: ");
        DumpKey(SA->RawKey, SA->RawKeyLength);
        DbgPrint("Send AuthData:\n");
        dump_encoded_mesg(IPSecToDo[i].AuthData, Alg->ResultSize);
#endif
    } // end of for (i = 0; ...)
}
