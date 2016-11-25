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
// This file contains the code for dealing with TDI Query/Set
// information calls.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "info.h"
#include "tdiinfo.h"
#include "tcpcfg.h"
#include "udp.h"
#include "tcpsend.h"

#ifndef UDP_ONLY
#define MY_SERVICE_FLAGS (TDI_SERVICE_CONNECTION_MODE     | \
                          TDI_SERVICE_ORDERLY_RELEASE     | \
                          TDI_SERVICE_CONNECTIONLESS_MODE | \
                          TDI_SERVICE_ERROR_FREE_DELIVERY | \
                          TDI_SERVICE_BROADCAST_SUPPORTED | \
                          TDI_SERVICE_DELAYED_ACCEPTANCE  | \
                          TDI_SERVICE_EXPEDITED_DATA      | \
                          TDI_SERVICE_NO_ZERO_LENGTH)
#else
#define MY_SERVICE_FLAGS (TDI_SERVICE_CONNECTIONLESS_MODE | \
                          TDI_SERVICE_BROADCAST_SUPPORTED)
#endif

extern LARGE_INTEGER StartTime;
extern KSPIN_LOCK AddrObjTableLock;

#ifndef UDP_ONLY
TCPStats TStats;
#endif

UDPStats UStats;

struct ReadTableStruct {
    uint (*rts_validate)(void *Context, uint *Valid);
    uint (*rts_readnext)(void *Context, void *OutBuf);
};

struct ReadTableStruct ReadAOTable = {ValidateAOContext, ReadNextAO};

#ifndef UDP_ONLY

struct ReadTableStruct ReadTCBTable = {ValidateTCBContext, ReadNextTCB};

extern KSPIN_LOCK TCBTableLock;
#endif

extern KSPIN_LOCK AddrObjTableLock;

struct TDIEntityID *EntityList;
uint EntityCount;

