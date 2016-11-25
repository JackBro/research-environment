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
// IP Security Policy/Association management tool.
//

#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <ip6.h>
#include <stdio.h>
#include <stdlib.h>

#include "ipsec.h"

HANDLE Handle;

ulong GetLastSecPolicyIndex(uint SPInterface);
ulong GetLastSecAssocIndex(void);
void CreateSecurityPolicyFile(int argc, char *argv[]);
void CreateGUIResultFile(int argc, char *argv[]);
void CreateSecurityAssociationFile(int argc, char *argv[]);
void QuerySecurityPolicyList(int argc, char *argv[]);
void QuerySecurityAssociationList(void);
char * ReadFileToString(int argc, char *argv[], int Type);
char * ReadGUIFileToString(int argc, char *argv[], int Type);
void CreateSPInKernel(char *SPString);
void CreateSAInKernel(char *SAString);
void InsertSPInKernel(char *SPString, char *argv[]);
void DeleteSPInKernel(int argc, char *argv[]);
void DeleteSAInKernel(int argc, char *argv[]);
void SetMobilitySecurity(int argc, char *argv[]);

IPv6Addr UnspecifiedAddr = { 0 };

#define SP_FILE 1
#define SA_FILE 0

#define MAX_KEY_SIZE 1024

//
// Amount of "____" space.
//
#define SA_FILE_BORDER      236
#define SA_SCREEN_BORDER    219
#define SP_FILE_BORDER      258
#define SP_SCREEN_BORDER    237

// 
// Transport Protocols
//
#define IP_PROTOCOL_TCP     6
#define IP_PROTOCOL_UDP     17
#define IP_PROTOCOL_ICMPv6  58

typedef struct SecPolicyEntry SecPolicyEntry;
typedef struct SecAssocEntry SecAssocEntry;

struct SecPolicyEntry {
    SecPolicyEntry *Next;
    IPV6_CREATE_SECURITY_POLICY CreateSP;
};

struct SecAssocEntry {
    SecAssocEntry *Next;
    IPV6_CREATE_SECURITY_ASSOCIATION CreateSA;
};

void
usage(void)
{
    printf("usage: ipsec\n");
    printf("\tsp [interface] - Print the SP entries to the screen.\n");
    printf("\tsa - Print the SA entries to the screen.\n");      
    printf("\tc [filename <no ext>] - "
           "Create files used to enter the SP and SA entries.\n");
    printf("\ta [filename <no ext>] - "
           "Add the SP and SA entries.\n");
    printf("\ti [policy] [filename <no ext>] - "
           "Insert the SP and SA entries.\n"); 
    printf("\td [type = sp sa] [index] - Delete the SP or SA entry.\n");
    printf("\tm off/on.\n");
      
    exit(1);
}


int _CRTAPI1
main(int argc, char **argv)
{
    int err;
    ulong SPIndex, SAIndex;
    WSADATA WsaData;
    FILE *cfPtr;
    char *String;

    err = WSAStartup(MAKEWORD(2, 0), &WsaData);
    if (err) {
        printf("Unable to initialize Windows Sockets interface, "
               "error code %d\n", GetLastError());
        exit(1);
    }

    if (argc < 2) {
        usage();
    }

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,      // access mode
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        printf("Could not access IPv6 protocol stack\n");
        exit(1);
    }
           
    if (!strcmp(argv[1], "sp")) {
        QuerySecurityPolicyList(argc - 2, argv + 2);

    } else if (!strcmp(argv[1], "sa")) {
        QuerySecurityAssociationList();

    } else if (!strcmp(argv[1], "c")) {
        CreateSecurityPolicyFile(argc - 2, argv + 2);
        CreateSecurityAssociationFile(argc - 2, argv + 2);

    } 
    else if (!strcmp(argv[1], "cg"))
    {
        CreateGUIResultFile(argc - 2, argv + 2);        
    }
    else if (!strcmp(argv[1], "a")) {
        String = ReadFileToString(argc - 2, argv + 2, SP_FILE);
        CreateSPInKernel(String);

        String = ReadFileToString(argc - 2, argv + 2, SA_FILE);
        CreateSAInKernel(String);

    } 
    else if (!strcmp(argv[1], "ag"))
    {
        String = ReadGUIFileToString(argc - 2, argv + 2, SP_FILE);
        CreateSPInKernel(String);

        String = ReadGUIFileToString(argc - 2, argv + 2, SA_FILE);
        CreateSAInKernel(String);
    }
    else if (!strcmp(argv[1], "i"))
    {
        String = ReadFileToString(argc - 3, argv + 3, SP_FILE);
        InsertSPInKernel(String, argv + 2);

        String = ReadFileToString(argc - 3, argv + 3, SA_FILE);
        CreateSAInKernel(String); 
        
    }     
    else if (!strcmp(argv[1], "d"))
    {
        if(argc < 3)
        {
            usage();
        }

        if(!strcmp(argv[2], "sp"))
        {
            DeleteSPInKernel(argc - 3, argv + 3);
        }
        else if (!strcmp(argv[2], "sa"))
        {
            DeleteSAInKernel(argc - 3, argv + 3);
        }
        else
        {
            usage();
        }
           
    }  
    else if (!strcmp(argv[1], "m"))
    {        
        SetMobilitySecurity(argc - 2, argv + 2);        
    }
    else {
        usage();
    }

    return(0);
}


int
GetAddress(char *astr, IPv6Addr *address)
{
    struct hostent *hostp;
    int error_num;
        
    hostp = getipnodebyname(astr, AF_INET6, AI_DEFAULT, &error_num);
    if ((hostp != NULL) && (hostp->h_length == sizeof *address)) 
    {        
        memcpy(address, hostp->h_addr, sizeof *address);
    }
    else 
    {        
        return FALSE;
    }

    freehostent(hostp);
        
    return TRUE;    
}

char *
FormatIPv6Address(IPv6Addr *Address)
{
    static char buffer[41];
    DWORD buflen = sizeof buffer;
    struct sockaddr_in6 sin6;

    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = 0;
    sin6.sin6_flowinfo = 0;
    sin6.sin6_scope_id = 0;

    memcpy(&sin6.sin6_addr, Address, sizeof *Address);

    if (IN6_ADDR_EQUAL(Address, &UnspecifiedAddr)) {
        strcpy(buffer, "POLICY");
    } else
        if (WSAAddressToString((struct sockaddr *) &sin6,
                               sizeof sin6,
                               NULL,       // LPWSAPROTOCOL_INFO
                               buffer,
                               &buflen) == SOCKET_ERROR) {
            strcpy(buffer, "<invalid>");
        }

    return buffer;
}


int
GetIPAddressEntered(char *AddrString, IPv6Addr *Address, int EntryNum, 
                    FILE *FilePtr)
{
    if (!strcmp(AddrString, "POLICY")) {
        *Address = UnspecifiedAddr;
    } else {
        if (! GetAddress(AddrString, Address)) {
            printf("Bad IPv6 Address, %s, SA Entry %d.\n", AddrString, EntryNum);
            fprintf(FilePtr, "Bad SA IPv6 Address \"%s\".\n", AddrString);
            fclose(FilePtr);
            exit(1);
        }
    }

    return(TRUE);
}


char *
FormatSPIPv6Address(IPv6Addr *AddressStart, IPv6Addr *AddressEnd, uint AddressField)
{
    const char *PointerReturn;
    static char Buffer[100];
    char TempBuffer[100];
    DWORD Buflen = sizeof Buffer;
    struct sockaddr_in6 sin6;

    switch (AddressField)
    {

    case WILDCARD_VALUE:            
        strcpy(Buffer, "*");

        break;

    case SINGLE_VALUE:        
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;

        memcpy(&sin6.sin6_addr, AddressStart, sizeof *AddressStart);
                
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            Buffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(Buffer, "???");
        }       

        break;

    case RANGE_VALUE:
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;

        memcpy(&sin6.sin6_addr, AddressStart, sizeof *AddressStart);
                
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            Buffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(Buffer, "???");
        }  
        
        memcpy(&sin6.sin6_addr, AddressEnd, sizeof *AddressEnd);
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;  
        
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            TempBuffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(TempBuffer, "???");
        } 
        
        strcat(Buffer, "-");
        strcat(Buffer, TempBuffer);        

        break;

    default:
        strcpy(Buffer, "???");

        break;
    }

    return Buffer;
}

