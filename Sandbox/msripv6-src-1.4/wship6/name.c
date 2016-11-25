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
// Forward & reverse name resolution library routines
// and related helper functions.
//
// Could be improved if necessary:
// QueryDNSforA could use WSALookupService instead of gethostbyname.
// gethostbyname will return success on some weird strings.
// Similarly, inet_addr is very loose (octal digits, etc).
// Could support multiple h_aliases.
// Could support hosts.txt file entries.
//

#include <windows.h>
#include <winsock2.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>

//
// Note that we use LocalAlloc/LocalFree instead of malloc/free
// to avoid introducing a dependency of wship6.dll on msvcrt.dll.
//

//
// REVIEW: Recent versions of svcguid.h (i.e. the one in the NT5 DDK,
// REVIEW: and the Windows 2000 beta 3 SDK) define SVCID_DNS.  But we
// REVIEW: want this to compile with older versions which don't define
// REVIEW: it, thus the following check.  Do away with this someday.
// 
#ifndef SVCID_DNS
#define SVCID_DNS(_RecordType) SVCID_TCP_RR(53, _RecordType)
#endif

#define L_A              0x1
#define L_AAAA           0x2
#define L_BOTH           0x3
#define L_AAAA_PREFERRED 0x6
#define L_A_PREFERRED    0x9  // Not used, but code would support it.

#define T_A     1
#define T_CNAME 5
#define T_AAAA  28
#define T_PTR   12
#define T_ALL   255

#define C_IN    1


static void
CreateV4MappedAddress(struct in6_addr *addr6, struct in_addr addr4);

static int
ParseDNSReply(ushort Needed, uchar *data, uint size,
              TDI_ADDRESS_IP6 **pAddrs, uint *pNumSlots, uint *pNumAddrs,
              char **pName);

static int
SortIPv6Addrs(TDI_ADDRESS_IP6 *Addrs, uint *pNumAddrs);

static int
QueryDNS(const char *name, uint LookupType,
         TDI_ADDRESS_IP6 **pAddrs, uint *pNumAddrs, char **pAlias);

//* getipnodebyname - get address corresponding to a node name.
//
//  Wrapper around inet6_addr, inet_addr, and QueryDNS.
//
struct hostent * WSAAPI
getipnodebyname(const char *name, int af, int flags, int *error_num)
{
    struct hostent *h = NULL;                   // Init for branches to Return.
    uint hsize;
    char *buffer;
    uint LookupType;
    TDI_ADDRESS_IP6 addrbuffer;
    TDI_ADDRESS_IP6 *Addrs = &addrbuffer;       // Init for branches to Return.
    uint NumAddrs = 1;
    char *Alias = NULL;                         // Init for branches to Return.
    uint addrlen, namelen, aliaslen;
    struct in_addr addr4;
    struct in6_addr addr6;
    int err;
    uint i;

    //
    // Determine what the caller has asked for.
    //
    if (af == AF_INET) {
        //
        // The caller has asked for IPv4 addresses only.
        //
        LookupType = L_A;
    } else if ((af == AF_INET6) && (flags & AI_ALL)) {
        //
        // The caller will accept both V4 and V6 addresses.
        // If V4 addresses are found, they are returned as
        // V4 Mapped V6 addresses.
        //
        LookupType = L_BOTH;
    } else if ((af == AF_INET6) && (flags == 0)) {
        //
        // The caller has asked for IPv6 addresses only.
        //
        LookupType = L_AAAA;
    } else if ((af == AF_INET6) && (flags & AI_V4MAPPED)) {
        //
        // The caller will accept both V4 and V6 addresses.
        // V4 addresses are returned only if no V6 addresses 
        // are found.  If they are returned, V4 addresses are
        // returned as V4 Mapped V6 addresses.
        //
        LookupType = L_AAAA_PREFERRED;
    } else {
        //
        // We don't support other combinations.
        //
        err = WSAEINVAL;
        goto Return;
    }

    if (name == NULL) {
        err = WSAEFAULT;
        goto Return;
    }

    //
    // First, see if the caller wants a simple conversion
    // from a numeric string address to a binary address.
    //
    if ((af == AF_INET6) && inet6_addr(name, &addr6)) {
        //
        // We were given an IPv6 hex address.
        //
        memset(Addrs, 0, sizeof *Addrs);
        memcpy(&Addrs->sin6_addr, &addr6, sizeof(struct in6_addr));
        err = 0;

    } else if (((af == AF_INET) ||
                ((af == AF_INET6) && (flags & (AI_ALL | AI_V4MAPPED)))) &&
               ((*(ulong *)&addr4 = inet_addr(name)) != INADDR_NONE)) {
        //
        // We were given a dotted decimal V4 address.
        //
        memset(Addrs, 0, sizeof *Addrs);
        CreateV4MappedAddress((struct in6_addr *)&Addrs->sin6_addr, addr4);
        err = 0;

    } else {
        //
        // We were not given a numeric literal address.
        //
        // Try querying DNS. There are error conditions
        // where we might have some addresses but not all
        // of them, or maybe can not sort them. In such cases,
        // we report an error instead of a partial result.
        //
        err = QueryDNS(name, LookupType, &Addrs, &NumAddrs, &Alias);
        if (err)
            goto Return;

        //
        // If we're returning more than one v6 address, sort them.
        // The sort is in-place but it may remove some addresses.
        // It will not reduce NumAddrs to zero.
        //
        if ((NumAddrs > 1) && (af == AF_INET6)) {
            err = SortIPv6Addrs(Addrs, &NumAddrs);
            if (err)
                goto Return;
        }
    }

    //
    // Calculate the size of the hostent structure.
    //
    if (af == AF_INET)
        addrlen = sizeof(struct in_addr);
    else
        addrlen = sizeof(struct in6_addr);
    namelen = strlen(name);
    hsize = sizeof(struct hostent) +
        namelen + 1 +                           // The name string.
        sizeof(char *) +                        // For h_aliases.
        (NumAddrs + 1) * sizeof(char *) +       // h_addr_list pointers.
        NumAddrs * addrlen;
    if (Alias != NULL) {
        //
        // If the Alias is the same as the name we started with,
        // just ignore it.
        //
        if (strcmp(name, Alias) == 0) {
            LocalFree(Alias);
            Alias = NULL;
        } else {
            aliaslen = strlen(Alias);
            hsize += aliaslen + 1 + sizeof(char *);
        }
    }

    //
    // Allocate the hostent structure.
    //
    h = (struct hostent *) LocalAlloc(0, hsize);
    if (h == NULL) {
        err = WSA_NOT_ENOUGH_MEMORY;
        goto Return;
    }

    h->h_addrtype = af;
    h->h_length = addrlen;

    //
    // Initialize h_addr_list and h_aliases.
    //
    buffer = (char *)(h + 1);
    h->h_addr_list = (char **)buffer;
    buffer += (NumAddrs + 1) * sizeof(char *);
    h->h_aliases = (char **)buffer;
    buffer += (1 + (Alias != NULL)) * sizeof(char *);

    //
    // Initialize h_addr_list pointers.
    //
    for (i = 0; i < NumAddrs; i++) {
        h->h_addr_list[i] = buffer;
        buffer += addrlen;
    }
    h->h_addr_list[i] = NULL;

    //
    // The name and the alias (if present) go at the end
    // because they don't have alignment requirements.
    // Note that if there is an alias, it gets returned
    // has h_name and the name parameter is returned in h_aliases.
    // This is because h_name is supposed to be the canonical name.
    //
    h->h_name = buffer;
    if (Alias != NULL) {
        strcpy(buffer, Alias);
        buffer += aliaslen + 1;
        h->h_aliases[0] = buffer;
        strcpy(buffer, name);
        h->h_aliases[1] = NULL;
    } else {
        strcpy(buffer, name);
        h->h_aliases[0] = NULL;
    }

    //
    // Copy the addresses. Note that getipnodebyname
    // returns in_addr/in6_addr, not sockaddrs,
    // so any scope-id information is lost.
    //
    for (i = 0; i < NumAddrs; i++) {
        if (af == AF_INET)
            *(struct in_addr *)h->h_addr_list[i] =
                *(struct in_addr *)&Addrs[i].sin6_addr[6];
        else
            memcpy(h->h_addr_list[i], &Addrs[i].sin6_addr,
                   sizeof(struct in6_addr));
    }

  Return:
    if (error_num != NULL)
        *error_num = err;
    if ((Addrs != NULL) && (Addrs != &addrbuffer))
        LocalFree(Addrs);
    if (Alias != NULL)
        LocalFree(Alias);
    return h;
}


