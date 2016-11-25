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
// Program to test our MD5 algorithm implementation.  Derived from the
// RSA Data Security, Inc. MD5 Message-Digest Algorithm.  RSA's original
// copyright follows:
//

//
// Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
// rights reserved.
//
// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// documentation and/or software.
//

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "md5.h"

//
// Length of test block, number of test blocks.
//
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 2000

static void MDString(char *);
static void MDTimeTrial(void);
static void MDTestSuite(void);
static void MDFile(char *);
static void MDFilter(void);
static void MDPrint(unsigned char [16]);
static void MDUsage(void);

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final


//
// Main program.
//
int _CRTAPI1
main(
    int argc,
    char *argv[])
{
    int i;

    if (argc > 1)
        for (i = 1; i < argc; i++)
            if (argv[i][0] == '-' && argv[i][1] == 's')
                MDString (argv[i] + 2);
            else if (strcmp(argv[i], "-t") == 0)
                MDTimeTrial();
            else if (strcmp(argv[i], "-x") == 0)
                MDTestSuite();
            else if (strcmp(argv[i], "-?") == 0)
                MDUsage();
            else
                MDFile(argv[i]);
    else
        MDFilter();

    return 0;
}


//
// Print a friendly usage message.
//
static void
MDUsage(void)
{

    printf("Arguments (may be any combination):\n");
    printf("  -sstring - digests string\n");
    printf("  -t       - runs time trial\n");
    printf("  -x       - runs test script\n");
    printf("  filename - digests file\n");
    printf("  (none)   - digests standard input\n");
}


//
// Digests a string and prints the result.
//
static void
MDString(
    char *string)
{
    MD_CTX context;
    unsigned char digest[16];
    unsigned int len = strlen (string);

    MDInit(&context);
    MDUpdate(&context, string, len);
    MDFinal(digest, &context);

    printf("MD5 (\"%s\") = ", string);
    MDPrint(digest);
    printf("\n");
}


//
// Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
// blocks.
//
static void
MDTimeTrial(void)
{
    MD_CTX context;
    __int64 StartTime, EndTime, Frequency;
    unsigned char block[TEST_BLOCK_LEN], digest[16];
    unsigned int i, Elapsed;

    printf("MD5 time trial.  Digesting %d %d-byte blocks ...",
           TEST_BLOCK_COUNT, TEST_BLOCK_LEN);

    //
    // Initialize block.
    //
    for (i = 0; i < TEST_BLOCK_LEN; i++)
        block[i] = (unsigned char)(i & 0xff);

    //
    // Start timer.
    //
    (void) QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency);
    (void) QueryPerformanceCounter((LARGE_INTEGER *)&StartTime);

    //
    // Digest blocks.
    //
    MDInit(&context);
    for (i = 0; i < TEST_BLOCK_COUNT; i++)
        MDUpdate(&context, block, TEST_BLOCK_LEN);
    MDFinal(digest, &context);

    //
    // Stop timer.
    //
    (void) QueryPerformanceCounter((LARGE_INTEGER *)&EndTime);
    Elapsed = (long) ((1000 * (EndTime - StartTime)) / Frequency);  // in ms.
    
    printf(" done\n");
    printf("Digest = ");
    MDPrint(digest);
    printf("\nTime = %ld milliseconds\n", Elapsed);
    printf("Speed = %ld bytes/ms\n",
           (long)TEST_BLOCK_LEN * (long)TEST_BLOCK_COUNT / Elapsed);
}


//
// Digests a reference suite of strings and prints the results.
//
static void
MDTestSuite(void)
{
    printf ("MD5 test suite:\n");

    MDString("");
    MDString("a");
    MDString("abc");
    MDString("message digest");
    MDString("abcdefghijklmnopqrstuvwxyz");
    MDString("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    MDString("1234567890123456789012345678901234567890"
             "1234567890123456789012345678901234567890");
}


//
// Digests a file and prints the result.
//
static void
MDFile(
    char *filename)
{
    FILE *file;
    MD_CTX context;
    int len;
    unsigned char buffer[1024], digest[16];

    if ((file = fopen (filename, "rb")) == NULL)
        printf ("%s can't be opened\n", filename);

    else {
        MDInit(&context);
        while (len = fread(buffer, 1, 1024, file))
            MDUpdate(&context, buffer, len);
        MDFinal(digest, &context);

        fclose(file);

        printf("MD5 (%s) = ", filename);
        MDPrint(digest);
        printf("\n");
    }
}


//
// Digests the standard input and prints the result.
//
static void
MDFilter(void)
{
    MD_CTX context;
    int len;
    unsigned char buffer[16], digest[16];

    MDInit(&context);
    while (len = fread(buffer, 1, 16, stdin))
        MDUpdate(&context, buffer, len);
    MDFinal(digest, &context);

    MDPrint(digest);
    printf("\n");
}


//
// Prints a message digest in hexadecimal.
//
static void
MDPrint(
    unsigned char digest[16])
{
    unsigned int i;

    for (i = 0; i < 16; i++)
        printf("%02x", digest[i]);
}