void
GetSPIPv6Address(
    char *AddressRange, 
    IPv6Addr *AddressStart, 
    IPv6Addr *AddressEnd,
    uint *AddressField,
    int EntryNum, 
    FILE *FilePtr)
{
    char String[100], AddressString2[100];
    char *AddressString;    
    int l, m, n;
        
    strcpy(String, AddressRange);
    AddressString = strpbrk(String, "-");

    if(AddressString == NULL)
    {
        if (!strcmp(String, "*")) 
        {
            *AddressStart = UnspecifiedAddr;
            *AddressField = WILDCARD_VALUE;
        }         
        else 
        {
            if (! GetAddress(String, AddressStart)) 
            {
                printf("Bad IPv6 Address, %s, SP Entry %d.\n", String, EntryNum);
                fprintf(FilePtr, "Bad SP IPv6 Address \"%s\".\n", String);
                fclose(FilePtr);
                exit(1);
            } 
            
            *AddressField = SINGLE_VALUE;
        }

        *AddressEnd = UnspecifiedAddr;
    }

    else
    {
        n = strlen(String);
        m = strlen(AddressString);
        l = n - m;
        
        strncpy(AddressString2, String, l);
        
        if (! GetAddress(AddressString2, AddressStart)) 
        {
            printf("Bad IPv6 Start Address Range, %s, Entry %d.\n", String, EntryNum);
            fprintf(FilePtr, "Bad SP IPv6 Start Address Range \"%s\".\n", String);
            fclose(FilePtr);
            exit(1);
        } 
        
        AddressString++;

        if (! GetAddress(AddressString, AddressEnd)) 
        {
            printf("Bad IPv6 End Address Range, %s, Entry %d.\n", String, EntryNum);
            fprintf(FilePtr, "Bad SP IPv6 End Address Range \"%s\".\n", String);
            fclose(FilePtr);
            exit(1);
        } 
        
        *AddressField = RANGE_VALUE;
    } 
    
}


char *
FormatIPSecProto(uint ProtoNum)
{
    static char buffer[5];

    switch(ProtoNum) {

    case IP_PROTOCOL_AH:
        strcpy(buffer, "AH");
        break;

    case IP_PROTOCOL_ESP:
        strcpy(buffer, "ESP");
        break;

    case NONE:
        strcpy(buffer, "NONE");
        break;

    default:
        strcpy(buffer, "???");
        break;
    }

    return buffer;
}


uint 
GetIPSecProto(char *Protocol, int EntryNum)
{
    uint result;

    if (!strcmp(Protocol, "AH")) {
        result = IP_PROTOCOL_AH;

    } else if (!strcmp(Protocol, "ESP")) {
        result = IP_PROTOCOL_ESP;

    } else if (!strcmp(Protocol, "NONE")) {
        result = NONE;

    } else {
        printf("Bad IPsec Protocol Value Entry %d\n", EntryNum);        
        exit(1);
    }

    return result;
}


char *
FormatIPSecMode(uint Mode)
{
    static char buffer[12];

    switch(Mode) {

    case TRANSPORT:
        strcpy(buffer, "TRANSPORT");
        break;

    case TUNNEL:
        strcpy(buffer, "TUNNEL");
        break;
    
    case NONE:
        strcpy(buffer, "*");
        break;

    default:
        strcpy(buffer, "???");
        break;
    }

    return buffer;
}


uint
GetIPSecMode(char *Mode, int EntryNum)
{
    uint result;

    if (!strcmp(Mode, "TRANSPORT")) {
        result = TRANSPORT;

    } else if (!strcmp(Mode, "TUNNEL")) {
        result = TUNNEL;   
        
    } else if (!strcmp(Mode, "*")) {
        result = NONE;

    } else {
        printf("Bad IPsec Mode Value Entry %d\n", EntryNum);
        exit(1);
    }

    return result;
}


char *
FormatRemoteGW(uint Mode, IPv6Addr *Address)
{
    static char buffer[41];

    switch (Mode) {

    case TRANSPORT:
        strcpy(buffer, "*");
        break;

    case TUNNEL:
    case NONE:  
        if (IN6_ADDR_EQUAL(Address, &UnspecifiedAddr))
        {
            strcpy(buffer, "*");    
        }
        else
        {            
            strcpy(buffer, FormatIPv6Address(Address)); 
        }

        break;

    default:
        break;
    }

    return buffer;
}


int
GetRemoteGW(char *AddrString, IPv6Addr *Address, uint Mode, int EntryNum, 
            FILE *FilePtr)
{
    switch (Mode) {

    case TRANSPORT:    
        *Address = UnspecifiedAddr;
        break;

    case TUNNEL:
    case NONE:
        if (!strcmp(AddrString, "*")) {
            *Address = UnspecifiedAddr;

        } else
            if (! GetAddress(AddrString, Address)) {
                printf("Bad IPv6 Address for RemoteGWIPAddr Entry %d\n", EntryNum);
                fprintf(FilePtr, "Bad Remote Tunnel IPv6 Address \"%s\"\n", 
                    AddrString);
                fclose(FilePtr);
                exit(1);
            }
        break;
    
    default:
        break;
    }

    return TRUE;
}


char *
FormatSATransportProto(ushort Protocol)
{
    static char buffer[5];

    switch (Protocol) {

    case IP_PROTOCOL_TCP:
        strcpy(buffer, "TCP");
        break;

    case IP_PROTOCOL_UDP:
        strcpy(buffer, "UDP");
        break;

    case IP_PROTOCOL_ICMPv6:
        strcpy(buffer, "ICMP");
        break;

    case NONE:
        strcpy(buffer, "POLICY");
        break;

    default:
        strcpy(buffer, "???");
        break;
    }

    return buffer;
}

ushort
GetSATransportProto(char *Protocol, int EntryNum)
{
    ushort result;

    if (!strcmp(Protocol, "TCP")) {
        result = IP_PROTOCOL_TCP;

    } else if (!strcmp(Protocol, "UDP")) {
        result = IP_PROTOCOL_UDP;

    } else if(!strcmp(Protocol, "ICMP")) {
        result = IP_PROTOCOL_ICMPv6;

    } else if (!strcmp(Protocol, "POLICY")) {
        result = NONE;

    } else {
        printf("Bad Protocol Value Entry %d\n", EntryNum);
        exit(1);
    }

    return result;
}


char *
FormatSPTransportProto(ushort Protocol)
{
    static char buffer[5];

    switch (Protocol) {

    case IP_PROTOCOL_TCP:
        strcpy(buffer, "TCP");
        break;

    case IP_PROTOCOL_UDP:
        strcpy(buffer, "UDP");
        break;

    case IP_PROTOCOL_ICMPv6:
        strcpy(buffer, "ICMP");
        break;

    case NONE:
        strcpy(buffer, "*");
        break;

    default:
        strcpy(buffer, "???");
        break;
    }

    return buffer;
}


ushort
GetSPTransportProto(char *Protocol, int EntryNum)
{
    ushort result;

    if (!strcmp(Protocol, "TCP")) {
        result = IP_PROTOCOL_TCP;

    } else if (!strcmp(Protocol, "UDP")) {
        result = IP_PROTOCOL_UDP;

    } else if(!strcmp(Protocol, "ICMP")) {
            result = IP_PROTOCOL_ICMPv6;            
            
    } else if (!strcmp(Protocol, "*")) {
        result = NONE;

    } else {
        printf("Bad Protocol Value Entry %d\n", EntryNum);
        exit(1);
    }

    return result;
}

