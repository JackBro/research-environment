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
// Definitions for our SHA-1 implementation.
//

#ifndef SHA1_INCLUDED
#define SHA1_INCLUDED 1

//
// SHA-1 works on 512 bit (64 bytes) blocks of data.  Although we've #define'd
// this here, other parts of the algorithm depend upon this being 64.
//
#define BLOCK_SIZE 64  // In bytes.
#define BLOCK_MASK 63  // 0x3f.


//
// SHA-1 context.
//
typedef struct {
    ulong H[5];                // Intermediate state information.
    uint Processed;            // Bytes processed so far.
    uchar Buffer[BLOCK_SIZE];  // Buffer to hold incomplete blocks.
} SHA1Context;


//
// Function Prototypes.
//
void
SHA1Init(SHA1Context *);

void
SHA1Update(SHA1Context *, uchar *, uint);

void
SHA1Final(SHA1Context *, uchar [16]);

#endif // SHA1_INCLUDED
