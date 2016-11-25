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
// NT specific routines for loading and configuring the TCP/IPv6 driver.
//


#include <oscfg.h>
#include <ndis.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdint.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <ip6imp.h>
#include <ip6def.h>
#include <ntddip6.h>
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpcfg.h"
#include <ntddtcp.h>

//
// Global variables.
//
PDEVICE_OBJECT TCPDeviceObject = NULL;
PDEVICE_OBJECT UDPDeviceObject = NULL;
PDEVICE_OBJECT RawIPDeviceObject = NULL;
extern PDEVICE_OBJECT IPDeviceObject;

HANDLE TCPRegistrationHandle;
HANDLE UDPRegistrationHandle;
HANDLE IPRegistrationHandle;

//
// Set to TRUE when the stack is unloading.
//
int Unloading = FALSE;

//
// External function prototypes.
// BUGBUG: These prototypes should be imported via include files.
//

int
TransportLayerInit(void);

NTSTATUS
TCPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
TCPDispatchInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS
GetRegMultiSZValue(HANDLE KeyHandle, PWCHAR ValueName, 
                   PUNICODE_STRING ValueData);

PWCHAR
EnumRegMultiSz(IN PWCHAR MszString, IN ULONG MszStringLength,
               IN ULONG StringIndex);

void
TCPUnload(void);

//
// Local funcion prototypes.
//
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

VOID
DriverUnload(IN PDRIVER_OBJECT DriverObject);

void
TLRegisterProtocol(uchar Protocol, void *RcvHandler, void  *XmitHandler,
                   void *StatusHandler, void *RcvCmpltHandler);

uchar
TCPGetConfigInfo(void);

NTSTATUS
TCPInitializeParameter(HANDLE KeyHandle, PWCHAR ValueName, PULONG Value);


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, TLRegisterProtocol)
#pragma alloc_text(INIT, TCPGetConfigInfo)
#pragma alloc_text(INIT, TCPInitializeParameter)

#endif // ALLOC_PRAGMA