char *
FormatSAPort(ushort Port)
{
    static char Buffer[10];

    if(Port == NONE)
    {
        strcpy(Buffer, "POLICY");
    }
    else
    {
        _itoa(Port, Buffer, 10);
    }

    return Buffer;
}



uint
GetSAPort(char *Port)
{
    uint result;

    if (!strcmp(Port, "POLICY") || !strcmp(Port, " ")) {
        result = NONE;
    } else {
        result = atoi(Port);
    }

    return result;
}


char *
FormatSPPort(ushort PortStart, ushort PortEnd, uint PortField)
{
    char TempBuffer[10];
    static char Buffer[20];

    switch (PortField) {

    case WILDCARD_VALUE:
        strcpy(Buffer, "*");
        break;
        
    case RANGE_VALUE:
        _itoa(PortEnd, TempBuffer, 10);
        _itoa(PortStart, Buffer, 10);
        strcat(Buffer, "-");
        strcat(Buffer, TempBuffer);
        break;
        
    case SINGLE_VALUE:
        _itoa(PortStart, Buffer, 10);
        break;
        
    default: 
        strcpy(Buffer, "???");
        break;
    }

    return Buffer;
}

void
GetSPPort(
    char *PortRange,    
    ushort *PortStart, 
    ushort *PortEnd,
    uint *PortField,
    int EntryNum)
{
    char String[20], PortString2[20];
    char *PortString;    
    int l, m, n;
        
    strcpy(String, PortRange);
    PortString = strpbrk(String, "-");

    if(PortString == NULL)
    {
        if (!strcmp(String, "*"))
        {
            *PortStart = NONE;
            *PortField = WILDCARD_VALUE;

        } 
        else 
        {
            *PortStart = atoi(String);
            *PortField = SINGLE_VALUE;
        }

        *PortEnd = NONE;
    }

    else
    {
        n = strlen(String);
        m = strlen(PortString);
        l = n - m;
        
        strncpy(PortString2, String, l);
        
        *PortStart = atoi(PortString2);        
        PortString++;
        *PortEnd = atoi(PortString);
        *PortField = RANGE_VALUE;
    } 
    
}



uchar *
FormatSelector(uint Selector)
{
    static char Buffer[2];

    switch (Selector) {

    case PACKET_SELECTOR:
        strcpy(Buffer, "+");
        break;

    case POLICY_SELECTOR:
        strcpy(Buffer, "-");
        break;

    default:
        strcpy(Buffer, "?");
        break;
    }

    return Buffer;
}

uint
GetSelector(char *Selector, int EntryNum)
{
    uint Result;

    if (!strcmp(Selector, "+")) {
        Result = PACKET_SELECTOR;
    } else if (!strcmp(Selector, "-")) {
        Result = POLICY_SELECTOR;
    } else {
        printf("Bad value for one of the selector types in policy %d.\n", EntryNum);
        exit(1);
    }

    return Result;
}
        

char *
FormatIndex(ulong Index)
{
    static char buffer[11];

    switch (Index) {

    case NONE:
        strcpy(buffer, "NONE");
        break;

    default:
        _itoa(Index, buffer, 10);
        break;
    }

    return buffer;
}


ulong
GetIndex(char *Index, int EntryNum)
{
    ulong result;

    if (!strcmp(Index, "NONE")) {
        result = NONE;
    } else {
        result = atoi(Index);
    }

    return result;
}


char *
FormatDirection(uint Direction)
{
    static char buffer[10];

    switch (Direction) {

    case INBOUND:
        strcpy(buffer, "INBOUND");
        break;

    case OUTBOUND:
        strcpy(buffer, "OUTBOUND");
        break;

    case BIDIRECTIONAL:
        strcpy(buffer, "BIDIRECT");
        break;

    default:
        break;
    }

    return buffer;
}


uint 
GetDirection(char *Direction, int EntryNum)
{
    uint result;

    if (!strcmp(Direction, "INBOUND")) {
        result = INBOUND;

    } else if (!strcmp(Direction, "OUTBOUND")) {
        result = OUTBOUND;

    } else if (!strcmp(Direction, "BIDIRECT")) {
        result = BIDIRECTIONAL;

    } else {
         printf("Bad Direction Value Entry %d\n", EntryNum);
         exit(1);
    }

    return result;
}


char *
FormatIPSecAction(uint PolicyFlag)
{
    static char buffer[10];

    switch (PolicyFlag) {

    case IPSEC_BYPASS:
        strcpy(buffer, "BYPASS");
        break;

    case IPSEC_DISCARD:
        strcpy(buffer, "DISCARD");
        break;

    case IPSEC_APPLY:
        strcpy(buffer, "APPLY");
        break;

    case IPSEC_APPCHOICE:
        strcpy(buffer, "APPCHOICE");
        break;

    default:
        break;
    }

    return buffer;
}


uint
GetIPSecAction(char *Action, int EntryNum)
{
    uint result;

    if (!strcmp(Action, "BYPASS")) {
        result = IPSEC_BYPASS;

    } else if (!strcmp(Action, "DISCARD")) {
        result = IPSEC_DISCARD;

    } else if (!strcmp(Action, "APPLY")) {
        result = IPSEC_APPLY;

    } else if (!strcmp(Action, "APPCHOICE")) {
        result = IPSEC_APPCHOICE;

    } else {
         printf("Bad IPSec Action Value Entry %d\n", EntryNum);
         exit(1);
    }

    return result;
}


char *
FormatSPInterface(uint SPInterface[])
{
    int i = 0;
    static char buffer[128];
    char TempBuffer[5];

    strcpy(buffer, "( ");
    while (SPInterface[i] != 0) {
        _itoa(SPInterface[i], TempBuffer, 10);
        strcat(buffer, TempBuffer);
        strcat(buffer, " ");
        i++;
    }

    strcat(buffer,")");

    return buffer;
}

uint
GetAuthAlg(char *AuthAlg, int EntryNum)
{

    if (!strcmp(AuthAlg, "NULL")) {
        return ALGORITHM_NULL;
    }

    if (!strcmp(AuthAlg, "HMAC-MD5")) {
        return ALGORITHM_HMAC_MD5;
    }

    if (!strcmp(AuthAlg, "HMAC-MD5-96")) {
        return ALGORITHM_HMAC_MD5_96;
    }

    if (!strcmp(AuthAlg, "HMAC-SHA1")) {
        return ALGORITHM_HMAC_SHA1;
    }

    if (!strcmp(AuthAlg, "HMAC-SHA1-96")) {
        return ALGORITHM_HMAC_SHA1_96;
    }

    printf("Bad Authentication Algorithm Value Entry %d\n", EntryNum);
    exit(1);
}

char *
FormatAuthAlg(uint AlgorithmId)
{
    static char buffer[15];  // REVIEW: Why buffer?

    switch (AlgorithmId)
    {
    case ALGORITHM_NULL:
        strcpy(buffer, "NULL");
        break;
    case ALGORITHM_HMAC_MD5:
        strcpy(buffer, "HMAC-MD5");
        break;
    case ALGORITHM_HMAC_MD5_96:
        strcpy(buffer, "HMAC-MD5-96");
        break;
    case ALGORITHM_HMAC_SHA1:
        strcpy(buffer, "HMAC-SHA1");
        break;
    case ALGORITHM_HMAC_SHA1_96:
        strcpy(buffer, "HMAC-SHA1-96");
        break;
    default:
        strcpy(buffer, "NONE");  // REVIEW: "UNKNOWN" a better choice?
        break;
    }

    return buffer;
}