//* SortIPv6Addrs
//
//  Sort an array of IPv6 addresses into the preferred order
//  for using the addresses as destinations.
//  This may also prune some addresses from the list.
//
static int
SortIPv6Addrs(TDI_ADDRESS_IP6 *Addrs, uint *pNumAddrs)
{
    HANDLE Handle;
    ulong AddrListBytes;
    int rc;

    //
    // Open a handle to the IPv6 device.
    //
    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
        0,      // access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,   // security attributes
        OPEN_EXISTING,
        0,      // flags & attributes
        NULL);  // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        //
        // We can not sort the list.
        //
        return WSASERVICE_NOT_FOUND;
    }

    AddrListBytes = *pNumAddrs * sizeof(TDI_ADDRESS_IP6);

    rc = DeviceIoControl(Handle, IOCTL_IPV6_SORT_DEST_ADDRS,
                         Addrs, AddrListBytes,
                         Addrs, AddrListBytes,
                         &AddrListBytes, NULL);
    CloseHandle(Handle);
    if (! rc) {
        //
        // We can not sort the list.
        //
        return GetLastError();
    }

    //
    // There might be fewer addresses now.
    //
    *pNumAddrs = AddrListBytes / sizeof(TDI_ADDRESS_IP6);
    return 0;
}


//* getipnodebyaddr - get node name corresponding to an address.
//
//  A wrapper around WSALookupServiceBegin/Next/End,
//  that is like gethostbyaddr but handles AF_INET6 addresses also.
//
struct hostent * WSAAPI
getipnodebyaddr(
    const void *Address,  // Address for which to look up corresponding name.
    int AddressLength,    // Length of Address, in bytes.
    int AddressFamily,    // Family address belongs to (i.e. AF_*).
    int *ReturnError)     // Where to return error (optional, NULL if none).
{
    uchar *LookupAddr = (uchar *)Address;
    int LookupAF = AddressFamily;
    char LookupString[80];
    GUID PtrGuid =  SVCID_DNS(T_PTR);
    HANDLE Resolver;
    char Buffer[sizeof(WSAQUERYSETA) + 2048];
    PWSAQUERYSETA query = (PWSAQUERYSETA) Buffer;
    uint Size;
    int Error;
    char *Name;
    uint NameLength;
    struct hostent *HostEntry;
    char *Marker;

    //
    // Prepare for error returns.
    //
    Name = NULL;
    query->lpszServiceInstanceName = NULL;
    HostEntry = NULL;

    //
    // Verify arguments are reasonable.
    //

    if (Address == NULL) {
        Error = WSAEFAULT;
        goto Return;
    }

    if (AddressFamily == AF_INET6) {
        if (AddressLength == 16) {
            // Check if this is a v4 mapped or v4 compatible address.
            if ((IN6_IS_ADDR_V4MAPPED((struct in6_addr *)Address)) ||
                (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)Address))) {
                // Skip over first 12 bytes of IPv6 address.
                LookupAddr = &LookupAddr[12];
                // Set address family to IPv4.
                LookupAF = AF_INET;
            }
        } else {
            // Bad length for IPv6 address.
            Error = WSAEFAULT;
            goto Return;
        }
    } else if (AddressFamily == AF_INET) {
        if (AddressLength != 4) {
            // Bad length for IPv4 address.
            Error = WSAEFAULT;
            goto Return;
        }
    } else {
        // Address family not supported.
        Error = WSAEAFNOSUPPORT;
        goto Return;
    }

    //
    // Prepare lookup string.
    //
    if (LookupAF == AF_INET6) {
        int Position, Loop;
        uchar Hex[] = "0123456789abcdef";

        //
        // Create reverse DNS name for IPv6.
        // The domain is "ip6.int".
        // Append a trailing "." to prevent domain suffix searching.
        //
        for (Position = 0, Loop = 15; Loop >= 0; Loop--) {
            LookupString[Position++] = Hex[LookupAddr[Loop] & 0x0f];
            LookupString[Position++] = '.';
            LookupString[Position++] = Hex[(LookupAddr[Loop] & 0xf0) >> 4];
            LookupString[Position++] = '.';
        }
        LookupString[Position] = 0;
        strcat(LookupString, "ip6.int.");

    } else {
        //
        // Create reverse DNS name for IPv4.
        // The domain is "in-addr.arpa".
        // Append a trailing "." to prevent domain suffix searching.
        //
        (void)sprintf(LookupString, "%u.%u.%u.%u.in-addr.arpa.",
                      LookupAddr[3], LookupAddr[2], LookupAddr[1],
                      LookupAddr[0]);
    }

    //
    // Format DNS query.
    // Build a Winsock T_PTR DNS query.
    //
    memset(query, 0, sizeof *query);
    query->dwSize = sizeof *query;
    query->lpszServiceInstanceName = LookupString;
    query->dwNameSpace = NS_DNS;
    query->lpServiceClassId = &PtrGuid;

    Error = WSALookupServiceBeginA(query, LUP_RETURN_NAME | LUP_RETURN_BLOB,
                                   &Resolver);
    if (Error) {
        Error = WSAGetLastError();
        if (Error == WSASERVICE_NOT_FOUND)
            Error = WSAHOST_NOT_FOUND;
        goto Return;
    }

    Size = sizeof Buffer;
    Error = WSALookupServiceNextA(Resolver, 0, &Size, query);
    if (Error) {
        Error = WSAGetLastError();
        if (Error == WSASERVICE_NOT_FOUND)
            Error = WSAHOST_NOT_FOUND;
        (void) WSALookupServiceEnd(Resolver);
        goto Return;
    }

    //
    // First check if we got parsed name back.
    //
    if (query->lpszServiceInstanceName != NULL) {
        Name = query->lpszServiceInstanceName;
    } else {
        //
        // Next check if got a raw DNS reply.
        //
        if (query->lpBlob != NULL) {
            //
            // Parse the DNS reply message, looking for a PTR record.
            //
            Error = ParseDNSReply(T_PTR,
                                  query->lpBlob->pBlobData,
                                  query->lpBlob->cbSize,
                                  NULL, NULL, NULL, &Name);
            if (Name == NULL) {
                if (Error == 0)
                    Error = WSAHOST_NOT_FOUND;
                (void) WSALookupServiceEnd(Resolver);
                goto Return;
            }

        } else {
            // 
            // Something wrong.
            //
            Error = WSANO_RECOVERY;
            (void) WSALookupServiceEnd(Resolver);
            goto Return;
        }
    }

    (void) WSALookupServiceEnd(Resolver);

    //
    // Allocate the hostent structure.  Also need space for the things it
    // points to - a name buffer and null terminator, an empty null terminated
    // array of aliases, and an array of addresses with one entry.
    //
    NameLength = strlen(Name) + sizeof(char);
    Size = sizeof(struct hostent) + NameLength + (3 * sizeof(char *)) +
        AddressLength;
    HostEntry = (struct hostent *) LocalAlloc(0, Size);
    if (HostEntry == NULL) {
        Error = WSA_NOT_ENOUGH_MEMORY;
        goto Return;
    }

    //
    // Populate the hostent structure we will return.
    //
    Marker = (char *)(HostEntry + 1);
    HostEntry->h_name = Marker;
    strcpy(HostEntry->h_name, Name);
    Marker += NameLength;

    HostEntry->h_aliases = (char **)Marker;
    *HostEntry->h_aliases = NULL;
    Marker += sizeof(char *);

    HostEntry->h_addrtype = AddressFamily;
    HostEntry->h_length = AddressLength;
    HostEntry->h_addr_list = (char **)Marker;
    *HostEntry->h_addr_list = Marker + (2 * sizeof(char *));
    memcpy(*HostEntry->h_addr_list, Address, AddressLength);
    Marker += sizeof(char *);
    *(char **)Marker = NULL;

  Return:
    if (ReturnError != NULL)
        *ReturnError = Error;
    if ((Name != NULL) && (Name != query->lpszServiceInstanceName))
        LocalFree(Name);
    return HostEntry;
}