//* TdiQueryInformation - Query Information handler.
//
//  The TDI QueryInformation routine.  Called when the client wants to
//  query information on a connection, the provider as a whole, or to
//  get statistics.
//
TDI_STATUS  // Returns: Status of attempt to query information.
TdiQueryInformation(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint QueryType,        // Type of query to be performed.
    PNDIS_BUFFER Buffer,   // Buffer to place data info.
    uint *BufferSize,      // Pointer to size in bytes of buffer.
                           // On return, filled in with number of bytes copied.
    uint IsConn)           // Valid only for TDI_QUERY_ADDRESS_INFO.  TRUE if
                           // we are querying the address info on a connection.
{
    union {
        TDI_CONNECTION_INFO ConnInfo;
        TDI_ADDRESS_INFO AddrInfo;
        TDI_PROVIDER_INFO ProviderInfo;
        TDI_PROVIDER_STATISTICS ProviderStats;
    } InfoBuf;

    uint InfoSize;
    KIRQL Irql0, Irql1, Irql2;  // One per lock nesting level.
#ifndef UDP_ONLY
    TCPConn *Conn;
    TCB *InfoTCB;
#endif
    AddrObj *InfoAO;
    void *InfoPtr = NULL;
    uint Offset;
    uint Size;
    uint BytesCopied;

    switch (QueryType) {
 
    case TDI_QUERY_BROADCAST_ADDRESS:
        return TDI_INVALID_QUERY;
        break;

    case TDI_QUERY_PROVIDER_INFO:
        InfoBuf.ProviderInfo.Version = 0x100;
#ifndef UDP_ONLY
        InfoBuf.ProviderInfo.MaxSendSize = 0xffffffff;
#else
        InfoBuf.ProviderInfo.MaxSendSize = 0;
#endif
        InfoBuf.ProviderInfo.MaxConnectionUserData = 0;
        InfoBuf.ProviderInfo.MaxDatagramSize = 0xffff - sizeof(UDPHeader);
        InfoBuf.ProviderInfo.ServiceFlags = MY_SERVICE_FLAGS;
        InfoBuf.ProviderInfo.MinimumLookaheadData = 1;
        InfoBuf.ProviderInfo.MaximumLookaheadData = 0xffff;
        InfoBuf.ProviderInfo.NumberOfResources = 0;
        InfoBuf.ProviderInfo.StartTime = StartTime;
        InfoSize = sizeof(TDI_PROVIDER_INFO);
        InfoPtr = &InfoBuf.ProviderInfo;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        InfoSize = sizeof(TDI_ADDRESS_INFO) - sizeof(TRANSPORT_ADDRESS) +
            TCP_TA_SIZE;
        RtlZeroMemory(&InfoBuf.AddrInfo, TCP_TA_SIZE);
        //
        // Since noone knows what this means, we'll set it to one.
        //
        InfoBuf.AddrInfo.ActivityCount = 1;

        if (IsConn) {
#ifdef UDP_ONLY
            return TDI_INVALID_QUERY;
#else

            KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
            KeAcquireSpinLock(&ConnTableLock, &Irql1);
            Conn = GetConnFromConnID((uint)Request->Handle.ConnectionContext);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                InfoTCB = Conn->tc_tcb;
                // If we have a TCB we'll return information about that TCB.
                // Otherwise we'll return info about the address object.
                if (InfoTCB != NULL) {
                    CHECK_STRUCT(InfoTCB, tcb);
                    KeAcquireSpinLock(&InfoTCB->tcb_lock, &Irql2);
                    KeReleaseSpinLock(&ConnTableLock, Irql2);
                    KeReleaseSpinLock(&AddrObjTableLock, Irql1);
                    BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                                    &InfoTCB->tcb_saddr,
                                    InfoTCB->tcb_sscope_id,
                                    InfoTCB->tcb_sport);
                    KeReleaseSpinLock(&InfoTCB->tcb_lock, Irql0);
                    InfoPtr = &InfoBuf.AddrInfo;
                    break;
                } else {
                    // No TCB, return info on the AddrObj.
                    InfoAO = Conn->tc_ao;
                    if (InfoAO != NULL) {
                        // We have an AddrObj.
                        CHECK_STRUCT(InfoAO, ao);
                        KeAcquireSpinLock(&InfoAO->ao_lock, &Irql2);
                        BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                                        &InfoAO->ao_addr,
                                        InfoAO->ao_scope_id,
                                        InfoAO->ao_port);
                        KeReleaseSpinLock(&InfoAO->ao_lock, Irql2);
                        KeReleaseSpinLock(&ConnTableLock, Irql1);
                        KeReleaseSpinLock(&AddrObjTableLock, Irql0);
                        InfoPtr = &InfoBuf.AddrInfo;
                        break;
                    }
                }
            }

            //
            // Fall through to here when we can't find the connection, or
            // the connection isn't associated.
            //
            KeReleaseSpinLock(&ConnTableLock, Irql1);
            KeReleaseSpinLock(&AddrObjTableLock, Irql0);
            return TDI_INVALID_CONNECTION;
            break;

#endif
        } else {
            // Asking for information on an addr. object.
            InfoAO = Request->Handle.AddressHandle;
            CHECK_STRUCT(InfoAO, ao);

            KeAcquireSpinLock(&InfoAO->ao_lock, &Irql0);

            if (!AO_VALID(InfoAO)) {
                KeReleaseSpinLock(&InfoAO->ao_lock, Irql0);
                return TDI_ADDR_INVALID;
            }

            BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                            &InfoAO->ao_addr, InfoAO->ao_scope_id,
                            InfoAO->ao_port);
            KeReleaseSpinLock(&InfoAO->ao_lock, Irql0);
            InfoPtr = &InfoBuf.AddrInfo;
            break;
        }

        break;

    case TDI_QUERY_CONNECTION_INFO:
#ifndef UDP_ONLY
        InfoSize = sizeof(TDI_CONNECTION_INFO);
        KeAcquireSpinLock(&ConnTableLock, &Irql0);
        Conn = GetConnFromConnID((uint)Request->Handle.ConnectionContext);

        if (Conn != NULL) {
            CHECK_STRUCT(Conn, tc);

            InfoTCB = Conn->tc_tcb;
            // If we have a TCB we'll return the information.
            // Otherwise we'll error out.
            if (InfoTCB != NULL) {
                ulong TotalTime;
                ulong BPS, PathBPS;
                IP_STATUS IPStatus;
                ULARGE_INTEGER TempULargeInt;

                CHECK_STRUCT(InfoTCB, tcb);
                KeAcquireSpinLock(&InfoTCB->tcb_lock, &Irql1);
                KeReleaseSpinLock(&ConnTableLock, Irql1);
                RtlZeroMemory(&InfoBuf.ConnInfo, sizeof(TDI_CONNECTION_INFO));
                InfoBuf.ConnInfo.State = (ulong)InfoTCB->tcb_state;

                // BUGBUG: IPv4 code called down into IP here to get PathBPS
                // BUGBUG: for InfoTCB's saddr, daddr pair.
#if 1                
                IPStatus = IP_BAD_REQ;
                if (1) {
#else
                if (IPStatus != IP_SUCCESS) {
#endif
                    InfoBuf.ConnInfo.Throughput.LowPart = 0xFFFFFFFF;
                    InfoBuf.ConnInfo.Throughput.HighPart = 0xFFFFFFFF;
                } else {
                    InfoBuf.ConnInfo.Throughput.HighPart = 0;
                    TotalTime = InfoTCB->tcb_totaltime / (1000 / MS_PER_TICK);
                    if (TotalTime != 0) {
                        TempULargeInt.LowPart = InfoTCB->tcb_bcountlow;
                        TempULargeInt.HighPart = InfoTCB->tcb_bcounthi;

                        BPS = (ulong)(TempULargeInt.QuadPart / TotalTime);
                        InfoBuf.ConnInfo.Throughput.LowPart =
                            MIN(BPS, PathBPS);
                    } else
                        InfoBuf.ConnInfo.Throughput.LowPart = PathBPS;
                }

                // To figure the delay we use the rexmit timeout.  Our
                // rexmit timeout is roughly the round trip time plus
                // some slop, so we use half of that as the one way delay.
                InfoBuf.ConnInfo.Delay.LowPart =
                    (REXMIT_TO(InfoTCB) * MS_PER_TICK) / 2;
                InfoBuf.ConnInfo.Delay.HighPart = 0;
                //
                // Convert milliseconds to 100ns and negate for relative
                // time.
                //
                InfoBuf.ConnInfo.Delay = RtlExtendedIntegerMultiply(
                    InfoBuf.ConnInfo.Delay, 10000);

                ASSERT(InfoBuf.ConnInfo.Delay.HighPart == 0);

                InfoBuf.ConnInfo.Delay.QuadPart =
                    -InfoBuf.ConnInfo.Delay.QuadPart;

                KeReleaseSpinLock(&InfoTCB->tcb_lock, Irql0);
                InfoPtr = &InfoBuf.ConnInfo;
                break;
            }

        }

        //
        // Come through here if we can't find the connection
        // or it has no TCB.
        //
        KeReleaseSpinLock(&ConnTableLock, Irql0);
        return TDI_INVALID_CONNECTION;
        break;

#else // UDP_ONLY
        return TDI_INVALID_QUERY;
        break;
#endif // UDP_ONLY
    case TDI_QUERY_PROVIDER_STATISTICS:
        RtlZeroMemory(&InfoBuf.ProviderStats, sizeof(TDI_PROVIDER_STATISTICS));
        InfoBuf.ProviderStats.Version = 0x100;
        InfoSize = sizeof(TDI_PROVIDER_STATISTICS);
        InfoPtr = &InfoBuf.ProviderStats;
        break;
    default:
        return TDI_INVALID_QUERY;
        break;
    }

    // When we get here, we've got the pointers set up and the information
    // filled in.

    ASSERT(InfoPtr != NULL);
    Offset = 0;
    Size = *BufferSize;
    (void)CopyFlatToNdis(Buffer, InfoPtr, MIN(InfoSize, Size), &Offset,
                         &BytesCopied);
    if (Size < InfoSize)
        return TDI_BUFFER_OVERFLOW;
    else {
        *BufferSize = InfoSize;
        return TDI_SUCCESS;
    }
}

//* TdiSetInformation - Set Information handler.
//
//  The TDI SetInformation routine.  Currently we don't allow anything to be
//  set.
//
TDI_STATUS  // Returns: Status of attempt to set information.
TdiSetInformation(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint SetType,          // Type of set to be performed.
    PNDIS_BUFFER Buffer,   // Buffer to set from.
    uint BufferSize,       // Size in bytes of buffer.
    uint IsConn)           // Valid only for TDI_QUERY_ADDRESS_INFO. TRUE if
                           // we are setting the address info on a connection.
{
    return TDI_INVALID_REQUEST;
}

