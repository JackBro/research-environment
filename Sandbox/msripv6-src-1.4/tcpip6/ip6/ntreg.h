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
// NT registry helper function declarations.
//


#ifndef NTREG_INCLUDED
#define NTREG_INCLUDED 1

NTSTATUS
OpenRegKey(PHANDLE HandlePtr, HANDLE Parent, PWCHAR KeyName);

NTSTATUS
GetRegDWORDValue(HANDLE KeyHandle, PWCHAR ValueName, PULONG ValueData);

NTSTATUS
SetRegDWORDValue(HANDLE KeyHandle, PWCHAR ValueName, PULONG ValueData);

NTSTATUS
GetRegStringValue(HANDLE KeyHandle, PWCHAR ValueName,
                  PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
                  PUSHORT ValueSize);

NTSTATUS
GetRegSZValue(HANDLE KeyHandle, PWCHAR ValueName, PUNICODE_STRING ValueData,
              PULONG ValueType);

NTSTATUS
GetRegMultiSZValue(HANDLE KeyHandle, PWCHAR ValueName,
                   PUNICODE_STRING ValueData);

PWCHAR
EnumRegMultiSz(IN PWCHAR MszString, IN ULONG MszStringLength,
               IN ULONG StringIndex);

VOID
InitRegDWORDParameter(HANDLE RegKey, PWCHAR ValueName, ULONG *Value,
                      ULONG DefaultValue);

#endif  // NTREG_INCLUDED