//* freehostent
//
void WSAAPI
freehostent(struct hostent *Free)
{
    LocalFree(Free);
}


//* freeaddrinfo - Free an addrinfo structure (or chain of structures).
//
//  As specified in RFC 2553, Section 6.4.
//
void WSAAPI
freeaddrinfo(
    struct addrinfo *Free)  // Structure (chain) to free.
{
    struct addrinfo *Next;

    for (Next = Free; Next != NULL; Free = Next) {
        if (Free->ai_canonname != NULL)
            LocalFree(Free->ai_canonname);
        if (Free->ai_addr != NULL)
            LocalFree(Free->ai_addr);
        Next = Free->ai_next;
        LocalFree(Free);
    }
}


//* NewAddrInfo - Allocate an addrinfo structure and populate some fields.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Returns a partially filled-in addrinfo struct, or NULL if out of memory.
//
static struct addrinfo *
NewAddrInfo(
    int ProtocolFamily,       // Must be either PF_INET or PF_INET6.
    int SocketType,           // SOCK_*.  Can be wildcarded (zero).
    int Protocol,             // IPPROTO_*.  Can be wildcarded (zero).
    struct addrinfo ***Prev)  // In/out param for accessing previous ai_next.
{
    struct addrinfo *New;

    //
    // Allocate a new addrinfo structure.
    //
    New = LocalAlloc(0, sizeof(struct addrinfo));
    if (New == NULL)
        return NULL;

    //
    // Fill in the easy stuff.
    //
    New->ai_flags = 0;  // REVIEW: Spec doesn't say what this should be.
    New->ai_family = ProtocolFamily;
    New->ai_socktype = SocketType;
    New->ai_protocol = Protocol;
    if (ProtocolFamily == PF_INET) {
        New->ai_addrlen = sizeof(struct sockaddr_in);
    } else {
        New->ai_addrlen = sizeof(struct sockaddr_in6);
    }
    New->ai_canonname = NULL;
    New->ai_addr = LocalAlloc(0, New->ai_addrlen);
    if (New->ai_addr == NULL) {
        LocalFree(New);
        return NULL;
    }
    New->ai_next = NULL;

    //
    // Link this one onto the end of the chain.
    //
    **Prev = New;
    *Prev = &New->ai_next;

    return New;
}


//* LookupNode - Resolve a nodename and add any addresses found to the list.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Note that if AI_CANONNAME is requested, then **Prev should be NULL
//  because the canonical name should be returned in the first addrinfo
//  that the user gets.
//
//  Returns 0 on success, an EAI_* style error value otherwise.
//
static int
LookupNode(
    const char *NodeName,     // Name of node to resolve.
    int ProtocolFamily,       // Must zero, PF_INET, or PF_INET6.
    int SocketType,           // SOCK_*.  Can be wildcarded (zero).
    int Protocol,             // IPPROTO_*.  Can be wildcarded (zero).
    ushort ServicePort,       // Port number of service.
    int Flags,                // Flags.
    struct addrinfo ***Prev)  // In/out param for accessing previous ai_next.
{
    struct addrinfo *CurrentInfo;
    uint LookupType;
    TDI_ADDRESS_IP6 *Addrs;
    uint NumAddrs;
    char *Alias;
    int Error;
    uint i;

    switch (ProtocolFamily) {
    case 0:
        LookupType = L_A | L_AAAA;
        break;
    case PF_INET:
        LookupType = L_A;
        break;
    case PF_INET6:
        LookupType = L_AAAA;
        break;
    }

    Error = QueryDNS(NodeName, LookupType, &Addrs, &NumAddrs, &Alias);
    if (Error) {
        if (Error == WSANO_DATA) {
            Error = EAI_NODATA;
        } else if (Error == WSAHOST_NOT_FOUND) {
            Error = EAI_NONAME;
        } else {
            Error = EAI_FAIL;
        }

        goto Done;
    }

    if ((NumAddrs > 1) && (ProtocolFamily != PF_INET)) {
        Error = SortIPv6Addrs(Addrs, &NumAddrs);
        if (Error) {
            Error = EAI_FAIL;
            goto Done;
        }
    }

    //
    // Loop through all the addresses returned by QueryDNS,
    // allocating an addrinfo structure and filling in the address field
    // for each.
    //
    for (i = 0; i < NumAddrs; i++) {

        if (IN6_IS_ADDR_V4MAPPED((struct in6_addr *)&Addrs[i].sin6_addr))
            ProtocolFamily = PF_INET;
        else
            ProtocolFamily = PF_INET6;

        CurrentInfo = NewAddrInfo(ProtocolFamily, SocketType, Protocol,
                                  Prev);
        if (CurrentInfo == NULL) {
            Error = EAI_MEMORY;
            goto Done;
        }

        if (ProtocolFamily == PF_INET6) {
            //
            // We're returning IPv6 addresses.
            //
            struct sockaddr_in6 *sin6;

            sin6 = (struct sockaddr_in6 *)CurrentInfo->ai_addr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = ServicePort;
            sin6->sin6_flowinfo = 0;
            sin6->sin6_scope_id = Addrs[i].sin6_scope_id;
            memcpy(&sin6->sin6_addr, &Addrs[i].sin6_addr,
                   sizeof sin6->sin6_addr);

        } else {
            //
            // We're returning IPv4 addresses.
            //
            struct sockaddr_in *sin;

            sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_port = ServicePort;
            memcpy(&sin->sin_addr, &Addrs[i].sin6_addr[6],
                   sizeof sin->sin_addr);
        }

        //
        // We fill in the ai_canonname field in the first addrinfo
        // structure that we return if we've been asked to do so.
        //
        if (Flags & AI_CANONNAME) {
            if (Alias != NULL) {
                //
                // The alias that QueryDNS gave us is
                // the canonical name.
                //
                CurrentInfo->ai_canonname = Alias;
                Alias = NULL;
            } else {
                int NameLength;

                NameLength = strlen(NodeName) + 1;
                CurrentInfo->ai_canonname = LocalAlloc(0, NameLength);
                if (CurrentInfo->ai_canonname == NULL) {
                    Error = EAI_MEMORY;
                    goto Done;
                }
                memcpy(CurrentInfo->ai_canonname, NodeName, NameLength);
            }

            // Turn off flag so we only do this once.
            Flags &= ~AI_CANONNAME;
        }
    }