//
//  Main initialization routine for the TCP/IPv6 driver.
//
//  This is the driver entry point, called by NT upon loading us.
//
//
NTSTATUS  //  Returns: final status from the initialization operation.
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,   // TCP/IPv6 driver object.
    IN PUNICODE_STRING RegistryPath)  // Path to our info in the registry.
{
    NTSTATUS Status;
    UNICODE_STRING deviceName;
    USHORT i;
    int initStatus;

    KdPrint(("Tcpip6: In DriverEntry routine\n"));

    TdiInitialize();


    //
    // Initialize network level protocol: IPv6.
    //
    Status = IPDriverEntry(DriverObject, RegistryPath);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Tcpip6: IPv6 init failed, status %lx\n", Status));
        return(Status);
    }

    //
    // Initialize transport level protocols: TCP, UDP, and RawIP.
    //

    //
    // Create the device objects.  IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_TCPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_NETWORK,
                            0, FALSE, &TCPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrint(("Tcpip6: Failed to create TCP device object, status %lx\n",
                  Status));
        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_UDPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_NETWORK,
                            0, FALSE, &UDPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrint(("Tcpip6: Failed to create UDP device object, status %lx\n",
                 Status));
        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_RAW_IPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_NETWORK,
                            0, FALSE, &RawIPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrint(("Tcpip6: Failed to create Raw IP device object, status %lx\n",
                 Status));
        goto init_failed;
    }

    //
    // Initialize the driver object.
    //
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->FastIoDispatch = NULL;
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = TCPDispatch;
    }

    //
    // We special case Internal Device Controls because they are the
    // hot path for kernel-mode clients.
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        TCPDispatchInternalDeviceControl;

    //
    // Intialize the device objects.
    //
    TCPDeviceObject->Flags |= DO_DIRECT_IO;
    UDPDeviceObject->Flags |= DO_DIRECT_IO;
    RawIPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Finally, initialize the stack.
    //
    initStatus = TransportLayerInit();

    if (initStatus == TRUE) {

        RtlInitUnicodeString(&deviceName, DD_TCPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &TCPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_UDPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &UDPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_RAW_IPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &IPRegistrationHandle);

        return(STATUS_SUCCESS);
    }

    KdPrint(("Tcpip6: "
             "TCP/UDP initialization failed, but IP will be available.\n"));

    //
    // REVIEW: Write an error log entry here?
    //
    Status = STATUS_UNSUCCESSFUL;


  init_failed:

    //
    // IP has successfully started, but TCP & UDP failed.  Set the
    // Dispatch routine to point to IP only, since the TCP and UDP
    // devices don't exist.
    //

    if (TCPDeviceObject != NULL) {
        IoDeleteDevice(TCPDeviceObject);
    }

    if (UDPDeviceObject != NULL) {
        IoDeleteDevice(UDPDeviceObject);
    }

    if (RawIPDeviceObject != NULL) {
        IoDeleteDevice(RawIPDeviceObject);
    }

    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = IPDispatch;
    }

    return(STATUS_SUCCESS);
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING WinDeviceName;

    KdPrint(("IPv6: DriverUnload\n"));

    //
    // Start the shutdown process by noting our change of state.
    // This will inhibit our starting new activities.
    // REVIEW - Is this actually needed? Possibly other factors
    // prevent new entries into the stack.
    //
    Unloading = TRUE;

    //
    // Cleanup our modules.
    // This will break connections with NDIS and the v4 stack.
    //
    TCPUnload();
    TunnelUnload();
    IPUnload();
    LanUnload();

    //
    // Deregister with TDI.
    //
    (void) TdiDeregisterDeviceObject(TCPRegistrationHandle);
    (void) TdiDeregisterDeviceObject(UDPRegistrationHandle);
    (void) TdiDeregisterDeviceObject(IPRegistrationHandle);

    //
    // Delete Win32 symbolic links.
    //
    RtlInitUnicodeString(&WinDeviceName, L"\\??\\" WIN_IPV6_BASE_DEVICE_NAME);
    (void) IoDeleteSymbolicLink(&WinDeviceName);

    //
    // Delete our device objects.
    //
    IoDeleteDevice(TCPDeviceObject);
    IoDeleteDevice(UDPDeviceObject);
    IoDeleteDevice(RawIPDeviceObject);
    IoDeleteDevice(IPDeviceObject);
}


//
// Interval in milliseconds between keepalive transmissions until a
// response is received.
//
#define DEFAULT_KEEPALIVE_INTERVAL 1000

//
// Time to first keepalive transmission.  2 hours == 7,200,000 milliseconds
//
#define DEFAULT_KEEPALIVE_TIME 7200000

#if 1