uint 
GetKeyFile(char *FileName, uchar *Key, int EntryNum, FILE *ResultFilePtr)
{
    FILE *FilePtr; 
    char *Token;
    char Temp[MAX_KEY_SIZE];
    uint KeySize = 0;

    if(!strcmp(FileName, "NONE"))
    {
        // This is for NULL algorithm.
        strcpy(Key, "NO KEY");
    }
    else
    {
        if ((FilePtr = fopen(FileName, "r")) == NULL) {
            printf("\nKey file %s could not be opened Entry %d.\n", 
                FileName, EntryNum);
            fprintf(ResultFilePtr, "Key File \"%s\" could not be opened.\n", FileName);
            fclose(ResultFilePtr);        
            exit(1);
        }    
        
        fread(Temp, MAX_KEY_SIZE, 1, FilePtr);
        
        Token = strtok(Temp, "\"");
        
        strcpy(Key, Token);
    }

    KeySize = strlen(Key);
    
    if(KeySize == 0)
    {
        printf("Key File %s is empty\n", FileName);
        fprintf(ResultFilePtr, "Key File \"%s\" is empty.\n", FileName);
        fclose(ResultFilePtr);
        fclose(FilePtr);
        exit(1);
    }
    
    return KeySize;
}

void
CreateSecurityPolicyFile(int argc, char *argv[])
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    uint BytesReturned;
    FILE *FilePtr;
    ulong Index;
    int i;
    char FileName[15] = "";

    if (argc == 0) {
        usage();
    }    

    strcpy(FileName, argv[0]);
    strcat(FileName, ".spd");

    if ((FilePtr = fopen(FileName, "w+")) == NULL) {
        printf("\nFile %s could not be opened.\n", FileName);
        exit(1);
    }

    fprintf(FilePtr, "\nSecurity Policy List\n");

    for (i = 0; i < SP_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n");

    fprintf(FilePtr,"%-10s", "Policy");
    fprintf(FilePtr,"%-2s", " ");
    fprintf(FilePtr,"%-40s", "RemoteIPAddr");
    fprintf(FilePtr,"%-2s", " ");
    fprintf(FilePtr,"%-40s", "LocalIPAddr");
    fprintf(FilePtr,"%-2s", " ");
    fprintf(FilePtr,"%-12s", "Protocol");
    fprintf(FilePtr,"%-2s", " ");
    fprintf(FilePtr,"%-12s", "RemotePort");
    fprintf(FilePtr,"%-2s", " ");
    fprintf(FilePtr,"%-12s", "LocalPort");
    fprintf(FilePtr,"%-15s", "IPSecProtocol");
    fprintf(FilePtr,"%-12s", "IPSecMode");
    fprintf(FilePtr,"%-40s", "RemoteGWIPAddr");
    fprintf(FilePtr,"%-15s", "SABundleIndex");
    fprintf(FilePtr,"%-12s", "Direction");
    fprintf(FilePtr,"%-12s", "Action");
    fprintf(FilePtr,"%-15s", "InterfaceIndex");
    fprintf(FilePtr, "\n");

    for (i = 0; i < SP_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n");   

    Query.SPInterface = 0;
    Query.Index = 0;
    
    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                             &Query, sizeof(Query), &Info, sizeof(Info), 
                             &BytesReturned, NULL)) {
            printf("No Policies exist.\n");
            exit(1);
        }

    Query.Index = Info.SPIndex;

    while (Query.Index != 0)
    {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                             &Query, sizeof(Query), &Info, sizeof(Info), 
                             &BytesReturned, NULL)) {
            break;
        }

        fprintf(FilePtr,"%-10lu", Info.SPIndex);

        fprintf(FilePtr,"%-2s", FormatSelector(Info.RemoteAddrSelector));
        fprintf(FilePtr,"%-40s", FormatSPIPv6Address(&Info.RemoteAddr, &Info.RemoteAddrData,
            Info.RemoteAddrField));                

        fprintf(FilePtr,"%-2s", FormatSelector(Info.LocalAddrSelector));
        fprintf(FilePtr,"%-40s", FormatSPIPv6Address(&Info.LocalAddr, &Info.LocalAddrData,
            Info.LocalAddrField));

        fprintf(FilePtr,"%-2s", FormatSelector(Info.TransportProtoSelector));
        fprintf(FilePtr,"%-12s", FormatSPTransportProto(Info.TransportProto));        

        fprintf(FilePtr,"%-2s", FormatSelector(Info.RemotePortSelector));
        fprintf(FilePtr,"%-12s", FormatSPPort(Info.RemotePort, Info.RemotePortData, 
            Info.RemotePortField));        

        fprintf(FilePtr,"%-2s", FormatSelector(Info.LocalPortSelector));
        fprintf(FilePtr,"%-12s", FormatSPPort(Info.LocalPort, Info.LocalPortData,
            Info.LocalPortField));        

        fprintf(FilePtr,"%-15s", FormatIPSecProto(Info.IPSecProtocol));
        fprintf(FilePtr,"%-12s", FormatIPSecMode(Info.IPSecMode));
        fprintf(FilePtr,"%-40s", FormatRemoteGW(Info.IPSecMode, 
                                                &(Info.RemoteSecurityGWAddr)));
        fprintf(FilePtr,"%-15s", FormatIndex(Info.SABundleIndex));
        fprintf(FilePtr,"%-12s", FormatDirection(Info.Direction));
        fprintf(FilePtr,"%-12s", FormatIPSecAction(Info.IPSecAction));
        fprintf(FilePtr,"%-15u", Info.SPInterface);
        fprintf(FilePtr,";");
        fprintf(FilePtr,"\n");

        Query.Index = Info.NextSPIndex;
    }

    for (i=0; i < SP_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n\n");

    fprintf(FilePtr, "- = Take selector from policy\n");
    fprintf(FilePtr, "+ = Take selector from packet\n");


    fclose(FilePtr);

    printf("SPD -> %s\n", FileName);

    return;
}

// Get the highest SP or SA entry numbers.
void
CreateGUIResultFile(int argc, char *argv[])
{ 
    IPV6_QUERY_SECURITY_POLICY_LIST SPQuery;
    IPV6_INFO_SECURITY_POLICY_LIST SPInfo;
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST SAQuery;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST SAInfo;
    uint BytesReturned, Index;
    FILE *FilePtr;    
    char FileName[15] = "";      

    if (argc == 0) {
        usage();
    } 
    
    strcpy(FileName, argv[0]);

    if ((FilePtr = fopen(FileName, "w+")) == NULL) {        
        exit(1);
    }    
    
    SPQuery.SPInterface = 0;
    SPQuery.Index = 0;
    
    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
        &SPQuery, sizeof(SPQuery), &SPInfo, sizeof(SPInfo), 
        &BytesReturned, NULL)) {
        Index = 0;
    }
    else
    {
        Index = SPInfo.SPIndex;
    }

    fprintf(FilePtr, "%d-", Index); 
    
    SAQuery.Index = 0;
    
    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
        &SAQuery, sizeof(SAQuery), &SAInfo, sizeof(SAInfo), 
        &BytesReturned, NULL)) {
        Index = 0;
    }
    else
    {
        Index = SAInfo.SAIndex;
    }   

    fprintf(FilePtr, "%d-\n", Index);

    fclose(FilePtr);    
}

