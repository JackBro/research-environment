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
// SHA-1 (Secure Hash Algorithm) implementation.  This code is based on the
// algorithm given in FIBS PUB 180-1 (Federal Information Processing Standards
// Publication 180-1) as issued by NIST (National Institute of Standards and
// Technology).  See http://www.itl.nist.gov/div897/pubs/fip180-1.htm.
//
// We've kept the terminology used in the above publication for variable and
// function names wherever reasonable.
//

#include "oscfg.h"
#include "sha1.h"

//
// Operations on words.  Most of the operations listed in section 3 of the
// spec are directly available in 'C'.  We don't have a circular shift though,
// so we need to build the circular left shift operation out of two ordinary
// shifts and an OR (as given in the spec).
//
#define S(X, n) (((X) << (n)) | ((X) >> (32 - (n))))

//
// Functions used.  These are the functions listed in section 5 of the spec.
// The spec lists these as four functions, but two are the same function.
// All operands and results are 32 bit words.
//
#define f1(B, C, D) (((B) & (C)) | (~(B) & (D)))
#define f2(B, C, D) ((B) ^ (C) ^ (D))
#define f3(B, C, D) (((B) & (C)) | ((B) & (D)) | ((C) & (D)))

//
// Constants used.  These are the constants listed in section 6 and
// section 7 of the spec.  They're all 32 bit values.
//
#define K0  0x5A827999
#define K20 0x6ED9EBA1
#define K40 0x8F1BBCDC
#define K60 0xCA62C1D6

#define H0 0x67452301
#define H1 0xEFCDAB89
#define H2 0x98BADCFE
#define H3 0x10325476
#define H4 0xC3D2E1F0


//* ProcessBlock - Process a 512 bit (64 byte) block of data.
//
//  As given by section 7 of the spec.
//
void
ProcessBlock(
    SHA1Context *Context,  // Context info maintained across operations.
    uchar *Block)          // Current block to process.
{
    ulong W[80];
    ulong A, B, C, D, E, Temp;
    int T;

    //
    // Step A.
    // Load our incoming data Block into 16 32-bit words W[0] ... W[15].
    // REVIEW: Endian issues.
    //
    for (T = 0; T < 16; T++) {
        W[T] = net_long(*(ulong *)Block);
        Block += 4;
    }

    //
    // Step B.
    // REVIEW: Unroll this loop?  Standard speed/size trade-off.
    //
    for (T = 16; T < 80; T++) {
        W[T] = S(W[T - 3] ^ W[T - 8] ^ W[T - 14] ^ W[T - 16], 1);
    }

    //
    // Step C.
    //
    A = Context->H[0];
    B = Context->H[1];
    C = Context->H[2];
    D = Context->H[3];
    E = Context->H[4];

    //
    // Step D.
    // REVIEW: Unroll these loops?  Standard speed/size trade-off.
    // REVIEW: Or combine into single loop with ifs?  Ditto.
    //
    for (T = 0; T < 20; T++) {
        Temp = S(A, 5) + f1(B, C, D) + E + W[T] + K0;
        E = D;
        D = C;
        C = S(B, 30);
        B = A;
        A = Temp;
    }
    for (T = 20; T < 40; T++) {
        Temp = S(A, 5) + f2(B, C, D) + E + W[T] + K20;
        E = D;
        D = C;
        C = S(B, 30);
        B = A;
        A = Temp;
    }
    for (T = 40; T < 60; T++) {
        Temp = S(A, 5) + f3(B, C, D) + E + W[T] + K40;
        E = D;
        D = C;
        C = S(B, 30);
        B = A;
        A = Temp;
    }
    for (T = 60; T < 80; T++) {
        Temp = S(A, 5) + f2(B, C, D) + E + W[T] + K60;
        E = D;
        D = C;
        C = S(B, 30);
        B = A;
        A = Temp;
    }


    //
    // Step E.
    //
    Context->H[0] += A;
    Context->H[1] += B;
    Context->H[2] += C;
    Context->H[3] += D;
    Context->H[4] += E;
}


