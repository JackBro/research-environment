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
// NT specific routines for loading and configuring the IP driver.
//


#define _CTYPE_DISABLE_MACROS  // REVIEW: does this do anything?

#include <oscfg.h>
#include <ndis.h>
#include <ip6imp.h>
#include "ip6def.h"
#include <ntddip6.h>
#include <tdiinfo.h>
#include "ntreg.h"


//
// Global variables.
//
PDRIVER_OBJECT IPDriverObject;
PDEVICE_OBJECT IPDeviceObject;
uint UseEtherSnap = FALSE;
WCHAR RegKeyNameParam[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" TCPIPV6_NAME L"\\Parameters";

//
// Local funcion prototypes
//
NTSTATUS
IPDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

uint
UseEtherSNAP(PNDIS_STRING Name);


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IPDriverEntry)
#pragma alloc_text(PAGE, UseEtherSNAP)

#endif // ALLOC_PRAGMA


//
// Function definitions
//

//* IPDriverEntry
//
// This is the IPv6 protocol initialization entry point, called from
// the common DriverEntry routine upon loading.
//
NTSTATUS                              // Status of initialization operation.
IPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,   // Common TCP/IP driver object.
    IN PUNICODE_STRING RegistryPath)  // Path to our info in the registry.
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING WinDeviceName;

    IPDriverObject = DriverObject;

    //
    // Create the device object.  IoCreateDevice zeroes the memory
    // occupied by the object.
    //
    RtlInitUnicodeString(&DeviceName, DD_IPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_NETWORK,
                            0, FALSE, &IPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("IP init failed: "
                 "Unable to create device object %ws, Status %lx.",
                 DD_IPV6_DEVICE_NAME, Status));

        return(Status);
    }

    //
    // Intialize the device object.
    //
    IPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the list of pending echo request IRPs.
    //
    InitializeListHead(&PendingEchoList);

    //
    // Read configuration parameters from the registry
    // and then initialize.
    //
    // We pass around a string name for the registry parameter key,
    // instead of passing around a key handle, because some code
    // needs to hold onto the name and consult parameters later.
    // Indefitely holding onto a key handle would be bad.
    //
    if (!IPConfigure(RegKeyNameParam) || !IPInit(RegKeyNameParam)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrint(("IP initialization failed.\n"));

        IoDeleteDevice(IPDeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Create a Win32-accessible link for the device.
    // This will allow Windows programs to make IOCTLs.
    //
    RtlInitUnicodeString(&WinDeviceName, L"\\??\\" WIN_IPV6_BASE_DEVICE_NAME);

    Status = IoCreateSymbolicLink(&WinDeviceName, &DeviceName);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("IPv6: IoCreateSymbolicLink failed\n"));

        IoDeleteDevice(IPDeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


//* IPConfigure
//
//  Read configuration parameters from the registry.
//  These parameters are "global" - they are not per-interface.
//
int
IPConfigure(
    WCHAR *RegKeyNameParam)
{
    HANDLE myRegKey;

    if (!NT_SUCCESS(OpenRegKey(&myRegKey, NULL, RegKeyNameParam)))
        return FALSE;

    InitRegDWORDParameter(myRegKey,
                          L"DefaultCurHopLimit",
                          &DefaultCurHopLimit,
                          0x80);

    ZwClose(myRegKey);
    return TRUE;
}


//* UseEtherSNAP
//
//  Determines whether the EtherSNAP protocol should be used on an interface.
//
uint  // Returns: Nonzero if SNAP is to be used on the I/F.  Zero otherwise.
UseEtherSNAP(
    PNDIS_STRING Name)  // Device name of the interface in question.
{
    UNREFERENCED_PARAMETER(Name);

    //
    // We currently set this on a global basis.
    //
    return(UseEtherSnap);
}
