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
// Program to test our HMAC-MD5 algorithm implementation.
//

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "md5.h"

//
// BUGBUG: Put these in an include file someplace.
//
void HMAC_MD5KeyPrep(unsigned char *RawKey, unsigned int RawKeySize,
                     unsigned char *Key);
void HMAC_MD5Init(void *Context, unsigned char *Key);
void HMAC_MD5Op(void *Context, unsigned char *Key, unsigned char *Data,
                unsigned int Len);
void HMAC_MD5Final(void *Context, unsigned char *Key, unsigned char *Result);


//
// Run the HMAC-MD5 process over a chunk of data.
//
void
Process(
    unsigned char *RawKey,  // Raw key info.
    unsigned int KeySize,   // Amount of above in bytes.
    unsigned char *Data,    // Data to digest.
    unsigned int DataLen,   // Amount of above in bytes.
    unsigned char *Digest)  // Where to put result.
{
    MD5_CTX Context;
    unsigned char KeyInfo[128];

    HMAC_MD5KeyPrep(RawKey, KeySize, KeyInfo);
    HMAC_MD5Init(&Context, KeyInfo);
    HMAC_MD5Op(&Context, KeyInfo, Data, DataLen);
    HMAC_MD5Final(&Context, KeyInfo, Digest);
}


//
// Dump digest/key content.
//
void
DumpIt(
    unsigned char *It)
{
    unsigned int Loop;

    for (Loop = 0; Loop < 16; Loop++)
        printf("%02x", It[Loop]);
    printf("\n");
}


//
// Run the test vectors from RFC 2104.
//
int _CRTAPI1
main(void)
{
    unsigned char Key1[] = {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                            0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
    unsigned char *Key2 = "Jefe";
    unsigned char *Data, Digest[16];

    printf("HMAC-MD5 test suite:\n");

    Data = "Hi There";
    Process(Key1, 16, Data, 8, Digest);
    printf("key = ");
    DumpIt(Key1);
    printf("data = %s\n", Data);
    printf("digest = ");
    DumpIt(Digest);

    Data = "what do ya want for nothing?";
    Process(Key2, 4, Data, 28, Digest);
    printf("key = %s\n", Key2);
    printf("data = %s\n", Data);
    printf("digest = ");
    DumpIt(Digest);

    return 0;
}
