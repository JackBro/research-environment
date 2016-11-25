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
// Transmission Control Protocol configuration information.
//


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


//
// TCP global configuration variables.
//
extern uint DeadGWDetect;
extern uint PMTUDiscovery;
extern uint PMTUBHDetect;
extern uint KeepAliveTime;
extern uint KAInterval;
extern uint DefaultRcvWin;
extern uint MaxConnections;
extern uint MaxConnectRexmitCount;
extern uint MaxDataRexmitCount;
extern uint BSDUrgent;
extern uint PreloadCount;
extern uint FinWait2TO;
extern uint NTWMaxConnectCount;
extern uint NTWMaxConnectTime;
extern uint MaxUserPort;


//
// Default values for many of the above globals.
//
#define DEFAULT_DEAD_GW_DETECT TRUE
#define DEFAULT_PMTU_DISCOVERY TRUE
#define DEFAULT_PMTU_BHDETECT FALSE
#define DEFAULT_KA_TIME 7200000
#define DEFAULT_KA_INTERVAL 1000
#define DEFAULT_RCV_WIN 8192
#define DEFAULT_MAX_CONNECTIONS (INVALID_CONN_INDEX - 1)
#define DEFAULT_CONNECT_REXMIT_CNT 3
#define DEFAULT_DATA_REXMIT_CNT 5
#define DEFAULT_BSD_URGENT TRUE
#define DEFAULT_PRELOAD_COUNT 0
#define MAX_PRELOAD_COUNT 32
#define PRELOAD_BLOCK_SIZE 16384
#define NTW_MAX_CONNECT_COUNT 15
#define NTW_MAX_CONNECT_TIME 600