void
CreateSecurityAssociationFile(int argc, char *argv[])
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;
    uint BytesReturned;
    FILE *FilePtr;
    ulong Index;
    int i;
    char FileName[15] = "";

    strcpy(FileName, argv[0]);
    strcat(FileName, ".sad");

    if ((FilePtr = fopen(FileName, "w+")) == NULL) {
        printf("\nFile %s could not be opened.\n", FileName);
        exit(1);
    }

    fprintf(FilePtr, "\nSecurity Association List\n");

    for (i=0; i < SA_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n");

    fprintf(FilePtr, "%-10s", "SAEntry");
    fprintf(FilePtr, "%-15s", "SPI");
    fprintf(FilePtr, "%-40s", "SADestIPAddr");
    fprintf(FilePtr, "%-40s", "DestIPAddr");
    fprintf(FilePtr, "%-40s", "SrcIPAddr");
    fprintf(FilePtr, "%-12s", "Protocol");
    fprintf(FilePtr, "%-12s", "DestPort");
    fprintf(FilePtr, "%-12s", "SrcPort");
    fprintf(FilePtr, "%-12s", "AuthAlg");
    fprintf(FilePtr, "%-15s", "KeyFile");
    fprintf(FilePtr, "%-12s", "Direction");
    fprintf(FilePtr, "%-15s", "SecPolicyIndex");
    fprintf(FilePtr, "\n");

    for (i=0; i < SA_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n");

    Query.Index = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                         &Query, sizeof(Query), &Info, sizeof(Info),
                         &BytesReturned, NULL)) {
        // There are no SA entries yet.
        Info.SAIndex = 0;
    }    

    Query.Index = Info.SAIndex;

    while (Query.Index != 0)
    {    
        if (!DeviceIoControl(Handle,
                             IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                             &Query, sizeof(Query), &Info, sizeof(Info),
                             &BytesReturned, NULL)) {
            break;
        }

        fprintf(FilePtr, "%-10lu", Info.SAIndex);
        fprintf(FilePtr, "%-15lu", Info.SPI);
        fprintf(FilePtr, "%-40s", FormatIPv6Address(&(Info.SADestAddr)));
        fprintf(FilePtr, "%-40s", FormatIPv6Address(&(Info.DestAddr)));
        fprintf(FilePtr, "%-40s", FormatIPv6Address(&(Info.SrcAddr)));
        fprintf(FilePtr, "%-12s",
                FormatSATransportProto(Info.TransportProto));
        fprintf(FilePtr, "%-12s", FormatSAPort(Info.DestPort));
        fprintf(FilePtr, "%-12s", FormatSAPort(Info.SrcPort));
        fprintf(FilePtr, "%-12s", FormatAuthAlg(Info.AlgorithmId));
        fprintf(FilePtr, "%-15s", " ");
        fprintf(FilePtr, "%-12s", FormatDirection(Info.Direction));
        fprintf(FilePtr, "%-15lu", Info.SecPolicyIndex);
        fprintf(FilePtr, "%-1;");
        fprintf(FilePtr, "\n");

        Query.Index = Info.NextSAIndex;
    }

    fprintf(FilePtr, "\n");

    for (i=0; i < SA_FILE_BORDER; i++) {
        fprintf(FilePtr, "_");
    }
    fprintf(FilePtr, "\n");

    fclose(FilePtr);

    printf("SAD -> %s\n", FileName);

    return;
}


void
QuerySecurityPolicyList(int argc, char *argv[])
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    uint BytesReturned;    
    int i;

    if (argc == 0) {
        Query.SPInterface = 0;
    } else {
        Query.SPInterface = atoi(argv[0]);
    }

    if (Query.SPInterface == 0) {
        printf("\nAll Security Policies\n");
    } else {
        printf("\nSecurity Policy List for Interface %d\n",
               Query.SPInterface);
    }

    for (i=0; i < SP_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");

    printf("%-10s", "Policy");
    printf("%-2s", " ");
    printf("%-40s", "RemoteIPAddr");
    printf("%-2s", " ");
    printf("%-40s", "LocalIPAddr");
    printf("%-2s", " ");
    printf("%-12s", "Protocol");
    printf("%-2s", " ");
    printf("%-12s", "RemotePort");
    printf("%-2s", " ");
    printf("%-12s", "LocalPort");
    printf("%-15s", "IPSecProtocol");
    printf("%-12s", "IPSecMode");
    printf("%-40s", "RemoteGWIPAddr");
    printf("%-15s", "SABundleIndex");
    printf("%-12s", "Direction");    
    printf("%-8s\n", "Action");

    for (i = 0; i < SP_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");
    
    Query.Index = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                         &Query, sizeof(Query), &Info, sizeof(Info), 
                         &BytesReturned, NULL)) {
        printf("No Policies exist.\n");
        exit(1);
    }

    // First SP.
    Query.Index = Info.SPIndex;    

    // Get all the SPs one at a time.  Starting with the highest number.
    while (Query.Index != 0)
    { 
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                             &Query, sizeof(Query), &Info, sizeof(Info), 
                             &BytesReturned, NULL)) {
           break;
        }

        printf("%-10lu", Info.SPIndex);

        printf("%-2s", FormatSelector(Info.RemoteAddrSelector));
        printf("%-40s", FormatSPIPv6Address(&Info.RemoteAddr, &Info.RemoteAddrData,
            Info.RemoteAddrField));        

        printf("%-2s", FormatSelector(Info.LocalAddrSelector));
        printf("%-40s", FormatSPIPv6Address(&Info.LocalAddr, &Info.LocalAddrData,
            Info.LocalAddrField));
        
        printf("%-2s", FormatSelector(Info.TransportProtoSelector));
        printf("%-12s", FormatSPTransportProto(Info.TransportProto));
        
        printf("%-2s", FormatSelector(Info.RemotePortSelector));
        printf("%-12s", FormatSPPort(Info.RemotePort, Info.RemotePortData, 
            Info.RemotePortField));
        
        printf("%-2s", FormatSelector(Info.LocalPortSelector));       
        printf("%-12s", FormatSPPort(Info.LocalPort, Info.LocalPortData,
            Info.LocalPortField));
        
        printf("%-15s", FormatIPSecProto(Info.IPSecProtocol));
        printf("%-12s", FormatIPSecMode(Info.IPSecMode));
        printf("%-40s", FormatRemoteGW(Info.IPSecMode,
                                       &(Info.RemoteSecurityGWAddr)));
        printf("%-15s", FormatIndex(Info.SABundleIndex));
        printf("%-12s", FormatDirection(Info.Direction));
        printf("%-8s", FormatIPSecAction(Info.IPSecAction));
        printf("\n");        

        Query.Index = Info.NextSPIndex;
    }

    for (i = 0; i < SP_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");

    return;
}


void
QuerySecurityAssociationList(void)
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;
    uint BytesReturned;    
    int i;

    printf("\nSecurity Association List\n");

    for (i=0; i < SA_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");

    printf("%-10s", "SAEntry");
    printf("%-15s", "SPI");
    printf("%-40s", "SADestIPAddr");
    printf("%-40s", "DestIPAddr");
    printf("%-40s", "SrcIPAddr");
    printf("%-12s", "Protocol");
    printf("%-12s", "DestPort");
    printf("%-12s", "SrcPort");
    printf("%-12s", "AuthAlg");
    printf("%-12s", "Direction");
    printf("%-15s", "SecPolicyIndex");
    printf("\n");

    for (i=0; i < SA_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");

    // Set Query index value to zero to get the total number of SAs.
    Query.Index = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                         &Query, sizeof(Query), &Info, sizeof(Info),
                         &BytesReturned, NULL)) {
        // There are no SA entries yet.
        printf("***No SAs exist***\n");
        exit(1);
    }    

    // First SA.
    Query.Index = Info.SAIndex;

    // Get all the SAs one at a time.  Starting with the highest number.
    while (Query.Index != 0)
    {
        if (!DeviceIoControl(Handle,
                             IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                             &Query, sizeof(Query), &Info, sizeof(Info),
                             &BytesReturned, NULL)) {
            break;
        }

        printf("%-10lu", Info.SAIndex);
        printf("%-15lu", Info.SPI);
        printf("%-40s", FormatIPv6Address(&(Info.SADestAddr)));
        printf("%-40s", FormatIPv6Address(&(Info.DestAddr)));
        printf("%-40s", FormatIPv6Address(&(Info.SrcAddr)));
        printf("%-12s", FormatSATransportProto(Info.TransportProto));
        printf("%-12s", FormatSAPort(Info.DestPort));
        printf("%-12s", FormatSAPort(Info.SrcPort));
        printf("%-12s", FormatAuthAlg(Info.AlgorithmId));
        printf("%-12s", FormatDirection(Info.Direction));
        printf("%-15lu", Info.SecPolicyIndex);
        printf("\n");

        Query.Index = Info.NextSAIndex;
    }

    for (i=0; i < SA_SCREEN_BORDER; i++) {
        printf("_");
    }
    printf("\n");

    return;
}