  Done:
    if (Addrs != NULL)
        LocalFree(Addrs);
    if (Alias != NULL)
        LocalFree(Alias);

    return Error;
}


//* getaddrinfo - Protocol-independent name-to-address translation.
//
//  As specified in RFC 2553, Section 6.4.
//
//  Returns zero if successful, an EAI_* error code if not.
//
int WSAAPI
getaddrinfo(
    const char *NodeName,          // Node name to lookup.
    const char *ServiceName,       // Service name to lookup.
    const struct addrinfo *Hints,  // Hints about how to process request.
    struct addrinfo **Result)      // Where to return result.
{
    struct addrinfo *CurrentInfo, **Next;
    int ProtocolId = 0;
    ushort ProtocolFamily = PF_UNSPEC;
    ushort ServicePort = 0;
    int SocketType = 0;
    int Flags = 0;
    int Error;
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;

    //
    // In case we have to bail early, make it clear to our caller
    // that we haven't allocated an addrinfo structure.
    //
    *Result = NULL;
    Next = Result;

    //
    // Both the node name and the service name can't be NULL.
    //
    if ((NodeName == NULL) && (ServiceName == NULL))
        return EAI_NONAME;

    //
    // Validate hints argument.
    //
    if (Hints != NULL) {
        //
        // All members other than ai_flags, ai_family, ai_socktype
        // and ai_protocol must be zero or a null pointer.
        //
        if ((Hints->ai_addrlen != 0) || (Hints->ai_canonname != NULL) ||
            (Hints->ai_addr != NULL) || (Hints->ai_next != NULL)) {
            //
            // REVIEW: Not clear what error to return here.
            //
            return EAI_FAIL;
        }

        //
        // The spec has the "bad flags" error code, so presumably we should
        // check something here.  Insisting that there aren't any unspecified
        // flags set would break forward compatibility, however.  So we just
        // check for non-sensical combinations.
        //
        Flags = Hints->ai_flags;
        if ((Flags & AI_CANONNAME) && (Flags & AI_NUMERICHOST)) {
            //
            // AI_NUMERICHOST is supposed to insure that no name resolution
            // service is called, so there is no way for us to come up with
            // a canonical name given a numeric node name.
            //
            return EAI_BADFLAGS;
        }

        //
        // We only support a limited number of protocol families.
        //
        ProtocolFamily = Hints->ai_family;
        if ((ProtocolFamily != PF_UNSPEC) && (ProtocolFamily != PF_INET6)
            && (ProtocolFamily != PF_INET))
            return EAI_FAMILY;

        //
        // We only support a limited number of socket types.
        //
        SocketType = Hints->ai_socktype;
        if ((SocketType != 0) && (SocketType != SOCK_STREAM) &&
            (SocketType != SOCK_DGRAM))
            return EAI_SOCKTYPE;

        //
        // REVIEW: What if ai_socktype and ai_protocol are at odds?
        // REVIEW: Should we enforce the mapping triples here?
        //
        ProtocolId = Hints->ai_protocol;
    }

    //
    // Lookup service first (if we're given one) as we'll need the
    // corresponding port number for all the address structures we return.
    //
    if (ServiceName != NULL) {

        //
        // The ServiceName string can be either a service name
        // or a decimal port number.  Check for the latter first.
        //
        ServicePort = htons((ushort)atoi(ServiceName));

        if (ServicePort == 0) {
            struct servent *ServiceEntry;

            //
            // We have to look up the service name.  Since it may be
            // socktype/protocol specific, we have to do multiple lookups
            // unless our caller limits us to one.
            //
            // Spec doesn't say whether we should use the Hints' ai_protocol
            // or ai_socktype when doing this lookup.  But the latter is more
            // commonly used in practice, and is what the spec implies anyhow.
            //

            if (SocketType != SOCK_DGRAM) {
                //
                // See if this service exists for TCP.
                //
                ServiceEntry = getservbyname(ServiceName, "tcp");
                if (ServiceEntry == NULL) {
                    Error = WSAGetLastError();
                    if (Error == WSANO_DATA) {
                        //
                        // Unknown service for this protocol.
                        // Bail if we're restricted to TCP.
                        //
                        if (SocketType == SOCK_STREAM)
                            return EAI_SERVICE;  // REVIEW: or EAI_NONAME?

                        // Otherwise we'll try UDP below...

                    } else {
                        // Some other failure.
                        return EAI_FAIL;
                    }
                } else {
                    //
                    // Service is known for TCP.
                    //
                    ServicePort = ServiceEntry->s_port;
                }
            }

            if (SocketType != SOCK_STREAM) {
                //
                // See if this service exists for UDP.
                //
                ServiceEntry = getservbyname(ServiceName, "udp");
                if (ServiceEntry == NULL) {
                    Error = WSAGetLastError();
                    if (Error == WSANO_DATA) {
                        //
                        // Unknown service for this protocol.
                        // Bail if we're restricted to UDP or if
                        // the TCP lookup also failed.
                        //
                        if (SocketType == SOCK_DGRAM)
                            return EAI_SERVICE;  // REVIEW: or EAI_NONAME?
                        if (ServicePort == 0)
                            return EAI_NONAME;  // Both lookups failed.
                        SocketType = SOCK_STREAM;
                    } else {
                        // Some other failure.
                        return EAI_FAIL;
                    }
                } else {
                    //
                    // Service is known for UDP.
                    //
                    if (ServicePort == 0) {
                        //
                        // If we made a TCP lookup, it failed.
                        // Limit ourselves to UDP.
                        //
                        SocketType = SOCK_DGRAM;
                        ServicePort = ServiceEntry->s_port;
                    } else {
                        if (ServicePort != ServiceEntry->s_port) {
                            //
                            // Port number is different for TCP and UDP,
                            // and our caller will accept either socket type.
                            // Arbitrarily limit ourselves to TCP.
                            //
                            SocketType = SOCK_STREAM;
                        }
                    }
                }
            }
        }
    }

