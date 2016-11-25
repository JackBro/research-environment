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
// LAN driver definitions.
//


#ifndef LAN_INCLUDED
#define LAN_INCLUDED 1

#define INTERFACE_UP    0                   // Interface is up.
#define INTERFACE_INIT  1                   // Interface is initializing.
#define INTERFACE_DOWN  2                   // Interface is down.

#define LOOKAHEAD_SIZE  128                 // A reasonable lookahead size.

#define IEEE_802_ADDR_LENGTH 6              // Length of an IEEE 802 address.


//
// Structure of an Ethernet header.
//
typedef struct EtherHeader {
    uchar eh_daddr[IEEE_802_ADDR_LENGTH];
    uchar eh_saddr[IEEE_802_ADDR_LENGTH];
    ushort eh_type;
} EtherHeader;

//
// Structure of a token ring header.
//
typedef struct TRHeader {
    uchar tr_ac;
    uchar tr_fc;
    uchar tr_daddr[IEEE_802_ADDR_LENGTH];
    uchar tr_saddr[IEEE_802_ADDR_LENGTH];
} TRHeader;
#define ARP_AC 0x10
#define ARP_FC 0x40
#define TR_RII 0x80

typedef struct RC {
    uchar rc_blen;  // Broadcast indicator and length.
    uchar rc_dlf;   // Direction and largest frame.
} RC;
#define RC_DIR      0x80
#define RC_LENMASK  0x1f
#define RC_SRBCST   0xc2  // Single route broadcast RC.
#define RC_BCST_LEN 0x70  // Length for a broadcast.
#define RC_LF_MASK  0x70  // Mask for length bits.

// Structure of source routing information.
typedef struct SRInfo {
    RC sri_rc;         // Routing control info.
    ushort sri_rd[1];  // Routing designators.
} SRInfo;
#define MAX_RD 8

//
// Structure of an FDDI header.
//
typedef struct FDDIHeader {
    uchar fh_pri;
    uchar fh_daddr[IEEE_802_ADDR_LENGTH];
    uchar fh_saddr[IEEE_802_ADDR_LENGTH];
} FDDIHeader;
#define FDDI_PRI 0x57
#define FDDI_MSS 4352

//
// Structure of an ARCNET header.
//
typedef struct ARCNetHeader {
    uchar ah_saddr;
    uchar ah_daddr;
    uchar ah_prot;
} ARCNetHeader;

//
// Structure of a SNAP header.
//
typedef struct SNAPHeader {
    uchar sh_dsap;
    uchar sh_ssap;
    uchar sh_ctl;
    uchar sh_protid[3];
    ushort sh_etype;
} SNAPHeader;
#define SNAP_SAP 170
#define SNAP_UI 3

#define MAX_MEDIA_ETHER sizeof(EtherHeader)
#define MAX_MEDIA_TR (sizeof(TRHeader)+sizeof(RC)+(MAX_RD*sizeof(ushort))+sizeof(SNAPHeader))
#define MAX_MEDIA_FDDI (sizeof(FDDIHeader)+sizeof(SNAPHeader))
#define MAX_MEDIA_ARC sizeof(ARCNetHeader)

#define ETHER_BCAST_MASK 0x01
#define TR_BCAST_MASK    0x80
#define FDDI_BCAST_MASK  0x01
#define ARC_BCAST_MASK   0xff

#define ETHER_BCAST_VAL 0x01
#define TR_BCAST_VAL    0x80
#define FDDI_BCAST_VAL  0x01
#define ARC_BCAST_VAL   0x00

#define ETHER_BCAST_OFF 0x00
#define TR_BCAST_OFF FIELD_OFFSET(struct TRHeader, tr_daddr)
#define FDDI_BCAST_OFF FIELD_OFFSET(struct FDDIHeader, fh_daddr)
#define ARC_BCAST_OFF FIELD_OFFSET(struct ARCNetHeader, ah_daddr)


//
// Lan driver specific information we keep on a per-interface basis.
//
typedef struct LanInterface {
    void *ai_context;                     // Upper layer context info.
    NDIS_HANDLE ai_handle;                // NDIS binding handle.
    KSPIN_LOCK ai_lock;                   // Lock for this structure.
    PNDIS_PACKET ai_tdpacket;             // Transfer Data packet.
    uchar ai_state;                       // State of the interface.
    int ai_resetting;                     // Is the interface resetting?
    uint ai_pfilter;                      // Packet filter for this I/F.

    //
    // Used for calling NdisOpenAdapter and NdisCloseAdapter.
    // BUGBUG - Is there a possibility of concurrent usage?
    //
    KEVENT ai_event;
    NDIS_STATUS ai_status;

    NDIS_MEDIUM ai_media;                 // Media type.
    uchar ai_addr[IEEE_802_ADDR_LENGTH];  // Local HW address.
    uchar ai_addrlen;                     // Length of ai_addr.
    uchar ai_bcastmask;                   // Mask for checking unicast.
    uchar ai_bcastval;                    // Value to check against.
    uchar ai_bcastoff;                    // Offset in frame to check against.
    uchar ai_hdrsize;                     // Size of link-level header.
    ushort ai_mtu;                        // MTU for this interface.
    uint ai_speed;                        // Speed.
    uint ai_qlen;                         // Output queue length.
    char *ai_desc;                        // Descriptor string.
    uint ai_desclen;                      // Length of desc. string.

    uint ai_uknprotos;                    // Unknown protocols received.
    uint ai_inoctets;                     // Input octets.
    uint ai_inpcount[2];                  // Count of packets received.
    uint ai_indiscards;                   // Input packets discarded.
    uint ai_inerrors;                     // Input errors.
    uint ai_outoctets;                    // Output octets.
    uint ai_outpcount[2];                 // Count of packets sent.
    uint ai_outdiscards;                  // Output packets discarded.
    uint ai_outerrors;                    // Output errors.
} LanInterface;