char *
ReadFileToString(int argc, char *argv[], int Type)
{
    FILE *FilePtr;
    char *Token;
    char TempString[300];
    char EntryString[20000];
    int i;
    char FileName[15] = "";

    if (argc == 0) {
        usage();
    }

    strcpy(FileName, argv[0]);

    if (Type == SP_FILE) {
        strcat(FileName, ".spd");
    } else
        if (Type == SA_FILE) {
            strcat(FileName, ".sad");
        }

    if ((FilePtr = fopen(FileName, "r")) == NULL) {
        printf("\nFile %s could not be opened.\n", FileName);
        exit(1);
    }

    while (!feof(FilePtr)) {
        fscanf(FilePtr, "%s ", TempString);
        strcat(EntryString, TempString);
        strcat(EntryString, " ");
    }

    //printf("Before tokenizing:\n%s\n", EntryString);

    Token = strtok(EntryString, "_");
    Token = strtok(NULL, "_");
    Token = strtok(NULL, "_");

    //printf("Start:  %s\n", Token);

    return(Token);
}

char *
ReadGUIFileToString(int argc, char *argv[], int Type)
{
    FILE *FilePtr;    
    char TempString[300] = "";
    char EntryString[20000] = "";
    char *String;
    int i;
    char FileName[15] = "";

    if (argc == 0) {
        usage();
    }

    strcpy(FileName, argv[0]);

    if (Type == SP_FILE) {
        strcat(FileName, ".spd");
    } else
        if (Type == SA_FILE) {
            strcat(FileName, ".sad");
        }

    if ((FilePtr = fopen(FileName, "r")) == NULL) {
        printf("\nFile %s could not be opened.\n", FileName);
        exit(1);
    }

    while (!feof(FilePtr)) {
        fscanf(FilePtr, "%s ", TempString);
        strcat(EntryString, TempString);
        strcat(EntryString, " ");
    }     

    String = (char *)malloc(strlen(EntryString));

    strcpy(String, EntryString);
    
    return String;
}

void
CreateSPInKernel(char *SPString)
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    IPV6_CREATE_SECURITY_POLICY CreateSP;
    IPV6_CREATE_SECURITY_POLICY_RESULT CreateSPResult;
    SecPolicyEntry *SPEntry, *NextSPEntry;    
    ulong BytesReturned, Index, EntryNum, i;  
    char Result[100];
    FILE *FilePtr;
    char *Token;    
    
    if ((FilePtr = fopen("~ipsec.res", "w+")) == NULL) {        
        exit(1);
    }  
    
    //
    // Get the first SP index from the kernel.
    //
    Query.Index = 0;
    Query.SPInterface = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                         &Query, sizeof(Query), &Info, sizeof(Info), 
                         &BytesReturned, NULL)) {
        Info.SPIndex = 0;        
    }     
       
    // First SP.
    Index = Info.SPIndex;  

    Token = strtok(SPString, " ");

    if(Token == NULL)
    {
        EntryNum = 0;
    }
    else
    {
        EntryNum = atol(Token);
    } 

    SPEntry = NULL;
    NextSPEntry = NULL;    

    while (EntryNum > Index) {

        SPEntry = malloc(sizeof(*SPEntry));       

        SPEntry->Next = NextSPEntry;        

        // Not inserting.
        SPEntry->CreateSP.InsertIndex = 0;

        // Policy Number
        SPEntry->CreateSP.SPIndex = atol(Token);

        i = SPEntry->CreateSP.SPIndex;

        // RemoteIPAddr Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.RemoteAddrSelector = GetSelector(Token, i);
        // RemoteIPAddr
        Token = strtok(NULL, " ");
        GetSPIPv6Address(Token, &SPEntry->CreateSP.RemoteAddr, 
            &SPEntry->CreateSP.RemoteAddrData, &SPEntry->CreateSP.RemoteAddrField,
            i, FilePtr);       
        
        // LocalIPAddr Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.LocalAddrSelector = GetSelector(Token, i);
        // LocalIPAddr
        Token = strtok(NULL, " ");
        GetSPIPv6Address(Token, &SPEntry->CreateSP.LocalAddr, 
            &SPEntry->CreateSP.LocalAddrData, &SPEntry->CreateSP.LocalAddrField,
            i, FilePtr); 
        
        // Protocol Selector        
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.TransportProtoSelector = GetSelector(Token, i);
        // Protocol
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.TransportProto = GetSPTransportProto(Token, i);         

        // RemotePort Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.RemotePortSelector = GetSelector(Token, i);
        // RemotePort
        Token = strtok(NULL, " ");
       
        GetSPPort(Token, &SPEntry->CreateSP.RemotePort, 
            &SPEntry->CreateSP.RemotePortData, &SPEntry->CreateSP.RemotePortField, i); 
            
        // LocalPort Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.LocalPortSelector = GetSelector(Token, i);
        // Local Port
        Token = strtok(NULL, " ");
        GetSPPort(Token, &SPEntry->CreateSP.LocalPort, 
            &SPEntry->CreateSP.LocalPortData, &SPEntry->CreateSP.LocalPortField, i);

        // IPSecProtocol
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecProtocol = GetIPSecProto(Token, i);

        // IPSecMode
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecMode = GetIPSecMode(Token, i);

        // RemoteGWIPAddr
        Token = strtok(NULL, " ");
        GetRemoteGW(Token, &SPEntry->CreateSP.RemoteSecurityGWAddr,
                    SPEntry->CreateSP.IPSecMode, i, FilePtr);

        // SABundleIndex
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.SABundleIndex = GetIndex(Token, i);

        // Direction
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.Direction = GetDirection(Token, i);

        // Action
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecAction = GetIPSecAction(Token, i);

        // Interface SP
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.SPInterface = atol(Token);

        // End of current policy
        Token = strtok(NULL, " ");
        if(strcmp(Token, ";"))
        {            
            printf("Missing \";\" at the end of policy %lu.\n", i);
            printf("Or some of the fields are empty.\n");            
            exit(1);
        }

        // The next policy entry number.
        Token = strtok(NULL, " ");
        if(Token == NULL)
        {
            EntryNum = 0;
        }
        else
        {
            EntryNum = atol(Token);
        }
        
        // Pointer to the current SPEntry
        NextSPEntry = SPEntry;       
    }     

    i = 0;

    //
    // Do this because I need to enter from general to specific.  
    // The SP Entries are more easily read by having the most general 
    // at the bottom of the file.
    //
    while (SPEntry != NULL) 
    {
        // Create the entry in the Kernel.
        if(!DeviceIoControl(Handle, 
            IOCTL_IPV6_CREATE_SECURITY_POLICY,
            &SPEntry->CreateSP, 
            sizeof(SPEntry->CreateSP),
            &CreateSPResult, 
            sizeof(CreateSPResult), 
            &BytesReturned, NULL))
        {
            printf("SP Entry %d could not be created.\n", SPEntry->CreateSP.SPIndex);
            printf("I/O Control Error\n");
            fclose(FilePtr); 
            exit(1);
        }
        
        if(CreateSPResult.Status != CREATE_SUCCESS)
        {
            printf("SP Entry %d could not be created.\n", SPEntry->CreateSP.SPIndex);

            switch(CreateSPResult.Status)
            {      
                case CREATE_MEMORY_ALLOC_ERROR:
                    printf("Memory could not be allocated in kernel.\n");
                    break;
                
                case CREATE_INVALID_SABUNDLE:
                    printf("Invalid SABundleIndex value.\n");
                    break;
                               
                case CREATE_INVALID_INTERFACE:
                    printf("Invalid Interface value.\n");
                    fprintf(FilePtr, "Invalid \"IPSec Interface\".\n");
                    break;
                
                case CREATE_INVALID_INDEX:
                    printf("Invalid Policy value.\n");
                    break;
                
                default:
                    
                    break;
            }
            fclose(FilePtr);                         
            exit(1);
        } // end of (if Status)

        // Pointer to current SPEntry, just reusing the pointer for deleting.
        NextSPEntry = SPEntry;

        SPEntry = SPEntry->Next;

        // Free the last SPEntry
        free(NextSPEntry);

        i++;
       
    } // end of while
    
    printf("%d new SP entries installed.\n", i);    
    
    fclose(FilePtr);   
}