//* TdiAction - Action handler.
//
//  The TDI Action routine.  Currently we don't support any actions.
//
TDI_STATUS  // Returns: Status of attempt to perform action.
TdiAction(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint ActionType,       // Type of action to be performed.
    PNDIS_BUFFER Buffer,   // Buffer of action info.
    uint BufferSize)       // Size in bytes of buffer.
{
    return TDI_INVALID_REQUEST;
}

//* TdiQueryInformationEx - Extended TDI query information.
//
//  This is the new TDI query information handler.  We take in a TDIObjectID
//  structure, a buffer and length, and some context information, and return
//  the requested information if possible.
//
TDI_STATUS  // Returns: Status of attempt to get information.
TdiQueryInformationEx(
    PTDI_REQUEST Request,  // Request structure for this command.
    TDIObjectID *ID,       // Object ID.
    PNDIS_BUFFER Buffer,   // Buffer to be filled in.
    uint *Size,            // Pointer to size in bytes of Buffer.
                           // On return, filled with number of bytes written.
    void *Context)         // Context buffer.
{
    uint BufferSize = *Size;
    uint InfoSize;
    void *InfoPtr;
    uint Fixed;
    KIRQL Irql0;
    KSPIN_LOCK *LockPtr;
    uint Offset = 0;
    uchar InfoBuffer[sizeof(TCPConnTableEntry)];
    uint BytesRead;
    uint Valid;
    uint Entity;
    uint BytesCopied;

    // First check to see if he's querying for list of entities.
    Entity = ID->toi_entity.tei_entity;
    if (Entity == GENERIC_ENTITY) {
        *Size = 0;

        if (ID->toi_class  != INFO_CLASS_GENERIC ||
            ID->toi_type != INFO_TYPE_PROVIDER ||
            ID->toi_id != ENTITY_LIST_ID) {
            return TDI_INVALID_PARAMETER;
        }

        // Make sure we have room for it the list in the buffer.
        InfoSize = EntityCount * sizeof(TDIEntityID);

        if (BufferSize < InfoSize) {
            // Not enough room.
            return TDI_BUFFER_TOO_SMALL;
        }

        *Size = InfoSize;

        // Copy it in, free our temp. buffer, and return success.
        (void)CopyFlatToNdis(Buffer, (uchar *)EntityList, InfoSize, &Offset,
                             &BytesCopied);
        return TDI_SUCCESS;
    }

    //* Check the level.  If it can't be for us, pass it down.
#ifndef UDP_ONLY
    if (Entity != CO_TL_ENTITY &&  Entity != CL_TL_ENTITY) {
#else
    if (Entity != CL_TL_ENTITY) {
#endif
        // When we support multiple lower entities at this layer we'll have
        // to figure out which one to dispatch to. For now, just pass it
        // straight down.

        // BUGBUG: IPv4 code passed query down to IP here.
        return TDI_SUCCESS;
    }

    if (ID->toi_entity.tei_instance != TL_INSTANCE) {
        // We only support a single instance.
        return TDI_INVALID_REQUEST;
    }

    // Zero returned parameters in case of an error below.
    *Size = 0;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // This is a generic request.
        if (ID->toi_type == INFO_TYPE_PROVIDER &&
            ID->toi_id == ENTITY_TYPE_ID) {
            if (BufferSize >= sizeof(uint)) {
                *(uint *)&InfoBuffer[0] = (Entity == CO_TL_ENTITY) ? CO_TL_TCP
                    : CL_TL_UDP;
                (void)CopyFlatToNdis(Buffer, InfoBuffer, sizeof(uint), &Offset,
                                     &BytesCopied);
                return TDI_SUCCESS;
            } else
                return TDI_BUFFER_TOO_SMALL;
        }
        return TDI_INVALID_PARAMETER;
    }

    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        // Handle protocol specific class of information. For us, this is
        // the MIB-2 stuff or the minimal stuff we do for oob_inline support.

#ifndef UDP_ONLY
        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPConn *Conn;
            TCB *QueryTCB;
            TCPSocketAMInfo *AMInfo;
            KIRQL Irql1;

            if (BufferSize < sizeof(TCPSocketAMInfo) ||
                ID->toi_id != TCP_SOCKET_ATMARK)
                return TDI_INVALID_PARAMETER;

            AMInfo = (TCPSocketAMInfo *)InfoBuffer;
            // BUGBUG: The following lock is never released!
            KeAcquireSpinLock(&ConnTableLock, &Irql0);

            Conn = GetConnFromConnID((uint)Request->Handle.ConnectionContext);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                QueryTCB = Conn->tc_tcb;
                if (QueryTCB != NULL) {
                    CHECK_STRUCT(QueryTCB, tcb);
                    KeAcquireSpinLock(&QueryTCB->tcb_lock, &Irql1);
                    if ((QueryTCB->tcb_flags & (URG_INLINE | URG_VALID)) ==
                        (URG_INLINE | URG_VALID)) {
                        // We're in inline mode, and the urgent data fields are
                        // valid.
                        AMInfo->tsa_size = QueryTCB->tcb_urgend -
                            QueryTCB->tcb_urgstart + 1;
                        // Rcvnext - pendingcnt is the sequence number of the
                        // next byte of data that will be delivered to the
                        // client. Urgend - that value is the offset in the
                        // data stream of the end of urgent data.
                        AMInfo->tsa_offset = QueryTCB->tcb_urgend -
                            (QueryTCB->tcb_rcvnext - QueryTCB->tcb_pendingcnt);
                    } else {
                        AMInfo->tsa_size = 0;
                        AMInfo->tsa_offset = 0;
                    }
                    KeReleaseSpinLock(&QueryTCB->tcb_lock, Irql1);
                    *Size = sizeof(TCPSocketAMInfo);
                    CopyFlatToNdis(Buffer, InfoBuffer, sizeof(TCPSocketAMInfo),
                                   &Offset, &BytesCopied);
                    return TDI_SUCCESS;
                }
            }
            return TDI_INVALID_PARAMETER;
        }