//
// NOTE: These two values MUST stay at 0 and 1.
//
#define AI_UCAST_INDEX    0
#define AI_NONUCAST_INDEX 1

// Ether Types
//
// Note that the Ether-Type field from classic Ethernet frames coincides
// in the header with the Length field in IEEE 802.3 frames.  This field
// can be used to distinguish between frame types as valid values for the
// 802.3 Length field are from 0x0000 to 0x05dc.  All valid Ether Types
// are greater than this.  See discussion on p.25 of RFC 1122.
//
#define ETYPE_MIN  0x05dd
#define ETYPE_IPv6 0x86dd

// ARCNet protocols
#define ARCPROT_IPv6 0xd4  // BUGBUG - what is correct here?

// The size we need to back off the buffer length because ARCNet address
// are one bytes instead of six.
#define ARCNET_ARPHEADER_ADJUSTMENT 10

#endif  // LAN_INCLUDED

#ifndef _BASETSD_H_
#define _BASETSD_H_

//
// These definitions are normally found in basetsd.h.
// They are provided here so we can build in the NT 4 DDK.
//

typedef ULONG ULONG_PTR;

#endif // _BASETSD_H_

#ifndef __NET_PNP__
#define __NET_PNP__

//
// These definitions are normally found in ndis.h.
// They are provided here so we can build in the NT 4 DDK.
//

//
// PnP and PM event codes that can be indicated up to transports
// and clients.
//
typedef enum _NET_PNP_EVENT_CODE
{
    NetEventSetPower,
    NetEventQueryPower,
    NetEventQueryRemoveDevice,
    NetEventCancelRemoveDevice,
    NetEventReconfigure,
    NetEventBindList,
    NetEventBindsComplete,
    NetEventPnPCapabilities,
    NetEventMaximum
} NET_PNP_EVENT_CODE, *PNET_PNP_EVENT_CODE;

//
// Networking PnP event indication structure.
//
typedef struct _NET_PNP_EVENT
{
    //
    // Event code describing action to take.
    //
    NET_PNP_EVENT_CODE NetEvent;

    //
    // Event specific data.
    //
    PVOID Buffer;

    //
    // Length of event specific data.
    //
    ULONG BufferLength;

    //
    // Reserved values are for use by respective components only.
    //
    //  Note: these reserved areas must be pointer aligned.
    //  

    ULONG_PTR NdisReserved[4];
    ULONG_PTR TransportReserved[4];
    ULONG_PTR TdiReserved[4];
    ULONG_PTR TdiClientReserved[4];
} NET_PNP_EVENT, *PNET_PNP_EVENT;

//
// The following structure defines the device power states.
//
typedef enum _NET_DEVICE_POWER_STATE
{
    NetDeviceStateUnspecified = 0,
    NetDeviceStateD0,
    NetDeviceStateD1,
    NetDeviceStateD2,
    NetDeviceStateD3,
    NetDeviceStateMaximum
} NET_DEVICE_POWER_STATE, *PNET_DEVICE_POWER_STATE;

#endif // __NET_PNP__

#ifndef NDIS_FLAGS_DONT_LOOPBACK

//
// These definitions are normally found in ndis.h.
// They are provided here so we can build in the NT 4 DDK.
//

typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS
{
#ifdef __cplusplus
    NDIS40_PROTOCOL_CHARACTERISTICS Ndis40Chars;
#else
    NDIS40_PROTOCOL_CHARACTERISTICS;
#endif

    //
    // Placeholders for protocol extensions for PnP/PM etc.
    //
    PVOID ReservedHandlers[4];

    //
    // Start of NDIS 5.0 extensions.
    //

    PVOID CoSendCompleteHandler;
    PVOID CoStatusHandler;
    PVOID CoReceivePacketHandler;
    PVOID CoAfRegisterNotifyHandler;

} NDIS50_PROTOCOL_CHARACTERISTICS;

#endif // NDIS_FLAGS_DONT_LOOPBACK
