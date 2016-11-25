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
// This source file contains the routines to access the NT Registry for
// configuration info.
//


#include <oscfg.h>
#include <ndis.h>
#include "ntreg.h"

#define WORK_BUFFER_SIZE  512


#ifdef ALLOC_PRAGMA
//
// This code is pagable.
//
#pragma alloc_text(PAGE, GetRegDWORDValue)
#pragma alloc_text(PAGE, SetRegDWORDValue)
#pragma alloc_text(PAGE, InitRegDWORDParameter)
#pragma alloc_text(PAGE, OpenRegKey)
#pragma alloc_text(PAGE, GetRegStringValue)
#pragma alloc_text(PAGE, GetRegSZValue)
#pragma alloc_text(PAGE, GetRegMultiSZValue)

#endif // ALLOC_PRAGMA


//* OpenRegKey
//
//  Opens a Registry key and returns a handle to it.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
OpenRegKey(
    PHANDLE HandlePtr,  // Where to write the opened handle.
    HANDLE Parent,
    PWCHAR KeyName)     // Name of Registry key to open.
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UKeyName;

    PAGED_CODE();

    RtlInitUnicodeString(&UKeyName, KeyName);

    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&ObjectAttributes, &UKeyName,
                               OBJ_CASE_INSENSITIVE, Parent, NULL);

    Status = ZwOpenKey(HandlePtr, KEY_READ, &ObjectAttributes);

    return Status;
}


//* GetRegDWORDValue
//
//  Reads a REG_DWORD value from the registry into the supplied variable.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegDWORDValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to read.
    PWCHAR ValueName,  // Name of the value to read.
    PULONG ValueData)  // Variable into which to read the data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

    status = ZwQueryValueKey(KeyHandle, &UValueName, KeyValueFullInformation,
                             keyValueFullInformation, WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)
                           ((PCHAR)keyValueFullInformation +
                            keyValueFullInformation->DataOffset));
        }
    }

    return status;
}


//* SetRegDWORDValue
//
//  Writes the contents of a variable to a REG_DWORD value.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
SetRegDWORDValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to write.
    PWCHAR ValueName,  // Name of the value to write.
    PULONG ValueData)  // Variable from which to write the data.
{
    NTSTATUS status;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwSetValueKey(KeyHandle, &UValueName, 0, REG_DWORD,
                           ValueData, sizeof(ULONG));

    return status;
}


//* GetRegStringValue
//
//  Reads a REG_*_SZ string value from the Registry into the supplied
//  key value buffer.  If the buffer string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegStringValue(
    HANDLE KeyHandle,   // Open handle to the parent key of the value to read.
    PWCHAR ValueName,   // Name of the value to read.
    PKEY_VALUE_PARTIAL_INFORMATION *ValueData,  // Destination of read data.
    PUSHORT ValueSize)  // Size of the ValueData buffer.  Updated on output.
{
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(KeyHandle, &UValueName,
                             KeyValuePartialInformation, *ValueData,
                             (ULONG) *ValueSize, &resultLength);

    if ((status == STATUS_BUFFER_OVERFLOW) ||
        (status == STATUS_BUFFER_TOO_SMALL)) {
        PVOID temp;

        //
        // Free the old buffer and allocate a new one of the
        // appropriate size.
        //

        ASSERT(resultLength > (ULONG) *ValueSize);

        if (resultLength <= 0xFFFF) {

            temp = ExAllocatePool(NonPagedPool, resultLength);

            if (temp != NULL) {

                if (*ValueData != NULL) {
                    ExFreePool(*ValueData);
                }

                *ValueData = temp;
                *ValueSize = (USHORT) resultLength;

                status = ZwQueryValueKey(KeyHandle, &UValueName,
                                         KeyValuePartialInformation,
                                         *ValueData, *ValueSize,
                                         &resultLength);

                ASSERT((status != STATUS_BUFFER_OVERFLOW) &&
                       (status != STATUS_BUFFER_TOO_SMALL));
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return status;
}


//* GetRegMultiSZValue
//
//  Reads a REG_MULTI_SZ string value from the Registry into the supplied
//  Unicode string.  If the Unicode string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegMultiSZValue(
    HANDLE KeyHandle,           // Open handle to parent key of value to read.
    PWCHAR ValueName,           // Name of value to read.
    PUNICODE_STRING ValueData)  // Destination string for the value data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = GetRegStringValue(KeyHandle, ValueName,
                               (PKEY_VALUE_PARTIAL_INFORMATION *)
                               &(ValueData->Buffer),
                               &(ValueData->MaximumLength));

    if (NT_SUCCESS(status)) {

        keyValuePartialInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION) ValueData->Buffer;

        if (keyValuePartialInformation->Type == REG_MULTI_SZ) {

            ValueData->Length = (USHORT)
                keyValuePartialInformation->DataLength;

            RtlCopyMemory(ValueData->Buffer,
                          &(keyValuePartialInformation->Data),
                          ValueData->Length);
        } else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;

} // GetRegMultiSZValue