void
CreateSAInKernel(char *SAString)
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;
    IPV6_CREATE_SECURITY_ASSOCIATION CreateSA;
    IPV6_CREATE_SECURITY_ASSOCIATION_RESULT CreateSAResult;  
    SecAssocEntry *SAEntry, *NextSAEntry;
    ulong BytesReturned, EntryNum, Index, i;
    uint TokenSize;
    char *Token, *TokenHold; 
    char Result[100];
    FILE *FilePtr;

    if ((FilePtr = fopen("~ipsec.res", "a+")) == NULL) {        
        exit(1);
    }  
    
    //
    // Get the first SA index from the kernel.
    //
    Query.Index = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                         &Query, sizeof(Query), &Info, sizeof(Info), 
                         &BytesReturned, NULL)) {
        Info.SAIndex = 0;        
    }
    
    // First SA.
    Index = Info.SAIndex;     

    Token = strtok(SAString, " ");
    
    if(Token == NULL)
    {
        EntryNum = 0;
    }
    else
    {
        EntryNum = atol(Token);
    } 
    
    SAEntry = NULL;
    NextSAEntry = NULL;

    while (EntryNum > Index) {      
        
        SAEntry = malloc(sizeof(*SAEntry));       

        SAEntry->Next = NextSAEntry;      

        // Policy Number
        SAEntry->CreateSA.SAIndex = atol(Token);

        i = SAEntry->CreateSA.SAIndex;

        // SPI
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.SPI = atol(Token);

        // SADestAddr
        Token = strtok(NULL, " ");
        GetIPAddressEntered(Token, &SAEntry->CreateSA.SADestAddr, i, FilePtr);

        // DestIPAddr
        Token = strtok(NULL, " ");
        GetIPAddressEntered(Token, &SAEntry->CreateSA.DestAddr, i, FilePtr);

        // SrcIPAddr
        Token = strtok(NULL, " ");
        GetIPAddressEntered(Token, &SAEntry->CreateSA.SrcAddr, i, FilePtr);

        // Protocol
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.TransportProto = GetSATransportProto(Token, i);

        // DestPort
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.DestPort = GetSAPort(Token);

        // SrcPort
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.SrcPort = GetSAPort(Token);

        // AuthAlg
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.AlgorithmId = GetAuthAlg(Token, i);         
        
        Token = strtok(NULL, " ");
        // KeyFile  
        SAEntry->CreateSA.RawKey = (uchar *)malloc(MAX_KEY_SIZE);      
        // Save location since we are tokenizing again.
        TokenSize = strlen(Token);
        TokenHold = Token + TokenSize + 1;
        
        SAEntry->CreateSA.RawKeySize = GetKeyFile(Token, SAEntry->CreateSA.RawKey, 
            i, FilePtr);
        
        Token = TokenHold;
        
        // Direction
        Token = strtok(TokenHold, " ");
        SAEntry->CreateSA.Direction = GetDirection(Token, i);

        // SecPolicyIndex
        Token = strtok(NULL, " ");
        SAEntry->CreateSA.SecPolicyIndex = atol(Token);

        // End of current association
        Token = strtok(NULL, " ");
        if(strcmp(Token, ";"))
        {            
            printf("Missing \";\" at the end of association %lu.\n", i);
            printf("Or some of the fields are empty.\n");
            exit(1);
        }

         // The next policy entry number.
        Token = strtok(NULL, " ");
        if(Token == NULL)
        {
            EntryNum = 0;
        }
        else
        {
            EntryNum = atol(Token);
        }

        // Pointer to the current SPEntry
        NextSAEntry = SAEntry;       
    } // end of while (EntryNum > Index)   
    
    i = 0;

    //
    // Similar to the SP entering.  If there is 
    // a problem with an entry, the entering stops at that point.  Fix the entry
    // and run again.  Only the current entry and later entries are uploaded because
    // lower entries already exist in the kernel.
    //
    while (SAEntry != NULL) 
    {
        // Create the entry in the Kernel.
        if(!DeviceIoControl(Handle, IOCTL_IPV6_CREATE_SECURITY_ASSOCIATION,
                             &SAEntry->CreateSA, 
                             sizeof(SAEntry->CreateSA),
                             &CreateSAResult, 
                             sizeof(CreateSAResult), 
                             &BytesReturned, NULL))
        {
            printf("SA Entry %d could not be created.\n", SAEntry->CreateSA.SAIndex);
            printf("I/O Control Error.\n");
            printf("%d new SA entries installed.\n", i);
            exit(1);
        }

        if(CreateSAResult.Status != CREATE_SUCCESS)
        {
            printf("SA Entry %d could not be created.\n", SAEntry->CreateSA.SAIndex);

            switch(CreateSAResult.Status)
            {      
                case CREATE_MEMORY_ALLOC_ERROR:
                    printf("Memory could not be allocated in kernel.\n");
                    break;
                
                case CREATE_INVALID_INDEX:
                    printf("Invalid SAEntry value.\n");
                    break;

                case CREATE_INVALID_SEC_POLICY:
                    printf("Invalid SecPolicyIndex value.\n");
                    break;

                case CREATE_INVALID_DIRECTION:
                    printf("Invalid Direction value.\n");
                    break;
                
                default:
                    
                    break;
            } 

            printf("%d new SA entries installed.\n", i);
            exit(1);
        }

        // Pointer to current SAEntry, just reusing the pointer for deleting.
        NextSAEntry = SAEntry;

        SAEntry = SAEntry->Next;

        // Free the last SPEntry
        free(NextSAEntry);

        i++;       
    } // end of while        
    
    printf("%d new SA entries installed.\n", i);

    fprintf(FilePtr, "OK");

    fclose(FilePtr);
}