//* TCPGetConfigInfo -
//
// Initializes TCP global configuration parameters.
//
uchar  // Returns: Zero on failure, nonzero on success.
TCPGetConfigInfo(void)
{
    HANDLE keyHandle;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING UKeyName;
    ULONG maxConnectRexmits = 0;
    ULONG maxDataRexmits = 0;
    ULONG pptpmaxDataRexmits = 0;
    ULONG useRFC1122UrgentPointer = 0;

    //
    // Initialize to the defaults in case an error occurs somewhere.
    //
    KAInterval = DEFAULT_KEEPALIVE_INTERVAL;
    KeepAliveTime = DEFAULT_KEEPALIVE_TIME;
    PMTUDiscovery = TRUE;
    PMTUBHDetect = FALSE;
    DeadGWDetect = TRUE;
    DefaultRcvWin = 0;  // Automagically pick a reasonable one.
    MaxConnections = DEFAULT_MAX_CONNECTIONS;
    maxConnectRexmits = MAX_CONNECT_REXMIT_CNT;
    pptpmaxDataRexmits = maxDataRexmits = MAX_REXMIT_CNT;
    BSDUrgent = TRUE;
    FinWait2TO = FIN_WAIT2_TO;
    NTWMaxConnectCount = NTW_MAX_CONNECT_COUNT;
    NTWMaxConnectTime = NTW_MAX_CONNECT_TIME;
    MaxUserPort = MAX_USER_PORT;


    //
    // Read the TCP optional (hidden) registry parameters.
    //
    RtlInitUnicodeString(&UKeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\IPv6\\Parameters"
        );

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&objectAttributes, &UKeyName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenKey(&keyHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        TCPInitializeParameter(keyHandle, L"KeepAliveInterval", &KAInterval);

        TCPInitializeParameter(keyHandle, L"KeepAliveTime", &KeepAliveTime);

        TCPInitializeParameter(keyHandle, L"EnablePMTUBHDetect",
                               &PMTUBHDetect);

        TCPInitializeParameter(keyHandle, L"TcpWindowSize", &DefaultRcvWin);

        TCPInitializeParameter(keyHandle, L"TcpNumConnections", 
                               &MaxConnections);

        TCPInitializeParameter(keyHandle, L"TcpMaxConnectRetransmissions",
                               &maxConnectRexmits);

        if (maxConnectRexmits > 255) {
            maxConnectRexmits = 255;
        }

        TCPInitializeParameter(keyHandle, L"TcpMaxDataRetransmissions",
                               &maxDataRexmits);

        if (maxDataRexmits > 255) {
            maxDataRexmits = 255;
        }

        //
        // If we fail, then set to same value as maxDataRexmit so that the
        // max(pptpmaxDataRexmit,maxDataRexmit) is a decent value
        // Need this since TCPInitializeParameter no longer "initializes"
        // to a default value.
        //

        if(TCPInitializeParameter(keyHandle, L"PPTPTcpMaxDataRetransmissions",
                                  &pptpmaxDataRexmits) != STATUS_SUCCESS) {
            pptpmaxDataRexmits = maxDataRexmits;
        }

        if (pptpmaxDataRexmits > 255) {
            pptpmaxDataRexmits = 255;
        }

        TCPInitializeParameter(keyHandle, L"TcpUseRFC1122UrgentPointer",
                               &useRFC1122UrgentPointer);

        if (useRFC1122UrgentPointer) {
            BSDUrgent = FALSE;
        }

        TCPInitializeParameter(keyHandle, L"TcpTimedWaitDelay", &FinWait2TO);

        if (FinWait2TO < 30) {
            FinWait2TO = 30;
        }
        if (FinWait2TO > 300) {
            FinWait2TO = 300;
        }
        FinWait2TO = MS_TO_TICKS(FinWait2TO*1000);

        NTWMaxConnectTime = MS_TO_TICKS(NTWMaxConnectTime*1000);

        TCPInitializeParameter(keyHandle, L"MaxUserPort", &MaxUserPort);

        if (MaxUserPort < 5000) {
            MaxUserPort = 5000;
        }
        if (MaxUserPort > 65534) {
            MaxUserPort = 65534;
        }

        //
        // Read a few IP optional (hidden) registry parameters that TCP
        // cares about.
        //
        TCPInitializeParameter(keyHandle, L"EnablePMTUDiscovery",
                               &PMTUDiscovery);

        TCPInitializeParameter(keyHandle, L"EnableDeadGWDetect",
                               &DeadGWDetect);

        ZwClose(keyHandle);
    }

    MaxConnectRexmitCount = maxConnectRexmits;

    //
    // Use the greater of the two, hence both values should be valid
    //

    MaxDataRexmitCount = (maxDataRexmits > pptpmaxDataRexmits ?
                          maxDataRexmits : pptpmaxDataRexmits);

    return(1);
}
#endif

#define WORK_BUFFER_SIZE 256

//* TCPInitializeParameter - Read a value from the registry.
//
//  Initializes a ULONG parameter from the registry.
//
NTSTATUS
TCPInitializeParameter(
    HANDLE KeyHandle,  // An open handle to the registry key for the parameter.
    PWCHAR ValueName,  // The UNICODE name of the registry value to read.
    PULONG Value)      // The ULONG into which to put the data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING UValueName;

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

    status = ZwQueryValueKey(KeyHandle, &UValueName, KeyValueFullInformation,
                             keyValueFullInformation, WORK_BUFFER_SIZE,
                             &resultLength);

    if (status == STATUS_SUCCESS) {
        if (keyValueFullInformation->Type == REG_DWORD) {
            *Value = *((ULONG UNALIGNED *) ((PCHAR)keyValueFullInformation +
                                  keyValueFullInformation->DataOffset));
        }
    }

    return(status);
}