//* GetRegSZValue
//
//  Reads a REG_SZ string value from the Registry into the supplied
//  Unicode string.  If the Unicode string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegSZValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to read.
    PWCHAR ValueName,  // Name of the value to read.
    PUNICODE_STRING ValueData,  // Destination string for the value data.
    PULONG ValueType)  // On return, contains Registry type of value read.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = GetRegStringValue(KeyHandle, ValueName,
                               (PKEY_VALUE_PARTIAL_INFORMATION *)
                               &(ValueData->Buffer),
                               &(ValueData->MaximumLength));

    if (NT_SUCCESS(status)) {

        keyValuePartialInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION)ValueData->Buffer;

        if ((keyValuePartialInformation->Type == REG_SZ) ||
            (keyValuePartialInformation->Type == REG_EXPAND_SZ)) {
            WCHAR *src;
            WCHAR *dst;
            ULONG dataLength;

            *ValueType = keyValuePartialInformation->Type;
            dataLength = keyValuePartialInformation->DataLength;

            ASSERT(dataLength <= ValueData->MaximumLength);

            dst = ValueData->Buffer;
            src = (PWCHAR) &(keyValuePartialInformation->Data);

            while (ValueData->Length <= dataLength) {

                if ((*dst++ = *src++) == UNICODE_NULL) {
                    break;
                }

                ValueData->Length += sizeof(WCHAR);
            }

            if (ValueData->Length < (ValueData->MaximumLength - 1)) {
                ValueData->Buffer[ValueData->Length/sizeof(WCHAR)] =
                    UNICODE_NULL;
            }
        } else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;
}


//* InitRegDWORDParameter
//
//  Reads a REG_DWORD parameter from the Registry into a variable.  If the
//  read fails, the variable is initialized to a default.
//
VOID
InitRegDWORDParameter(
    HANDLE RegKey,       // Open handle to the parent key of the value to read.
    PWCHAR ValueName,    // The name of the value to read.
    ULONG *Value,        // Destination variable into which to read the data.
    ULONG DefaultValue)  // Default to assign if the read fails.
{
    NTSTATUS status;

    PAGED_CODE();

    status = GetRegDWORDValue(RegKey, ValueName, Value);

    if (!NT_SUCCESS(status)) {
        //
        // These registry parameters override the defaults, so their
        // absence is not an error.
        //
        *Value = DefaultValue;
    }
}


//* EnumRegMultiSz
//
//  Parses a REG_MULTI_SZ string and returns the specified substring.
//
//  Note: This code is called at raised IRQL.  It is not pageable.
//    
PWCHAR
EnumRegMultiSz(
    IN PWCHAR MszString,       // Pointer to the REG_MULTI_SZ string.
    IN ULONG MszStringLength,  // Length of above, including terminating null.
    IN ULONG StringIndex)      // Index number of substring to return.
{
    PWCHAR string = MszString;

    if (MszStringLength < (2 * sizeof(WCHAR))) {
        return NULL;
    }

    //
    // Find the start of the desired string.
    //
    while (StringIndex) {

        while (MszStringLength >= sizeof(WCHAR)) {
            MszStringLength -= sizeof(WCHAR);

            if (*string++ == UNICODE_NULL) {
                break;
            }
        }

        //
        // Check for index out of range.
        //
        if (MszStringLength < (2 * sizeof(UNICODE_NULL))) {
            return NULL;
        }

        StringIndex--;
    }

    if (MszStringLength < (2 * sizeof(UNICODE_NULL))) {
        return NULL;
    }

    return string;
}