//* SHA1Init - Prepare to process data.
//
//  This routine initializes the context structure we use to process the
//  individual data blocks.
//
void
SHA1Init(
    SHA1Context *Context)  // Context info maintained across operations.
{

    //
    // Initialize the H array with initial constants.
    //
    Context->H[0] = H0;
    Context->H[1] = H1;
    Context->H[2] = H2;
    Context->H[3] = H3;
    Context->H[4] = H4;

    //
    // Zero our count of bytes processed so far.
    //
    Context->Processed = 0;
}


//* SHA1Update - Feed some more data into our digest calculator.
//
//  Since we accept updates of arbitrary byte lengths (i.e. this routine
//  doesn't insist upon being feed 512 bit blocks even though that is what the
//  algorithm operates on), we need to buffer whatever data we are given until
//  until we have 512 bits worth to give to ProcessBlock.  Likewise, if we're
//  given more than 512 bits at once, we need to break it into 512 bit blocks.
//
void
SHA1Update(
    SHA1Context *Context,  // Context info maintainted across operations.
    uchar *Data,           // Data comprising this update.
    uint DataLen)          // Length of above in bytes.
{
    uint Offset, Remaining;

    //
    // Determine current offset into Context Buffer.  This is the amount
    // of leftover data from the last call to this routine.
    //
    Offset = Context->Processed & BLOCK_MASK;

    //
    // Increment amount processed.
    //
    Context->Processed += DataLen;

    //
    // Calculate amount of free space remaining in our Context Buffer.
    // Then see if this new data is enough to fill it.
    //
    Remaining = BLOCK_SIZE - Offset;
    if (Offset && (DataLen >= Remaining)) {
        //
        // We have enough data to fill our previously partially-filled
        // block.  Load it up and feed it in.
        //
        memcpy(&Context->Buffer[Offset], Data, Remaining);
        ProcessBlock(Context, Context->Buffer);
        Data += Remaining;
        DataLen -= Remaining;
        Offset = 0;
    }

    //
    // While we have enough data to feed in full blocks, do so directly.
    //
    while (DataLen >= BLOCK_SIZE) {
        ProcessBlock(Context, Data);
        Data += BLOCK_SIZE;
        DataLen -= BLOCK_SIZE;
    }

    //
    // Buffer any leftover data for later.
    //
    if (DataLen) {
        memcpy(&Context->Buffer[Offset], Data, DataLen);
    }
}


uchar Padding[BLOCK_SIZE] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


//* SHA1Final - Extract the digest resulting from our computations.
//
//  Here we add the end of message padding and extract the final result.
//  The padding is comprising of a "1" bit followed by some number of "0"
//  bits followed by a 64-bit integer representing the length of the
//  original message.  The numbers of "0" padding bits is chosen to produce
//  a total message length that is a integral multiple of 512 bits.
//
void
SHA1Final(
    SHA1Context *Context,   // Context info maintained across operations.
    uchar *Digest)          // Where to return resulting digest.
{
    int Offset, PadLength, Loop;
    ulong MessageLen[2];

    //
    // Convert message length to bits and save it for later.
    //
    // BUGBUG: This code assumes that our messages never exceed 2^29 bytes.
    // BUGBUG: Could happen with Jumbo Payloads on media we don't support yet?
    //
    MessageLen[0] = 0;
    MessageLen[1] = net_long(Context->Processed << 3);

    //
    // Determine current offset into Context Buffer.  This is the amount
    // of leftover data remaining in the buffer.
    //
    Offset = Context->Processed & BLOCK_MASK;

    //
    // At a minimum, we need to add 9 bytes of padding.  One byte for the
    // initial "1" bit (and 7 "0"s), and 8 bytes to hold the 64-bit initial
    // (i.e. pre-padding) message length.
    //
    PadLength = BLOCK_SIZE - Offset;
    if (PadLength < 9) {
        PadLength += BLOCK_SIZE;
    }
    SHA1Update(Context, Padding, PadLength - 8);
    SHA1Update(Context, (uchar *)&MessageLen, 8);

    //
    // Place resulting digest where requested.
    //
    for (Loop = 0; Loop < 5; Loop++) {
        *(ulong *)Digest = net_long(Context->H[Loop]);
        Digest += 4;
    }

    //
    // Zero the Context region to protect against snoopers.
    // REVIEW: Bother?
    //
    memset(Context, 0, sizeof(*Context));
}