#endif
        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

        switch (ID->toi_id) {

        case UDP_MIB_STAT_ID:
#if UDP_MIB_STAT_ID != TCP_MIB_STAT_ID
        case TCP_MIB_STAT_ID:
#endif
            Fixed = TRUE;
            if (Entity == CL_TL_ENTITY) {
                InfoSize = sizeof(UDPStats);
                InfoPtr = &UStats;
            } else {
#ifndef UDP_ONLY
                InfoSize = sizeof(TCPStats);
                InfoPtr = &TStats;
#else
                return TDI_INVALID_PARAMETER;
#endif
            }
            break;
        case UDP_MIB_TABLE_ID:
#if UDP_MIB_TABLE_ID != TCP_MIB_TABLE_ID
        case TCP_MIB_TABLE_ID:
#endif
            Fixed = FALSE;
            if (Entity == CL_TL_ENTITY) {
                InfoSize = sizeof(UDPEntry);
                InfoPtr = &ReadAOTable;
                KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
                LockPtr = &AddrObjTableLock;
            } else {
#ifndef UDP_ONLY
                InfoSize = sizeof(TCPConnTableEntry);
                InfoPtr = &ReadTCBTable;
                KeAcquireSpinLock(&TCBTableLock, &Irql0);
                LockPtr = &TCBTableLock;
#else
                return TDI_INVALID_PARAMETER;
#endif
            }
            break;
        default:
            return TDI_INVALID_PARAMETER;
            break;
        }

        if (Fixed) {
            if (BufferSize < InfoSize)
                return TDI_BUFFER_TOO_SMALL;

            *Size = InfoSize;

            (void)CopyFlatToNdis(Buffer, InfoPtr, InfoSize, &Offset,
                                 &BytesCopied);
            return TDI_SUCCESS;
        } else {
            struct ReadTableStruct *RTSPtr;
            uint ReadStatus;

            // Have a variable length (or mult-instance) structure to copy.
            // InfoPtr points to the structure describing the routines to
            // call to read the table.
            // Loop through up to CountWanted times, calling the routine
            // each time.
            BytesRead = 0;

            RTSPtr = InfoPtr;

            ReadStatus = (*(RTSPtr->rts_validate))(Context, &Valid);

            // If we successfully read something we'll continue. Otherwise
            // we'll bail out.
            if (!Valid) {
                KeReleaseSpinLock(LockPtr, Irql0);
                return TDI_INVALID_PARAMETER;
            }

            while (ReadStatus)  {
                // The invariant here is that there is data in the table to
                // read. We may or may not have room for it. So ReadStatus
                // is TRUE, and BufferSize - BytesRead is the room left
                // in the buffer.
                if ((int)(BufferSize - BytesRead) >= (int)InfoSize) {
                    ReadStatus = (*(RTSPtr->rts_readnext))(Context,
                                                           InfoBuffer);
                    BytesRead += InfoSize;
                    Buffer = CopyFlatToNdis(Buffer, InfoBuffer, InfoSize,
                                            &Offset, &BytesCopied);
                } else
                    break;
            }

            *Size = BytesRead;
            KeReleaseSpinLock(LockPtr, Irql0);
            return (!ReadStatus ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
        }
    }

    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info.  For now, error out.
        return TDI_INVALID_PARAMETER;
    }

    return TDI_INVALID_PARAMETER;
}