    //
    // If we weren't given a node name, return the service info with
    // the loopback or wildcard address (which one depends upon the
    // AI_PASSIVE flag).  Note that if our caller didn't specify an
    // protocol family, we'll return both a V6 and a V4 address.
    //
    if (NodeName == NULL) {
        //
        // What address to return depends upon the protocol family and
        // whether or not the AI_PASSIVE flag is set.
        //
        if ((ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET6)) {
            //
            // Return an IPv6 address.
            //
            CurrentInfo = NewAddrInfo(PF_INET6, SocketType, ProtocolId, &Next);
            if (CurrentInfo == NULL) {
                Error = EAI_MEMORY;
                goto Bail;
            }
            sin6 = (struct sockaddr_in6 *)CurrentInfo->ai_addr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = ServicePort;
            sin6->sin6_flowinfo = 0;
            sin6->sin6_scope_id = 0;
            if (Flags & AI_PASSIVE) {
                //
                // Assume user wants to accept on any address.
                //
                sin6->sin6_addr = in6addr_any;
            } else {
                //
                // Only address we can return is loopback.
                //
                sin6->sin6_addr = in6addr_loopback;
            }
        }

        if ((ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET)) {
            //
            // Return an IPv4 address.
            //
            CurrentInfo = NewAddrInfo(PF_INET, SocketType, ProtocolId, &Next);
            if (CurrentInfo == NULL) {
                Error = EAI_MEMORY;
                goto Bail;
            }
            sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_port = ServicePort;
            if (Flags & AI_PASSIVE) {
                //
                // Assume user wants to accept on any address.
                //
                sin->sin_addr.s_addr = htonl(INADDR_ANY);
            } else {
                //
                // Only address we can return is loopback.
                //
                sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
        }

        return 0;  // Success!
    }

    //
    // We have a node name (either alpha or numeric) we need to look up.
    //

    //
    // First, see if this is a numeric string address that we can
    // just convert to a binary address.
    //
    // We shouldn't have to set the sa_family field prior to calling
    // WSAStringToAddress, but it appears we do.
    //
    // We arbitrarily check for IPv6 address format first.
    //
    if ((ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET6)) {
        struct sockaddr_in6 TempSockAddr;
        int AddressLength;

        TempSockAddr.sin6_family = AF_INET6;
        AddressLength = sizeof TempSockAddr;
        if (WSAStringToAddress((char *)NodeName, AF_INET6, NULL,
                               (struct sockaddr *)&TempSockAddr,
                               &AddressLength) != SOCKET_ERROR) {
            //
            // Conversion from IPv6 numeric string to binary address
            // was sucessfull.  Create an addrinfo structure to hold it,
            // and return it to the user.
            //
            CurrentInfo = NewAddrInfo(PF_INET6, SocketType, ProtocolId, &Next);
            if (CurrentInfo == NULL) {
                Error = EAI_MEMORY;
                goto Bail;
            }
            memcpy(CurrentInfo->ai_addr, &TempSockAddr, AddressLength);
            sin6 = (struct sockaddr_in6 *)CurrentInfo->ai_addr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = ServicePort;
            sin6->sin6_flowinfo = 0;

            return 0;  // Success!
        }
    }

    //
    // BUGBUG: The IPv4 numeric address parser that WSAStringToAddress uses
    // BUGBUG: is more liberal in what it will accept (e.g. octal values)
    // BUGBUG: than what the spec says getaddrinfo should accept.
    //
    if ((ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET)) {
        struct sockaddr_in TempSockAddr;
        int AddressLength;

        TempSockAddr.sin_family = AF_INET;
        AddressLength = sizeof TempSockAddr;
        if (WSAStringToAddress((char *)NodeName, AF_INET, NULL,
                               (struct sockaddr *)&TempSockAddr,
                               &AddressLength) != SOCKET_ERROR) {
            //
            // Conversion from IPv4 numeric string to binary address
            // was sucessfull.  Create an addrinfo structure to hold it,
            // and return it to the user.
            //
            CurrentInfo = NewAddrInfo(PF_INET, SocketType, ProtocolId, &Next);
            if (CurrentInfo == NULL) {
                Error = EAI_MEMORY;
                goto Bail;
            }
            memcpy(CurrentInfo->ai_addr, &TempSockAddr, AddressLength);
            sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_port = ServicePort;

            return 0;  // Success!
        }
    }

    //
    // It's not a numeric string address.  If our caller only wants us
    // to convert numeric addresses, give up now.
    //
    if (Flags & AI_NUMERICHOST) {
        Error = EAI_NONAME;
        goto Bail;
    }

    //
    // Since it's non-numeric, we have to do a regular node name lookup.
    //
    Error = LookupNode(NodeName, ProtocolFamily, SocketType, ProtocolId,
                       ServicePort, Flags, &Next);
    if (Error != 0)
        goto Bail;

    return 0;  // Success!

  Bail:
    if (*Result != NULL) {
        freeaddrinfo(*Result);
        *Result = NULL;
    }
    return Error;
}


//* getnameinfo - Protocol-independent address-to-name translation.
//
//  As specified in RFC 2553, Section 6.5.
//  REVIEW: Spec doesn't say what error code space to use.
//
int WSAAPI
getnameinfo(
    const struct sockaddr *SocketAddress,  // Socket address to translate.
    socklen_t SocketAddressLength,         // Length of above socket address.
    char *NodeName,                        // Where to return the node name.
    size_t NodeBufferSize,                 // Size of above buffer.
    char *ServiceName,                     // Where to return the service name.
    size_t ServiceBufferSize,              // Size of above buffer.
    int Flags)                             // Flags of type NI_*.
{
    char *p, *q;
    int Space;

    //
    // Sanity check SocketAddress and SocketAddressLength.
    //
    if ((SocketAddress == NULL) ||
        (SocketAddressLength < sizeof *SocketAddress))
        return WSAEFAULT;

    switch (SocketAddress->sa_family) {
    case AF_INET:
        if (SocketAddressLength != sizeof(struct sockaddr_in))
            return WSAEFAULT;
        break;
    case AF_INET6:
        if (SocketAddressLength != sizeof(struct sockaddr_in6))
            return WSAEFAULT;
        break;
    default:
        return WSAEAFNOSUPPORT;
    }

    //
    // Translate the address to a node name (if requested).
    //
    if (NodeName != NULL) {
        void *Address;
        int AddressLength;

        switch (SocketAddress->sa_family) {
        case AF_INET:
            //
            // Caller gave us an IPv4 address.
            //
            Address = &((struct sockaddr_in *)SocketAddress)->sin_addr;
            AddressLength = sizeof(struct in_addr);
            break;

        case AF_INET6:
            //
            // Caller gave us an IPv6 address.
            //
            Address = &((struct sockaddr_in6 *)SocketAddress)->sin6_addr;
            AddressLength = sizeof(struct in6_addr);
            break;
        }

        if (Flags & NI_NUMERICHOST) {
            //
            // Return numeric form of the address.
            // 
            struct sockaddr_storage TempSockAddr;  // Guaranteed big enough.

          ReturnNumeric:
            //
            // We need to copy our SocketAddress so we can zero the
            // port field before calling WSAAdressToString.
            //
            memcpy(&TempSockAddr, SocketAddress, SocketAddressLength);
            if (SocketAddress->sa_family == AF_INET) {
                ((struct sockaddr_in *)&TempSockAddr)->sin_port = 0;
            } else {
                ((struct sockaddr_in6 *)&TempSockAddr)->sin6_port = 0;
            }

            if (WSAAddressToString((struct sockaddr *)&TempSockAddr,
                                   SocketAddressLength, NULL, NodeName,
                                   &NodeBufferSize) == SOCKET_ERROR) {
                return WSAGetLastError();
            }

        } else {
            //
            // Return node name corresponding to address.
            //
            struct hostent *HostEntry;
            int Error;

            HostEntry = getipnodebyaddr(Address, AddressLength,
                                        SocketAddress->sa_family, &Error);
            if (HostEntry == NULL) {
                //
                // DNS lookup failed.
                //
                if (Flags & NI_NAMEREQD)
                    return Error;
                else
                    goto ReturnNumeric;
            }

            //
            // Lookup successful.  Put result in 'NodeName'.
            // Stop copying at a "." if NI_NOFQDN is specified.
            //
            Space = NodeBufferSize;
            for (p = NodeName, q = HostEntry->h_name; *q != '\0' ; ) {
                if (Space-- == 0) {
                    freehostent(HostEntry);
                    return WSAEFAULT;
                }
                if ((*q == '.') && (Flags & NI_NOFQDN))
                    break;
                *p++ = *q++;
            }
            if (Space == 0) {
                freehostent(HostEntry);
                return WSAEFAULT;
            }
            *p = '\0';
            freehostent(HostEntry);
        }
    }

    //
    // Translate the port number to a service name (if requested).
    //
    if (ServiceName != NULL) {
        ushort Port;

        switch (SocketAddress->sa_family) {
        case AF_INET:
            //
            // Caller gave us an IPv4 address.
            //
            Port = ((struct sockaddr_in *)SocketAddress)->sin_port;
            break;

        case AF_INET6:
            //
            // Caller gave us an IPv6 address.
            //
            Port = ((struct sockaddr_in6 *)SocketAddress)->sin6_port;
            break;
        }

        if (Flags & NI_NUMERICSERV) {
            //
            // Return numeric form of the port number.
            //
            char Temp[6];
            
            (void)sprintf(Temp, "%u", ntohs(Port));
            Space = ServiceBufferSize - 1;
            for (p = ServiceName, q = Temp; *q != '\0' ; ) {
                if (Space-- == 0) {
                    return WSAEFAULT;
                }
                *p++ = *q++;
            }
            *p = '\0';

        } else {
            //
            // Return service name corresponding to the port number.
            //
            struct servent *ServiceEntry;

            ServiceEntry = getservbyport(Port,
                                         (Flags & NI_DGRAM) ? "udp" : "tcp");
            if (ServiceEntry == NULL)
                return WSAGetLastError();

            Space = ServiceBufferSize;
            for (p = ServiceName, q = ServiceEntry->s_name; *q != '\0' ; ) {
                if (Space-- == 0)
                    return WSAEFAULT;
                *p++ = *q++;
            }
            if (Space == 0)
                return WSAEFAULT;
            *p = '\0';
        }
    }

    return 0;
}


//* CreateV4MappedAddress
//
//  Convert a v4 address to a v4-mapped v6 address.
//
static void
CreateV4MappedAddress(struct in6_addr *addr6, struct in_addr addr4)
{
    static char V4MappedAddrPrefix[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (char)0xff, (char)0xff };

    memcpy(addr6, V4MappedAddrPrefix, sizeof V4MappedAddrPrefix);
    * (struct in_addr *) ((char *)addr6 + sizeof V4MappedAddrPrefix) = addr4;
}


//* SaveAddress
//
//  Save an address into our array of addresses,
//  possibly growing the array.
//
//  Returns FALSE on failure. (Couldn't grow array.)
//
static int
SaveAddress(struct in6_addr *NewAddr,
            TDI_ADDRESS_IP6 **pAddrs, uint *pNumSlots, uint *pNumAddrs)
{
    TDI_ADDRESS_IP6 *addr6;

    if (*pNumSlots == *pNumAddrs) {
        TDI_ADDRESS_IP6 *NewAddrs;
        uint NewSlots;

        //
        // We must grow the array.
        //
        if (*pAddrs == NULL) {
            NewSlots = 1;
            NewAddrs = LocalAlloc(0, sizeof **pAddrs);
        }
        else {
            NewSlots = 2 * *pNumSlots;
            NewAddrs = LocalReAlloc(*pAddrs, NewSlots * sizeof **pAddrs,
                                    LMEM_MOVEABLE);
        }
        if (NewAddrs == NULL)
            return FALSE;
        *pAddrs = NewAddrs;
        *pNumSlots = NewSlots;
    }

    addr6 = &(*pAddrs)[(*pNumAddrs)++];
    memset(addr6, 0, sizeof *addr6);
    memcpy(&addr6->sin6_addr, NewAddr, sizeof *NewAddr);
    return TRUE;
}


//* DomainNameLength
//
//  Determine the length of a domain name (a sequence of labels)
//  in a DNS message. Zero return means error.
//  On success, the length includes one for a null terminator.
//
static uint
DomainNameLength(uchar *start, uint total, uchar *data, uint size)
{
    uint NameLength = 0;

    for (;;) {
        uchar length;

        if (size < sizeof length)
            return 0;
        length = *data;

        if ((length & 0xc0) == 0xc0) {
            ushort pointer;
            //
            // This is a pointer to labels elsewhere.
            //
            if (size < sizeof pointer)
                return FALSE;
            pointer = ((length & 0x3f) << 8) | data[1];

            if (pointer > total)
                return 0;

            data = start + pointer;
            size = total - pointer;
            continue;
        }

        data += sizeof length;
        size -= sizeof length;

        //
        // Zero-length label terminates the name.
        //
        if (length == 0)
            break;

        if (size < length)
            return 0;

        NameLength += length + 1;
        data += length;
        size -= length;

        //
        // Prevent infinite loops with an upper-bound.
        // Note that each label adds at least one to the length.
        //
        if (NameLength > NI_MAXHOST)
            return 0;
    }

    return NameLength;
}


//* CopyDomainName
//
//  Copy a domain name (a sequence of labels) from a DNS message
//  to a null-terminated C string.
// 
//  The DNS message must be syntactically correct.
//
static void
CopyDomainName(char *Name, uchar *start, uchar *data)
{
    for (;;) {
        uchar length = *data;

        if ((length & 0xc0) == 0xc0) {
            ushort pointer;
            //
            // This is a pointer to labels elsewhere.
            //
            pointer = ((length & 0x3f) << 8) | data[1];
            data = start + pointer;
            continue;
        }

        data += sizeof length;

        if (length == 0) {
            //
            // Backup and overwrite the last '.' with a null.
            //
            Name[-1] = '\0';
            break;
        }

        memcpy(Name, data, length);
        Name[length] = '.';
        Name += length + 1;
        data += length;
    }
}


//* SaveDomainName
//
//  Copy a domain name (a sequence of labels) from a DNS message
//  to a null-terminated C string.
//
//  Return values are WSA error codes, 0 means success.
//
static int
SaveDomainName(char **pName, uchar *start, uint total, uchar *data, uint size)
{
    uint NameLength;
    char *Name;

    NameLength = DomainNameLength(start, total, data, size);
    if (NameLength == 0)
        return WSANO_RECOVERY;

    Name = *pName;
    if (Name == NULL)
        Name = LocalAlloc(0, NameLength);
    else
        Name = LocalReAlloc(Name, NameLength, LMEM_MOVEABLE);
    if (Name == NULL)
        return WSA_NOT_ENOUGH_MEMORY;

    *pName = Name;
    CopyDomainName(Name, start, data);
    return 0;
}


//* ParseDNSReply
//
//  This is a bit complicated so it gets its own helper function.
//
//  Needed indicates the desired type: T_A, T_AAAA, T_CNAME, or T_PTR.
//  For T_A and T_AAAA, pAddrs, pNumSlots, and pNumAddrs are used.
//  For T_CNAME and T_PTR, pName is used (only last name found is returned).
//
//  Return values are WSA error codes, 0 means successful parse
//  but that does not mean anything was found.
//
static int
ParseDNSReply(ushort Needed, uchar *data, uint size,
              TDI_ADDRESS_IP6 **pAddrs, uint *pNumSlots, uint *pNumAddrs,
              char **pName)
{
    ushort id, codes, qdcount, ancount, nscount, arcount;
    uchar *start = data;
    uint total = size;
    int err;

    //
    // The DNS message starts with six two-byte fields.
    //
    if (size < sizeof(ushort) * 6)
        return WSANO_RECOVERY;

    id = ntohs(((ushort * UNALIGNED)data)[0]);
    codes = ntohs(((ushort * UNALIGNED)data)[1]);
    qdcount = ntohs(((ushort * UNALIGNED)data)[2]);
    ancount = ntohs(((ushort * UNALIGNED)data)[3]);
    nscount = ntohs(((ushort * UNALIGNED)data)[4]);
    arcount = ntohs(((ushort * UNALIGNED)data)[5]);

    data += sizeof(ushort) * 6;
    size -= sizeof(ushort) * 6;

    //
    // Skip over the question records.
    // Each question record has a QNAME, a QTYPE, and a QCLASS.
    // The QNAME is a sequence of labels, where each label
    // has a length byte. It is terminated by a zero-length label.
    // The QTYPE and QCLASS are two bytes each.
    //
    while (qdcount > 0) {

        //
        // Skip over the QNAME labels.
        //
        for (;;) {
            uchar length;

            if (size < sizeof length)
                return WSANO_RECOVERY;
            length = *data;
            if ((length & 0xc0) == 0xc0) {
                //
                // This is a pointer to labels elsewhere.
                //
                if (size < sizeof(ushort))
                    return WSANO_RECOVERY;
                data += sizeof(ushort);
                size -= sizeof(ushort);
                break;
            }

            data += sizeof length;
            size -= sizeof length;

            if (length == 0)
                break;

            if (size < length)
                return WSANO_RECOVERY;
            data += length;
            size -= length;
        }

        //
        // Skip over QTYPE and QCLASS.
        //
        if (size < sizeof(ushort) * 2)
            return WSANO_RECOVERY;
        data += sizeof(ushort) * 2;
        size -= sizeof(ushort) * 2;

        qdcount--;
    }

    //
    // Examine the answer records, looking for A/AAAA/CNAME records.
    // Each answer record has a name, followed by several values:
    // TYPE, CLASS, TTL, RDLENGTH, and then RDLENGTH bytes of data.
    // TYPE, CLASS, and RDLENGTH are two bytes; TTL is four bytes.
    //
    while (ancount > 0) {
        ushort type, class, rdlength;
        uint ttl;

        //
        // Skip over the name.
        //
        for (;;) {
            uchar length;

            if (size < sizeof length)
                return WSANO_RECOVERY;
            length = *data;
            if ((length & 0xc0) == 0xc0) {
                //
                // This is a pointer to labels elsewhere.
                //
                if (size < sizeof(ushort))
                    return WSANO_RECOVERY;
                data += sizeof(ushort);
                size -= sizeof(ushort);
                break;
            }
            data += sizeof length;
            size -= sizeof length;

            if (length == 0)
                break;

            if (size < length)
                return WSANO_RECOVERY;
            data += length;
            size -= length;
        }

        if (size < sizeof(ushort) * 3 + sizeof(uint))
            return WSANO_RECOVERY;
        type = ntohs(((ushort * UNALIGNED)data)[0]);
        class = ntohs(((ushort * UNALIGNED)data)[1]);
        ttl = ntohl(((uint * UNALIGNED)data)[1]);
        rdlength = ntohs(((ushort * UNALIGNED)data)[4]);

        data += sizeof(ushort) * 3 + sizeof(uint);
        size -= sizeof(ushort) * 3 + sizeof(uint);

        if (size < rdlength)
            return WSANO_RECOVERY;

        //
        // Is this the answer record type that we want?
        //
        if ((type == Needed) && (class == C_IN)) {
            struct in6_addr addr6;

            switch (type) {
            case T_A:
                if (rdlength != sizeof(struct in_addr))
                    return WSANO_RECOVERY;

                //
                // We have found a valid A record - save as v4-mapped.
                //
                CreateV4MappedAddress(&addr6, *(struct in_addr *)data);
                if (! SaveAddress(&addr6, pAddrs, pNumSlots, pNumAddrs))
                    return WSA_NOT_ENOUGH_MEMORY;
                break;

            case T_AAAA:
                if (rdlength != sizeof(struct in6_addr))
                    return WSANO_RECOVERY;

                //
                // We have found a valid AAAA record.
                //
                if (! SaveAddress((struct in6_addr *)data,
                                  pAddrs, pNumSlots, pNumAddrs))
                    return WSA_NOT_ENOUGH_MEMORY;
                break;

            case T_CNAME:
            case T_PTR:
                //
                // We have found a valid CNAME or PTR record.
                // Save the name.
                //
                err = SaveDomainName(pName, start, total, data, rdlength);
                if (err)
                    return err;
                break;
            }
        }

        data += rdlength;
        size -= rdlength;
        ancount--;
    }

    return 0;
}


//* QueryDNSforAAAA
//
//  Helper routine for getipnodebyname and getaddrinfo
//  that performs name resolution by querying the DNS
//  for AAAA records.
//
static int
QueryDNSforAAAA(const char *name,
                TDI_ADDRESS_IP6 **pAddrs, uint *pNumSlots, uint *pNumAddrs,
                char **pAlias)
{
    static GUID QuadAGuid = SVCID_DNS(T_AAAA);
    char buffer[sizeof(WSAQUERYSETA) + 2048];
    uint Size;
    PWSAQUERYSETA query = (PWSAQUERYSETA) buffer;
    HANDLE handle;
    int err;

    //
    // Build a Winsock T_AAAA DNS query.
    //
    memset(query, 0, sizeof *query);
    query->dwSize = sizeof *query;
    query->lpszServiceInstanceName = (char *)name;
    query->dwNameSpace = NS_DNS;
    query->lpServiceClassId = &QuadAGuid;

    //
    // Initiate the DNS query, asking for both addresses and
    // a raw DNS reply. On NT5, we'll get back addresses.
    // On NT4, we'll only get a raw DNS reply. (The resolver
    // does not understand AAAA records.) This means that on
    // NT4 there is no caching. OTOH, NT5 has a bug
    // that causes WSALookupServiceNextA to return a NetBios
    // address instead of any IPv6 addresses when you lookup
    // an unqualified machine name on that machine. That is,
    // lookup of "foobar" on foobar does the wrong thing.
    // Finally, we also ask for the fully qualified name.
    //
    err = WSALookupServiceBeginA(query,
                                 LUP_RETURN_ADDR |
                                 LUP_RETURN_BLOB |
                                 LUP_RETURN_NAME,
                                 &handle);
    if (err) {
        err = WSAGetLastError();
        if ((err == 0) || (err == WSASERVICE_NOT_FOUND))
            err = WSAHOST_NOT_FOUND;
        return err;
    }

    //
    // Loop until we get all of the queryset back in answer to our query.
    //
    // REVIEW: It's not clear to me that this is implemented
    // REVIEW: right, shouldn't we be checking for a WSAEFAULT and
    // REVIEW: then either increase the queryset buffer size or
    // REVIEW: set LUP_FLUSHPREVIOUS to move on for the next call?
    // REVIEW: Right now we just bail in that case.
    //
    for (;;) {
        Size = sizeof buffer;
        err = WSALookupServiceNextA(handle, 0, &Size, query);
        if (err) {
            //
            // We ignore the specific error code and
            // treat all failures as indicating that there
            // are no more addresses associated with the name.
            //
            break;
        }

        if (query->dwNumberOfCsAddrs != 0) {
            uint i;

            //
            // We got back parsed addresses.
            //
            for (i = 0; i < query->dwNumberOfCsAddrs; i++) {
                CSADDR_INFO *CsAddr = &query->lpcsaBuffer[i];

                //
                // We check iSockaddrLength against 24 instead of
                // sizeof(sockaddr_in6). Some versions of the NT5 resolver
                // are built with a sockaddr_in6 that is smaller than
                // the one we use here, but sin6_addr is in the same place
                // so it is ok.
                //
                if ((CsAddr->iProtocol == AF_INET6) &&
                    (CsAddr->RemoteAddr.iSockaddrLength >= 24) &&
                    (CsAddr->RemoteAddr.lpSockaddr->sa_family == AF_INET6)) {
                    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)
                        CsAddr->RemoteAddr.lpSockaddr;

                    //
                    // Again, note that sin6 may be pointing to a
                    // structure which is actually not our sockaddr_in6!
                    //
                    if (! SaveAddress(&sin6->sin6_addr,
                                      pAddrs, pNumSlots, pNumAddrs)) {
                        (void) WSALookupServiceEnd(handle);
                        return WSA_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }
        else if (query->lpBlob != NULL) {
            //
            // We got back a raw DNS reply message.
            // Parse first for T_AAAA and then for T_CNAME.
            // These functions only return errors
            // if the DNS reply is malformed.
            //
            if ((err = ParseDNSReply(T_AAAA,
                                     query->lpBlob->pBlobData,
                                     query->lpBlob->cbSize,
                                     pAddrs, pNumSlots, pNumAddrs, NULL)) ||
                (err = ParseDNSReply(T_CNAME,
                                     query->lpBlob->pBlobData,
                                     query->lpBlob->cbSize,
                                     NULL, NULL, NULL, pAlias))) {
                (void) WSALookupServiceEnd(handle);
                return err;
            }
        }
        else {
            //
            // Otherwise there's something wrong with WSALookupServiceNextA.
            // But to be more robust, just keep going.
            //
        }

        //
        // Pick up the canonical name. Note that this will override
        // any alias from ParseDNSReply.
        //
        if (query->lpszServiceInstanceName != NULL) {
            uint length;
            char *Alias;

            length = strlen(query->lpszServiceInstanceName) + 1;
            Alias = *pAlias;
            if (Alias == NULL)
                Alias = LocalAlloc(0, length);
            else
                Alias = LocalReAlloc(Alias, length + 1, LMEM_MOVEABLE);
            if (Alias == NULL) {
                (void) WSALookupServiceEnd(handle);
                return WSA_NOT_ENOUGH_MEMORY;
            }

            memcpy(Alias, query->lpszServiceInstanceName, length);
            *pAlias = Alias;
        }
    }

    (void) WSALookupServiceEnd(handle);
    return 0;
}

//* QueryDNSforA
//
//  Helper routine for getipnodebyname and getaddrinfo
//  that performs name resolution by querying the DNS
//  for A records.
//
static int
QueryDNSforA(const char *name,
             TDI_ADDRESS_IP6 **pAddrs, uint *pNumSlots, uint *pNumAddrs,
             char **pAlias)
{
    struct hostent *hA;
    char **addrs;
    uint length;
    char *Alias;

    hA = gethostbyname(name);
    if (hA != NULL) {
        if ((hA->h_addrtype == AF_INET) &&
            (hA->h_length == sizeof(struct in_addr))) {

            for (addrs = hA->h_addr_list; *addrs != NULL; addrs++) {
                struct in6_addr in6;

                //
                // Save the v4 address as a v4-mapped v6 address.
                //
                CreateV4MappedAddress(&in6, *(struct in_addr *)*addrs);

                if (! SaveAddress(&in6, pAddrs, pNumSlots, pNumAddrs))
                    return WSA_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Pick up the canonical name.
        //
        length = strlen(hA->h_name) + 1;
        Alias = *pAlias;
        if (Alias == NULL)
            Alias = LocalAlloc(0, length);
        else
            Alias = LocalReAlloc(Alias, length + 1, LMEM_MOVEABLE);
        if (Alias == NULL)
            return WSA_NOT_ENOUGH_MEMORY;

        memcpy(Alias, hA->h_name, length);
        *pAlias = Alias;
    }

    return 0;
}

//* QueryDNS
//
//  Helper routine for getipnodebyname and getaddrinfo
//  that performs name resolution by querying the DNS.
//
//  This helper function always initializes
//  *pAddrs, *pNumAddrs, and *pAlias
//  and may return memory that must be freed,
//  even if it returns an error code.
//
//  Return values are WSA error codes, 0 means success.
//
//  The NT4 DNS name space resolver (rnr20.dll) does not
//  cache replies when you request a specific RR type.
//  This means that every call to getipnodebyname
//  results in DNS message traffic. There is no caching!
//  On NT5 there is caching because the resolver understands AAAA.
//
static int
QueryDNS(const char *name, uint LookupType,
         TDI_ADDRESS_IP6 **pAddrs, uint *pNumAddrs,
         char **pAlias)
{
    uint AliasCount = 0;
    uint NumSlots;
    char *Name = (char *)name;
    int err;

    //
    // Start with zero addresses.
    //
    *pAddrs = NULL;
    NumSlots = *pNumAddrs = 0;
    *pAlias = NULL;

    for (;;) {
        //
        // Must have at least one of L_AAAA and L_A enabled.
        //
        // Unfortunately it seems that some DNS servers out there
        // do not react properly to T_ALL queries - they reply
        // with referrals instead of doing the recursion
        // to get A and AAAA answers. To work around this bug,
        // we query separately for A and AAAA.
        //
        if (LookupType & L_AAAA) {
            err = QueryDNSforAAAA(Name, pAddrs, &NumSlots, pNumAddrs, pAlias);
            if (err)
                break;
        }

        if (LookupType & L_A) {
            err = QueryDNSforA(Name, pAddrs, &NumSlots, pNumAddrs, pAlias);
            if (err)
                break;
        }

        //
        // If we found addresses, then we are done.
        //
        if (*pNumAddrs != 0) {
            err = 0;
            break;
        }

        if ((*pAlias != NULL) && (strcmp(Name, *pAlias) != 0)) {
            char *alias;

            //
            // Stop infinite loops due to DNS misconfiguration.
            // There appears to be no particular recommended
            // limit in RFCs 1034 and 1035.
            //
            if (++AliasCount > 16) {
                err = WSANO_RECOVERY;
                break;
            }

            //
            // If there was a new CNAME, then look again.
            // We need to copy *pAlias because *pAlias
            // could be deleted during the next iteration.
            //
            alias = LocalAlloc(0, strlen(*pAlias) + 1);
            if (alias == NULL) {
                err = WSA_NOT_ENOUGH_MEMORY;
                break;
            }
            strcpy(alias, *pAlias);
            if (Name != name)
                LocalFree(Name);
            Name = alias;
        }
        else if (LookupType >> 2) {
            //
            // Or we were looking for one type and are willing to take another.
            //
            LookupType >>= 2;  // Switch to secondary lookup type.
        }
        else {
            //
            // This name does not resolve to any addresses.
            //
            err = WSAHOST_NOT_FOUND;
            break;
        }
    }

    if (Name != name)
        LocalFree(Name);
    return err;
}