void
InsertSPInKernel(char *SPString, char *argv[])
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    IPV6_CREATE_SECURITY_POLICY CreateSP;
    IPV6_CREATE_SECURITY_POLICY_RESULT CreateSPResult;
    SecPolicyEntry *SPEntry, *NextSPEntry;    
    ulong BytesReturned, Index, EntryNum, i;
    uint BeforeIndex, AfterIndex;
    char *Token;   
    FILE *FilePtr;
    
    FilePtr = NULL;

    // 
    // Get SP info for the insert start point.
    //
    Query.Index = atol(argv[0]);
    Query.SPInterface = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
        &Query, sizeof(Query), &Info, sizeof(Info), 
        &BytesReturned, NULL)) {
        printf("Cannot find the SP specified.\n");
        exit(1);
    } 
    
    //
    // Query again to get the insert end point.
    //    
    Query.SPInterface = Info.SPInterface;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
        &Query, sizeof(Query), &Info, sizeof(Info), 
        &BytesReturned, NULL)) {
        printf("Cannot get Next SP info for insert.\n");
        exit(1);
    }
    
    // Insert up to this index.
    Index = Info.NextSPIndex;  

    Token = strtok(SPString, " ");

    if(Token == NULL)
    {
        EntryNum = 0;
    }
    else
    {
        EntryNum = atol(Token);
    } 

    SPEntry = NULL;
    NextSPEntry = NULL;    

    while (EntryNum > Index) {
        SPEntry = malloc(sizeof(*SPEntry));       

        SPEntry->Next = NextSPEntry;        

        // Insert Point.
        SPEntry->CreateSP.InsertIndex = atol(argv[0]);

        // Policy Number
        SPEntry->CreateSP.SPIndex = atol(Token);

        i = SPEntry->CreateSP.SPIndex;

        // RemoteIPAddr Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.RemoteAddrSelector = GetSelector(Token, i);
        // RemoteIPAddr
        Token = strtok(NULL, " ");
        GetSPIPv6Address(Token, &SPEntry->CreateSP.RemoteAddr, 
            &SPEntry->CreateSP.RemoteAddrData, &SPEntry->CreateSP.RemoteAddrField, 
            i, FilePtr);       
        
         // LocalIPAddr Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.LocalAddrSelector = GetSelector(Token, i);
        // LocalIPAddr
        Token = strtok(NULL, " ");
        GetSPIPv6Address(Token, &SPEntry->CreateSP.LocalAddr, 
            &SPEntry->CreateSP.LocalAddrData, &SPEntry->CreateSP.LocalAddrField,
            i, FilePtr); 

        // Protocol Selector        
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.TransportProtoSelector = GetSelector(Token, i);
        // Protocol
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.TransportProto = GetSPTransportProto(Token, i);        

        // RemotePort Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.RemotePortSelector = GetSelector(Token, i);
        // RemotePort
        Token = strtok(NULL, " ");
        GetSPPort(Token, &SPEntry->CreateSP.RemotePort, 
            &SPEntry->CreateSP.RemotePortData, &SPEntry->CreateSP.RemotePortField, i); 
            
        // LocalPort Selector
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.LocalPortSelector = GetSelector(Token, i);
        // Local Port
        Token = strtok(NULL, " ");
        GetSPPort(Token, &SPEntry->CreateSP.LocalPort, 
            &SPEntry->CreateSP.LocalPortData, &SPEntry->CreateSP.LocalPortField, i);

        // IPSecProtocol
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecProtocol = GetIPSecProto(Token, i);

        // IPSecMode
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecMode = GetIPSecMode(Token, i);

        // RemoteGWIPAddr
        Token = strtok(NULL, " ");
        GetRemoteGW(Token, &SPEntry->CreateSP.RemoteSecurityGWAddr,
                    SPEntry->CreateSP.IPSecMode, i, FilePtr);

        // SABundleIndex
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.SABundleIndex = GetIndex(Token, i);

        // Direction
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.Direction = GetDirection(Token, i);

        // Action
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.IPSecAction = GetIPSecAction(Token, i);

        // Interface SP
        Token = strtok(NULL, " ");
        SPEntry->CreateSP.SPInterface = atol(Token);

        // End of current policy
        Token = strtok(NULL, " ");
        if(strcmp(Token, ";"))
        {            
            printf("Missing \";\" at the end of policy %lu.\n", i);
            printf("Or some of the fields are empty.\n");
            exit(1);
        }

        // The next policy entry number.
        Token = strtok(NULL, " ");
        if(Token == NULL)
        {
            EntryNum = 0;
        }
        else
        {
            EntryNum = atol(Token);
        }
        
        // Pointer to the current SPEntry
        NextSPEntry = SPEntry;       
    }     

    i = 0;

    //
    // Do this because I need to enter from general to specific.  
    // The SP Entries are more easily read by having the most general 
    // at the bottom of the file.
    //
    while (SPEntry != NULL) 
    {
        // Create the entry in the Kernel.
        if(!DeviceIoControl(Handle, 
            IOCTL_IPV6_CREATE_SECURITY_POLICY,
            &SPEntry->CreateSP, 
            sizeof(SPEntry->CreateSP),
            &CreateSPResult, 
            sizeof(CreateSPResult), 
            &BytesReturned, NULL))
        {
            printf("SP Entry %d could not be created.\n", SPEntry->CreateSP.SPIndex);
            printf("I/O Control Error\n");
            exit(1);
        }
        
        if(CreateSPResult.Status != CREATE_SUCCESS)
        {
            printf("SP Entry %d could not be created.\n", SPEntry->CreateSP.SPIndex);

            switch(CreateSPResult.Status)
            {      
                case CREATE_MEMORY_ALLOC_ERROR:
                    printf("Memory could not be allocated in kernel.\n");
                    break;
                
                case CREATE_INVALID_SABUNDLE:
                    printf("Invalid SABundleIndex value.\n");
                    break;
                               
                case CREATE_INVALID_INTERFACE:
                    printf("Invalid Interface value.\n");
                    break;
                
                case CREATE_INVALID_INDEX:
                    printf("Invalid Policy value.\n");
                    break;
                
                default:
                    
                    break;
            }
                        
            exit(1);
        } // end of (if Status)

        // Pointer to current SPEntry, just reusing the pointer for deleting.
        NextSPEntry = SPEntry;

        SPEntry = SPEntry->Next;

        // Free the last SPEntry
        free(NextSPEntry);

        i++;
       
    } // end of while
    
    printf("%d new SP entries inserted.\n", i);    
}


void
DeleteSPInKernel(int argc, char *argv[])
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    ulong BytesReturned;
    
    if(argc == 0)
    {
        usage();
    }    

    if(!strcmp(argv[0], "all"))
    {
        Query.Index = 0;
    }
    else
    {
        Query.Index = atol(argv[0]);    
        if(Query.Index <= 0)
        {
            printf("Cannot delete SP Entry 0\n.");
            exit(1);
        }
    }
    
    Info.SPIndex = 0;
    
    if (!DeviceIoControl(Handle, IOCTL_IPV6_DELETE_SECURITY_POLICY,
                         &Query, sizeof(Query), &Info, sizeof(Info), 
                         &BytesReturned, NULL)) {
        
        printf("SP Entry %lu not deleted.\n", Query.Index);         
        exit(1);
    }
    
    // Indicates that this entry was not deleted.
    if(Info.NextSPIndex == 0)
    {
        printf("SP Entry %lu not deleted.\n", Info.SPIndex);
    }
    else
    {
        printf("SP Entry %lu deleted.\n", Info.SPIndex);
    }

    // Indicates the number of entries deleted.
    if(Info.SABundleIndex > 0)
    {    
        printf("%lu SP Entries deleted.\n", Info.SABundleIndex);   
    }
}

void
DeleteSAInKernel(int argc, char *argv[])
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;    
    ulong BytesReturned;

    if(argc == 0)
    {
        usage();
    }
    
    if(!strcmp(argv[0], "all"))
    {
        Query.Index = 0;
    }
    else
    {
        Query.Index = atol(argv[0]);    
        if(Query.Index <= 0)
        {
            printf("Cannot delete SA Entry 0\n.");
            exit(1);
        }
    }

    Info.SAIndex = 0;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_DELETE_SECURITY_ASSOCIATION,
                         &Query, sizeof(Query), &Info, sizeof(Info), 
                         &BytesReturned, NULL)) {
        
        printf("SA Entry %lu not deleted.\n", Query.Index);            
        exit(1);
    }


    // Indicates that this entry was not deleted.
    if(Info.NextSAIndex == 0)
    {
        printf("SA Entry %lu not deleted.\n", Info.SAIndex);
    }
    else
    {
        printf("SA Entry %lu deleted.\n", Info.SAIndex);
    }

    // Indicates the number of entries deleted.
    if(Info.SecPolicyIndex > 0)
    {    
        printf("%lu SA Entries deleted.\n", Info.SecPolicyIndex);   
    }
}

void
SetMobilitySecurity(int argc, char *argv[])
{
    IPV6_SET_MOBILITY_SECURITY Set, Result;
    ulong BytesReturned;

    if(argc < 1)
        Set.MobilitySecurity = MOBILITY_SECURITY_QUERY;
    else
    {
        if (!strcmp(argv[0], "off"))    
            Set.MobilitySecurity = MOBILITY_SECURITY_OFF;           
        else if (!strcmp(argv[0], "on"))    
            Set.MobilitySecurity = MOBILITY_SECURITY_ON;    
        else   
            usage();       
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_SET_MOBILITY_SECURITY,
                         &Set, sizeof(Set), &Result, sizeof(Result),
                         &BytesReturned, NULL)) 
    {   
        printf("\nCan't set mobility security\n");
        exit(1);
    }

    printf("\nMobility Security was %s.\n", 
        (Result.MobilitySecurity == MOBILITY_SECURITY_ON)?"on":"off");
}