//* TdiSetInfoEx - Extended TDI set information.
//
//  This is the new TDI set information handler.  We take in a TDIObjectID
//  structure, a buffer and length.  We set the object specifed by the ID
//  (and possibly by the Request) to the value specified in the buffer.
//
TDI_STATUS  // Returns: Status of attempt to get information.
TdiSetInformationEx(
    PTDI_REQUEST Request,  // Request structure for this command.
    TDIObjectID *ID,       // Object ID.
    void *Buffer,          // Buffer containing value to set.
    uint Size)             // Size in bytes of Buffer.
{
    TCPConnTableEntry *TCPEntry;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
#ifndef UDP_ONLY
    TCB *SetTCB;
    TCPConn *Conn;
#endif
    uint Entity;
    TDI_STATUS Status;

    // Check the level.  If it can't be for us, pass it down.
    Entity = ID->toi_entity.tei_entity;

    if (Entity != CO_TL_ENTITY && Entity != CL_TL_ENTITY) {
        // Someday we'll have to figure out how to dispatch.
        // For now, just pass it down.

        // BUGBUG: IPv4 code passed the set info request down to IP here.
        return TDI_SUCCESS;
    }

    if (ID->toi_entity.tei_instance != TL_INSTANCE)
        return TDI_INVALID_REQUEST;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // Fill this in when we have generic class defines.
        return TDI_INVALID_PARAMETER;
    }

    // Now look at the rest of it.
    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        // Handle protocol specific class of information.  For us, this is
        // the MIB-2 stuff, as well as common sockets options,
        // and in particular the setting of the state of a TCP connection.

        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPSocketOption *Option;
            uint Flag;
            uint Value;

#ifndef UDP_ONLY
            // A connection type.  Get the connection, and then figure out
            // what to do with it.
            Status = TDI_INVALID_PARAMETER;

            if (Size < sizeof(TCPSocketOption))
                return Status;

            KeAcquireSpinLock(&ConnTableLock, &Irql0);

            Conn = GetConnFromConnID((uint)Request->Handle.ConnectionContext);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                Status = TDI_SUCCESS;

                if (ID->toi_id == TCP_SOCKET_WINDOW) {
                    // This is a funny option, because it doesn't involve
                    // flags.  Handle this specially.
                    Option = (TCPSocketOption *)Buffer;

                    // We don't allow anyone to shrink the window, as this
                    // gets too weird from a protocol point of view.  Also,
                    // make sure they don't try and set anything too big.
                    if (Option->tso_value > 0xffff)
                        Status = TDI_INVALID_PARAMETER;
                    else if (Option->tso_value > Conn->tc_window ||
                             Conn->tc_tcb == NULL) {
                        Conn->tc_flags |= CONN_WINSET;
                        Conn->tc_window = Option->tso_value;
                        SetTCB = Conn->tc_tcb;

                        if (SetTCB != NULL) {
                            CHECK_STRUCT(SetTCB, tcb);
                            KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                            ASSERT(Option->tso_value > SetTCB->tcb_defaultwin);
                            if (DATA_RCV_STATE(SetTCB->tcb_state) &&
                                !CLOSING(SetTCB)) {
                                SetTCB->tcb_flags |= WINDOW_SET;
                                SetTCB->tcb_defaultwin = Option->tso_value;
                                SetTCB->tcb_refcnt++;
                                KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                                SendACK(SetTCB);
                                KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                                DerefTCB(SetTCB, Irql1);
                            } else {
                                KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                            }
                        }
                    }
                    KeReleaseSpinLock(&ConnTableLock, Irql0);
                    return Status;
                }

                Flag = 0;
                Option = (TCPSocketOption *)Buffer;
                Value = Option->tso_value;
                // We have the connection, so figure out which flag to set.
                switch (ID->toi_id) {

                case TCP_SOCKET_NODELAY:
                    Value = !Value;
                    Flag = NAGLING;
                    break;
                case TCP_SOCKET_KEEPALIVE:
                    Flag = KEEPALIVE;
                    break;
                case TCP_SOCKET_BSDURGENT:
                    Flag = BSD_URGENT;
                    break;
                case TCP_SOCKET_OOBINLINE:
                    Flag = URG_INLINE;
                    break;
                default:
                    Status = TDI_INVALID_PARAMETER;
                    break;
                }

                if (Status == TDI_SUCCESS) {
                    if (Value)
                        Conn->tc_tcbflags |= Flag;
                    else
                        Conn->tc_tcbflags &= ~Flag;

                    SetTCB = Conn->tc_tcb;
                    if (SetTCB != NULL) {
                        CHECK_STRUCT(SetTCB, tcb);
                        KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                        if (Value)
                            SetTCB->tcb_flags |= Flag;
                        else
                            SetTCB->tcb_flags &= ~Flag;

                        if (ID->toi_id == TCP_SOCKET_KEEPALIVE) {
                            SetTCB->tcb_alive = TCPTime;
                            SetTCB->tcb_kacount = 0;
                        }

                        KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                    }
                }
            }

            KeReleaseSpinLock(&ConnTableLock, Irql0);
            return Status;
