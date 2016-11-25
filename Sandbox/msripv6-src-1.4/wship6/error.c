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


#include <windows.h>
#include <winsock2.h>
#include <ws2ip6.h>

struct ErrorTable {
    IP_STATUS Error;
    const char *ErrorString;
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,         "Buffer too small."                     },
    { IP_DEST_NO_ROUTE,         "No route to destination."              },
    { IP_DEST_ADDR_UNREACHABLE, "Destination address unreachable."      },
    { IP_DEST_PROHIBITED,       "Communication prohibited."             },
    { IP_DEST_PORT_UNREACHABLE, "Destination port unreachable."         },
    { IP_NO_RESOURCES,          "No resources."                         },
    { IP_BAD_OPTION,            "Bad option."                           },
    { IP_HW_ERROR,              "Hardware error."                       },
    { IP_PACKET_TOO_BIG,        "Packet too big and can't fragment."    },
    { IP_REQ_TIMED_OUT,         "Request timed out."                    },
    { IP_BAD_REQ,               "Bad request."                          },
    { IP_BAD_ROUTE,             "Invalid source route specified."       },
    { IP_HOP_LIMIT_EXCEEDED,    "Hop limit exceeded."                   },
    { IP_REASSEMBLY_TIME_EXCEEDED, "Reassembly time exceeded."          },
    { IP_PARAMETER_PROBLEM,     "Parameter problem."                    },
    { IP_OPTION_TOO_BIG,        "Option too large."                     },
    { IP_BAD_DESTINATION,       "Invalid destination."                  },
    { IP_DEST_UNREACHABLE,      "Destination unreachable."              },
    { IP_TIME_EXCEEDED,         "Time exceeded."                        },
    { IP_BAD_HEADER,            "Invalid field in header."              },
    { IP_UNRECOGNIZED_NEXT_HEADER, "Unrecognized next header value."    },
    { IP_ICMP_ERROR,            "ICMP error."                           },
    // the following error must be last - it is used as a sentinel      },
    { IP_GENERAL_FAILURE,       "General failure."                      }
};

const char * WSAAPI
GetErrorString(IP_STATUS Error)
{
    int i;

    for (i = 0; ErrorTable[i].Error != IP_GENERAL_FAILURE; i++)
        if (ErrorTable[i].Error == Error)
            break;

    return ErrorTable[i].ErrorString;
}


//* gai_strerror - A description of the error returned by getaddrinfo().
//
char * WSAAPI
gai_strerror(
             int ErrorCode)  // EAI_XXX error code.
{
    switch (ErrorCode) {
    case EAI_ADDRFAMILY:
        return("Address family for nodename not supported");
    case EAI_AGAIN:
        return("Temporary failure in name resolution");
    case EAI_BADFLAGS:
        return("Invalid value for ai_flags");
    case EAI_FAIL:
        return("Non-recoverable failure in name resolution");
    case EAI_FAMILY:
        return("Address family not supported");
    case EAI_MEMORY:
        return("Memory allocation failure");
    case EAI_NODATA:
        return("No address associated with nodename");
    case EAI_NONAME:
        return("Node name nor service name provided, or not known");
    case EAI_SERVICE:
        return("Service name not supported for socket type");
    case EAI_SOCKTYPE:
        return("Socket type not supported");
    case EAI_SYSTEM:
        return("System error returned in errno");
    default:
        return("Unknown error");
    }
}
