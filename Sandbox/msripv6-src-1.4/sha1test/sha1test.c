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
// Program to test our SHA-1 algorithm implementation.
//

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "sha1.h"

void SHA1Usage(void);
void SHA1PrintDigest(unsigned char *);
void SHA1String(char *);
void SHA1TestSuite(void);


//
// Main program.
//
int _CRTAPI1
main(
    int ArgCount,
    char *ArgVector[])
{
    int Loop;

    if (ArgCount > 1) {
        for (Loop = 1; Loop < ArgCount; Loop++) {
            if (ArgVector[Loop][0] == '-' && ArgVector[Loop][1] == 's') {
                SHA1String(ArgVector[Loop] + 2);
            } else {
                if (strcmp(ArgVector[Loop], "-x") == 0) {
                    SHA1TestSuite();
                } else {
                    SHA1Usage();
                }
            }
        }
    } else {
        SHA1Usage();
    }

    return 0;
}


//
// Print a friendly usage message.
//
void
SHA1Usage(void)
{
    printf("Usage:\n");
    printf("\t-sstring - digest string\n");
    printf("\t-x       - runs test suite\n");
}


//
// Prints a message digest in hexadecimal.
//
void
SHA1PrintDigest(
    unsigned char *Digest)
{
    unsigned int Loop;

    for (Loop = 0; Loop < 20; Loop++) {
        printf("%02x", Digest[Loop]);
    }
}


//
// Digests a string and prints the result.
//
void
SHA1String(
    char *String)
{
    SHA1Context Context;
    unsigned char Digest[20];
    unsigned int Len = strlen(String);

    SHA1Init(&Context);
    SHA1Update(&Context, String, Len);
    SHA1Final(&Context, Digest);

    printf("SHA-1 (\"%s\") = ", String);
    SHA1PrintDigest(Digest);
    printf("\n");
}


//
// Digests a reference suite of strings and prints the results.
//
static void
SHA1TestSuite(void)
{
    printf ("SHA-1 test suite:\n");

    SHA1String("abc");
    SHA1String("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
}