#else
            return TDI_INVALID_PARAMETER;
#endif
        }

        if (ID->toi_type == INFO_TYPE_ADDRESS_OBJECT) {
            // We're setting information on an address object.  This is
            // pretty simple.

            return SetAddrOptions(Request, ID->toi_id, Size, Buffer);
        }

        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

#ifndef UDP_ONLY
        if (ID->toi_id == TCP_MIB_TABLE_ID) {
            if (Size != sizeof(TCPConnTableEntry))
                return TDI_INVALID_PARAMETER;

            TCPEntry = (TCPConnTableEntry *)Buffer;

            if (TCPEntry->tct_state != TCP_DELETE_TCB)
                return TDI_INVALID_PARAMETER;

            // We have an apparently valid request.  Look up the TCB.
            KeAcquireSpinLock(&TCBTableLock, &Irql0);
            SetTCB = FindTCB(&TCPEntry->tct_localaddr,
                             &TCPEntry->tct_remoteaddr,
                             TCPEntry->tct_localscopeid,
                             TCPEntry->tct_remotescopeid,
                             (ushort)TCPEntry->tct_localport,
                             (ushort)TCPEntry->tct_remoteport);

            // We found him.  If he's not closing or closed, close him.
            if (SetTCB != NULL) {
                KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                KeReleaseSpinLock(&TCBTableLock, Irql1);

                // We've got him.  Bump his ref. count, and call TryToCloseTCB
                // to mark him as closing. Then notify the upper layer client
                // of the disconnect.
                SetTCB->tcb_refcnt++;
                if (SetTCB->tcb_state != TCB_CLOSED && !CLOSING(SetTCB)) {
                    SetTCB->tcb_flags |= NEED_RST;
                    TryToCloseTCB(SetTCB, TCB_CLOSE_ABORTED, Irql0);
                    KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql0);

                    if (SetTCB->tcb_state != TCB_TIME_WAIT) {
                        // Remove him from the TCB, and notify the client.
                        KeReleaseSpinLock(&SetTCB->tcb_lock, Irql0);
                        RemoveTCBFromConn(SetTCB);
                        NotifyOfDisc(SetTCB, TDI_CONNECTION_RESET);
                        KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql0);
                    }
                }

                DerefTCB(SetTCB, Irql0);
                return TDI_SUCCESS;
            } else {
                KeReleaseSpinLock(&TCBTableLock, Irql0);
                return TDI_INVALID_PARAMETER;
            }
        } else
            return TDI_INVALID_PARAMETER;
#else
        return TDI_INVALID_PARAMETER;
#endif

    }

    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info.  For now, error out.
        return TDI_INVALID_REQUEST;
    }

    return TDI_INVALID_REQUEST;
}
