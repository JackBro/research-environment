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
// Test tool for sending interesting IPv6 packets.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2ip6.h>
#include <ip6.h>
#include <icmp6.h>

__inline uint
MIN(uint a, uint b)
{
    if (a < b)
        return a;
    else
        return b;
}

#define SEND_PAUSE 20

extern int
parse_addr(char *name, struct sockaddr_in6 *sin6);

extern void
TestMultipleExtensionHeaders(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest);

extern void
TestManyExtensionHeaders(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest,
    uchar WhichExtensionHeader);

extern void
TestCompletelyRandomPackets(SOCKET s);

extern void
TestSomewhatRandomPackets(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest);

extern void
TestManySmallFragments(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest);

void _CRTAPI1
main(int argc, char **argv)
{
    WSADATA WsaData;
    SOCKET s;
    SOCKADDR_IN6 sin6LLSrc, sin6LLDst;
    SOCKADDR_IN6 sin6IPSrc, sin6IPDst;
    char *LLSrc = NULL, *LLDst = NULL;
    int i;

    //
    // Initialize winsock.
    //
    if (WSAStartup(MAKEWORD(2, 0), &WsaData)) {
        fprintf(stderr, "hdrtest: WSAStartup failed\n");
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-ls") == 0) && (i+1 < argc)) {
            LLSrc = argv[++i];
        }
        else if ((strcmp(argv[i], "-ld") == 0) && (i+1 < argc)) {
            LLDst = argv[++i];
        }
        else
            break;
    }
    argc -= i;
    argv += i;

    if (argc < 2) {
        fprintf(stderr,
            "usage: hdrtest [-ls ll-src] [-ld ll-dst] src dst tst1 tst2 ...\n"
            "  ll-src   IPv6 address specifying the sending interface\n"
            "  ll-dst   IPv6 address resolving to the link-level destination\n"
            "  src      IPv6 header Source field\n"
            "  dst      IPv6 header Destination field\n\n"
            "Test Numbers:\n"
            "  1        Send 377 packets with extension header combinations\n"
            "  2        Send fragmented packets with many extension headers\n"
            "  3        Send somewhat random packets\n"
            "  4        Send many small fragments very rapidly\n");
        exit(1);
    }

    //
    // Get the source address for the IPv6 header.
    //
    if (!parse_addr(argv[0], &sin6IPSrc)) {
        fprintf(stderr, "hdrtest: bad address %s\n", argv[0]);
        exit(1);
    }

    //
    // Get the destination address for the IPv6 header.
    //
    if (!parse_addr(argv[1], &sin6IPDst)) {
        fprintf(stderr, "hdrtest: bad address %s\n", argv[1]);
        exit(1);
    }

    if (LLSrc == NULL) {
        //
        // Use sin6IPSrc by default.
        //
        sin6LLSrc = sin6IPSrc;
    }
    else {
        //
        // Get the source address specifying the sending interface.
        //
        if (!parse_addr(LLSrc, &sin6LLSrc)) {
            fprintf(stderr, "hdrtest: bad address %s\n", LLSrc);
            exit(1);
        }
    }

    if (LLDst == NULL) {
        //
        // Use sin6IPDst by default.
        //
        sin6LLDst = sin6IPDst;
    }
    else {
        //
        // Get the destination address specifying the link-level address.
        //
        if (!parse_addr(LLDst, &sin6LLDst)) {
            fprintf(stderr, "hdrtest: bad address %s\n", LLDst);
            exit(1);
        }
    }

    //
    // Create a raw IPv6 socket.
    //
	s = socket(PF_INET6, SOCK_RAW, 0);
	if (s == INVALID_SOCKET) {
        fprintf(stderr, "hdrtest: socket failed\n");
        exit(1);
	}

    //
    // Bind our source address to the socket.
    // This has the effect of picking an outgoing interface.
    //
    if (bind(s, (PSOCKADDR)&sin6LLSrc, sizeof sin6LLSrc) < 0) {
        fprintf(stderr, "hdrtest: bind(%s) failed\n",
                inet6_ntoa(&sin6LLSrc.sin6_addr));
        exit(1);
    }

    //
    // Connect to the destination address.
    // This has the effect of setting the destination for send() calls.
    //
    if (connect(s, (PSOCKADDR)&sin6LLDst, sizeof sin6LLDst) < 0) {
        fprintf(stderr, "hdrtest: connect failed\n");
        exit(1);
    }

    //
    // Perform the requested tests.
    //
    for (i = 2; i < argc; i++) {
        switch (atoi(argv[i])) {
        default:
            fprintf(stderr, "hdrtest: Unrecognized test %s\n", argv[i]);
            exit(1);
        case 1:
            TestMultipleExtensionHeaders(s,
                                         &sin6IPSrc.sin6_addr,
                                         &sin6IPDst.sin6_addr);
            break;
        case 2:
            //
            // Send the packet with lots of fragment headers first...
            // It takes much longer to process, so the Reply should
            // be sent after the Reply for later tests.
            //
            TestManyExtensionHeaders(s,
                                     &sin6IPSrc.sin6_addr,
                                     &sin6IPDst.sin6_addr,
                                     IP_PROTOCOL_FRAGMENT);
            Sleep(1000);
            TestManyExtensionHeaders(s,
                                     &sin6IPSrc.sin6_addr,
                                     &sin6IPDst.sin6_addr,
                                     IP_PROTOCOL_DEST_OPTS);
            Sleep(1000);
            TestManyExtensionHeaders(s,
                                     &sin6IPSrc.sin6_addr,
                                     &sin6IPDst.sin6_addr,
                                     IP_PROTOCOL_ROUTING);
            Sleep(1000);
            TestManyExtensionHeaders(s,
                                     &sin6IPSrc.sin6_addr,
                                     &sin6IPDst.sin6_addr,
                                     IP_PROTOCOL_V6);
            break;
        case 3:
            TestSomewhatRandomPackets(s,
                                      &sin6IPSrc.sin6_addr,
                                      &sin6IPDst.sin6_addr);
            break;
        case 4:
            TestManySmallFragments(s,
                                 &sin6IPSrc.sin6_addr,
                                 &sin6IPDst.sin6_addr);
            break;
        }
    }

    //
    // Cleanup.
    //
    closesocket(s);
    WSACleanup();
    exit(0);
}

uint RandomValue = 0;

//* Random - Generate a psuedo random value between 1 and 2^32.
//
//  This routine is a quick and dirty psuedo random number generator.
//  It has the advantages of being fast and consuming very little
//  memory (for either code or data).  The random numbers it produces are
//  not of the best quality, however.  A much better generator could be
//  had if we were willing to use an extra 256 bytes of memory for data.
//
//  This routine uses the linear congruential method (see Knuth, Vol II),
//  with specific values for the multiplier and constant taken from
//  Numerical Recipes in C Second Edition by Press, et. al.
//
uint  // Returns: A random value between 1 and 2^32.
Random(void)
{
    //
    // The algorithm is R = (aR + c) mod m, where R is the random number,
    // a is a magic multiplier, c is a constant, and the modulus m is the
    // maximum number of elements in the period.  We chose our m to be 2^32
    // in order to get the mod operation for free.
    //
    RandomValue = (1664525 * RandomValue) + 1013904223;

    return RandomValue;
}

//* RandomNumber
//
//  Returns a number randomly selected from a range [Min, Max)
//
uint
RandomNumber(uint Min, uint Max)
{
    uint Number;

    //
    // Note that the high bits of Random() are much more random
    // than the low bits.
    //
    Number = Max - Min; // Spread.
    Number = (uint)(((unsigned __int64)Random() * Number) >> 32);
    Number += Min;

    return Number;
}

void
RandomBytes(uchar *Data, uint Length)
{
    uint i;

    for (i = 0; i < Length; i++)
        Data[i] = (uchar)(Random() >> 24);
}

//
// Use getipnodebyname to parse an address.
//
int
parse_addr(char *name, struct sockaddr_in6 *sin6)
{
    struct hostent *h;

    h = getipnodebyname(name, AF_INET6, AI_DEFAULT, NULL);
    if (h == NULL)
        return FALSE;

    memset(sin6, 0, sizeof *sin6);
    sin6->sin6_family = AF_INET6;
    memcpy(&sin6->sin6_addr, h->h_addr, sizeof(struct in6_addr));
    freehostent(h);
    return TRUE;
}

struct PseudoHeader {
    in6_addr Source;
    in6_addr Dest;
    uint PayloadLength;
    uint NextHeader;
};

ushort
Checksum(
    struct PseudoHeader *Pseudo,
    uint PayloadLength,
    uchar *Payload)
{
    uint Checksum;
    uint i;

    //
    // Checksum the pseudo-header.
    //
    Checksum = 0;
    for (i = 0; i < sizeof *Pseudo / sizeof(ushort); i++)
        Checksum += ((ushort *)Pseudo)[i];

    //
    // Add in the payload.
    //
    for (i = 0; i+1 < PayloadLength; i += 2) {
        Checksum += *(ushort *)Payload;
        Payload += 2;
    }

    //
    // Add in the odd byte at the end.
    //
    if (i < PayloadLength)
        Checksum += htons((ushort)*Payload);

    //
    // Reduce to 16 bits and take complement.
    //
    while (Checksum >> 16)
        Checksum = (Checksum & 0xffff) + (Checksum >> 16);

    Checksum = ~Checksum;

    //
    // Unless zero results - replace with ones.
    //
    if (Checksum == 0)
        Checksum = 0xffff;

    return (ushort)Checksum;
}

ushort
ChecksumPacket(
    IPv6Header *Packet,
    uint PacketLength,
    uchar *UpperLayer,
    uchar UpperLayerProtocol
    )
{
    struct PseudoHeader Pseudo;
    uint UpperLayerLength;

    UpperLayerLength = PacketLength - (UpperLayer - (uchar *)Packet);

    //
    // Construct the pseudo-header.
    //
    Pseudo.Source = Packet->Source;
    Pseudo.Dest = Packet->Dest;
    Pseudo.PayloadLength = htonl(UpperLayerLength);
    Pseudo.NextHeader = htonl((uint)UpperLayerProtocol);

    return Checksum(&Pseudo, UpperLayerLength, UpperLayer);
}

uchar *
WriteHeader(IPv6Header *H, uchar *Buffer, uchar *BufferEnd,
            uchar ThisHeader, uchar NextHeader)
{
    switch (ThisHeader) {
    case IP_PROTOCOL_HOP_BY_HOP:
    case IP_PROTOCOL_DEST_OPTS: {
        IPv6OptionsHeader *Hdr = (IPv6OptionsHeader *)(Buffer - 8);
        Hdr->NextHeader = NextHeader;
        Hdr->HeaderExtLength = 0;
        memset(Hdr + 1, 0, 6);
        return (uchar *)Hdr;
    }

    case IP_PROTOCOL_ROUTING: {
        IPv6RoutingHeader *Hdr = (IPv6RoutingHeader *)Buffer - 1;
        Hdr->NextHeader = NextHeader;
        Hdr->HeaderExtLength = 0;
        Hdr->RoutingType = 0;
        Hdr->SegmentsLeft = 0;
        Hdr->Reserved = 0;
        return (uchar *)Hdr;
    }

    case IP_PROTOCOL_FRAGMENT: {
        FragmentHeader *Hdr = (FragmentHeader *)Buffer - 1;
        Hdr->NextHeader = NextHeader;
        Hdr->Reserved = 0;
        Hdr->OffsetFlag = 0; // just the one fragment
        Hdr->Id = htonl(0xdeadbeef);
        return (uchar *)Hdr;
    }

    case IP_PROTOCOL_V6: {
        IPv6Header *Hdr = (IPv6Header *)Buffer - 1;

        *Hdr = *H;
        Hdr->NextHeader = NextHeader;
        Hdr->PayloadLength = htons(BufferEnd - Buffer);
        return (uchar *)Hdr;
    }
    }

    // Shouldn't happen.
    return NULL;
}

//
// Test multiple extension headers.
//
// Sends an Echo Request with a variety of extension headers.
// We test every combination of 4 extension headers,
// restricting hop-by-hop to only appear after an IPv6 header.
//
// This test sends 377 packets, one a second.
//
void
TestMultipleExtensionHeaders(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest)
{
    static uchar Prots[] = {
        IP_PROTOCOL_HOP_BY_HOP,
        IP_PROTOCOL_DEST_OPTS,
        IP_PROTOCOL_ROUTING,
        IP_PROTOCOL_FRAGMENT,
        IP_PROTOCOL_V6
    };

    IPv6Header H;
    struct {
        IPv6Header H;
        // space for four headers (each either 8 or 40 bytes)
        uchar Headers[4 * sizeof(IPv6Header)];
        ICMPv6Header IH;
        uint Data;
    } Packet;

    uint h0, h1, h2, h3;

    // Fill in template fields for WriteHeader.
    H.VersClassFlow = IP_VERSION;
    H.HopLimit = 128;
    H.Source = *Source;
    H.Dest = *Dest;

    // Fill in ICMP fields.
    Packet.IH.Type = ICMPv6_ECHO_REQUEST;
    Packet.IH.Code = 0;
    Packet.IH.Checksum = 0;
    Packet.Data = htonl(0xfeedfeed);

    // Calculate checksum.
    Packet.H = H;
    Packet.IH.Checksum = ChecksumPacket(&Packet.H,
                                        sizeof Packet,
                                        (uchar *)&Packet.IH,
                                        IP_PROTOCOL_ICMPv6);

    for (h0 = 0; h0 < 5; h0++) {
        for (h1 = (h0 != 4); h1 < 5; h1++) {
            for (h2 = (h1 != 4); h2 < 5; h2++) {
                for (h3 = (h2 != 4); h3 < 5; h3++) {
                    uchar *Headers;
                    uchar *PacketEnd;

                    Headers = (uchar *) &Packet.IH;
                    PacketEnd = (uchar *)(&Packet + 1);

                    Headers = WriteHeader(&H, Headers, PacketEnd,
                                          Prots[h3], IP_PROTOCOL_ICMPv6);
                    Headers = WriteHeader(&H, Headers, PacketEnd,
                                          Prots[h2], Prots[h3]);
                    Headers = WriteHeader(&H, Headers, PacketEnd,
                                          Prots[h1], Prots[h2]);
                    Headers = WriteHeader(&H, Headers, PacketEnd,
                                          Prots[h0], Prots[h1]);
                    Headers = WriteHeader(&H, Headers, PacketEnd,
                                          IP_PROTOCOL_V6, Prots[h0]);

                    if (send(s, (char *)Headers, PacketEnd - Headers, 0) < 0) {
                        fprintf(stderr, "hdrtest: send failed\n");
                        exit(1);
                    }

                    //
                    // Sleep one second, so if the receiver feels
                    // a need to generate ICMP errors it should
                    // not be inhibited by rate-limiting.
                    //
                    Sleep(1000);
                }
            }
        }
    }
}

//
// Sends multiple fragments that assemble to yield a large packet
// with 8190 extension headers.
//
void
TestManyExtensionHeaders(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest,
    uchar WhichExtensionHeader)
{
    struct {
        IPv6Header H;
        FragmentHeader FH;
        union {
            unsigned char Payload[IPv6_MINIMUM_MTU - sizeof(IPv6Header) - sizeof(FragmentHeader)];
            struct {
                ICMPv6Header IH;
                uint Seq;
            };
        };
    } Packet;
    uint ExtensionHeaderSize;
    uint PayloadSize;
    uint NumTotalExtensionHeaders;
    uint NumFragExtensionHeaders;
    uint NumRemainingExtensionHeaders;
    uint NumFullFragments;
    uint PayloadLength;
    uint i, j;

    if (WhichExtensionHeader == IP_PROTOCOL_V6)
        ExtensionHeaderSize = sizeof(IPv6Header);
    else
        ExtensionHeaderSize = 8;

    //
    // Final payload is an ICMP Echo Request.
    //
    PayloadSize = sizeof(ICMPv6Header) + sizeof(uint);

    //
    // We want the reassembled packet to be as large as possible,
    // with extension headers followed by the final payload.
    // How many extension headers?
    //
    NumTotalExtensionHeaders =
        (MAX_IPv6_PAYLOAD - PayloadSize) / ExtensionHeaderSize;

    //
    // We will send a number of fragments. Each fragment (except the last)
    // will consist of a fragment header followed by some number of extension
    // headers. How many extension headers will fit in a fragment?
    //
    NumFragExtensionHeaders =
        (IPv6_MINIMUM_MTU - sizeof(IPv6Header) - sizeof(FragmentHeader)) /
                                                        ExtensionHeaderSize;

    //
    // How many "full" fragments will we send?
    // After we've sent the full fragments,
    // how many extension headers are left over?
    // We need this number to be non-zero.
    //
    for (;;) {
        NumFullFragments = NumTotalExtensionHeaders / NumFragExtensionHeaders;
        NumRemainingExtensionHeaders = NumTotalExtensionHeaders -
            NumFullFragments * NumFragExtensionHeaders;
        if (NumRemainingExtensionHeaders != 0)
            break;

        //
        // Try again, sending fewer extension headers in each fragment.
        //
        NumFragExtensionHeaders--;
    }

    //
    // Send the full fragments.
    //

    Packet.H.VersClassFlow = IP_VERSION;
    Packet.H.NextHeader = IP_PROTOCOL_FRAGMENT;
    Packet.H.HopLimit = 128;
    Packet.H.Source = *Source;
    Packet.H.Dest = *Dest;

    Packet.FH.NextHeader = WhichExtensionHeader;
    Packet.FH.Reserved = 0;
    Packet.FH.Id = htonl(0x12345678);

    for (i = 0; i <= NumFullFragments; i++) {
        uint NumExtensionHeaders;

        if (i == NumFullFragments)
            NumExtensionHeaders = NumRemainingExtensionHeaders;
        else
            NumExtensionHeaders = NumFragExtensionHeaders;

        PayloadLength = sizeof(FragmentHeader) +
            ExtensionHeaderSize * NumExtensionHeaders;
        Packet.H.PayloadLength = htons(PayloadLength);

        Packet.FH.OffsetFlag =
            htons(FRAGMENT_FLAG_MASK |
                  (i * ExtensionHeaderSize * NumFragExtensionHeaders));

        for (j = 1; j <= NumExtensionHeaders; j++) {
            uchar *PayloadStart, *PayloadEnd;

            //
            // The WriteHeader helper is not so convenient here.
            //
            PayloadStart = &Packet.Payload[ExtensionHeaderSize * j];
            PayloadEnd = PayloadStart + PayloadSize +
                (ExtensionHeaderSize *
                 (NumTotalExtensionHeaders - i*NumFragExtensionHeaders - j));

            //
            // The very last extension header in the last packet
            // has a different NextHeader value.
            //
            WriteHeader(&Packet.H,
                        PayloadStart, PayloadEnd,
                        WhichExtensionHeader,
                        ((i == NumFullFragments) &&
                         (j == NumExtensionHeaders) ?
                         IP_PROTOCOL_ICMPv6 : WhichExtensionHeader));
        }

        Sleep(SEND_PAUSE);
        if (send(s, (char *)&Packet, sizeof Packet.H + PayloadLength, 0) < 0) {
            fprintf(stderr, "hdrtest: send failed\n");
            exit(1);
        }
    }

    //
    // Send final fragment with the ICMP Echo Request.
    //
    PayloadLength = sizeof(FragmentHeader) + PayloadSize;
    Packet.H.PayloadLength = htons(PayloadLength);
    Packet.FH.OffsetFlag =
        htons(ExtensionHeaderSize * NumTotalExtensionHeaders);

    Packet.IH.Type = ICMPv6_ECHO_REQUEST;
    Packet.IH.Code = 0;
    Packet.IH.Checksum = 0;
    Packet.Seq = htonl(0xfeed0000 + WhichExtensionHeader);

    Packet.IH.Checksum = ChecksumPacket(&Packet.H,
                                        sizeof Packet.H + PayloadLength,
                                        (uchar *)&Packet.IH,
                                        IP_PROTOCOL_ICMPv6);

    Sleep(SEND_PAUSE);
    if (send(s, (char *)&Packet, sizeof Packet.H + PayloadLength, 0) < 0) {
        fprintf(stderr, "hdrtest: send failed\n");
        exit(1);
    }
}

void
SendCompletelyRandomPacket(SOCKET s)
{
    uchar Packet[IPv6_MINIMUM_MTU];
    uint PacketLength;

    //
    // Select a length for the random packet.
    //
    PacketLength = RandomNumber(0, IPv6_MINIMUM_MTU+1);

    //
    // Randomly generate data.
    //
    RandomBytes(Packet, PacketLength);

    //
    // Send the packet.
    //
    if (send(s, (char *)Packet, PacketLength, 0) < 0) {
        fprintf(stderr, "hdrtest: send failed\n");
        exit(1);
    }
}

void
TestCompletelyRandomPackets(SOCKET s)
{
    for (;;) {
        Sleep(SEND_PAUSE);
        SendCompletelyRandomPacket(s);
    }
}

void
SomewhatRandomAddress(IPv6Addr *Addr, IPv6Addr *Preferred)
{
    uint Rand;

    Rand = RandomNumber(0, 100);
    if (Rand < 2) {
        //
        // Unspecified address.
        //
        *Addr = in6addr_any;
    }
    else if (Rand < 4) {
        //
        // Loopback address.
        //
        *Addr = in6addr_loopback;
    }
    else if (Rand < 6) {
        //
        // Link-local address.
        //
        Addr->s6_bytes[0] = 0xfe;
        Addr->s6_bytes[1] = (Addr->s6_bytes[1] & 0x3f) | 0x80;
    }
    else if (Rand < 8) {
        //
        // Site-local address.
        //
        Addr->s6_bytes[0] = 0xfe;
        Addr->s6_bytes[1] = (Addr->s6_bytes[1] & 0x3f) | 0xc0;
    }
    else if (Rand < 10) {
        //
        // v4-compatible address.
        //
        Addr->s6_dwords[0] = 0;
        Addr->s6_dwords[1] = 0;
        Addr->s6_dwords[2] = 0;
    }
    else if (Rand < 12) {
        //
        // v4-mapped address.
        //
        Addr->s6_dwords[0] = 0;
        Addr->s6_dwords[1] = 0;
        Addr->s6_words[4] = 0;
        Addr->s6_words[5] = 0xffff;
    }
    else if (Rand < 14) {
        //
        // Random address.
        //
    }
    else if (Rand < 16) {
        //
        // Multicast address.
        //
        Addr->s6_bytes[0] = 0xff;
    }
    else if (Rand < 20) {
        //
        // All-nodes link-local address.
        //
        *Addr = in6addr_loopback;
        Addr->s6_bytes[0] = 0xff;
        Addr->s6_bytes[1] = 0x02;
    }
    else {
        //
        // Use the preferred address.
        //
        *Addr = *Preferred;
    }
}

uint
SomewhatRandomHeaderLength(
    IPv6OptionsHeader *pHdr,
    uint Length)
{
    uint HdrLen;

    if ((RandomNumber(0, 100) < 5) || (Length < EXT_LEN_UNIT)) {
        //
        // Use a random length that might be too big.
        //
    }
    else {
        //
        // Pick a random length.
        //
        pHdr->HeaderExtLength = RandomNumber(0, Length / EXT_LEN_UNIT);
    }

    HdrLen = (pHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (HdrLen > Length)
        HdrLen = Length;

    return HdrLen;
}

void
SomewhatRandomFragmentHeader(
    FragmentHeader *pHdr)
{
    uint Rand;

    //
    // Leave the reserved field random.
    //

    if (RandomNumber(0, 100) < 90) {
        //
        // This fragment is zero-offset, no more fragments.
        //
        pHdr->OffsetFlag = 0;
    }
    else {
        //
        // Random offset and flag bits.
        //
    }

    if (RandomNumber(0, 100) < 50) {
        //
        // Use a common fragment id.
        //
        pHdr->Id = 0xdeadbeef;
    }
    else {
        //
        // Random id.
        //
    }
}

void
SomewhatRandomOptions(
    IPv6OptionsHeader *pHdr,
    uint Length,
    uchar ThisHeader)
{
    uint Rand;
    uchar *Data;

    Data = (uchar *)(pHdr + 1);
    Length -= sizeof *pHdr;

    while (Length != 0) {
        OptionHeader *pOpt = (OptionHeader *) Data;
        uint OptLen;

        //
        // Pick an option type.
        //
        Rand = RandomNumber(0, 100);
        if (Rand < 40)
            pOpt->Type = OPT6_PAD_1;
        else if (Rand < 80)
            pOpt->Type = OPT6_PAD_N;
        else if (Rand < 82)
            pOpt->Type = OPT6_JUMBO_PAYLOAD;
        else if (Rand < 84)
            pOpt->Type = OPT6_ROUTER_ALERT;
        else if (Rand < 85)
            pOpt->Type = OPT6_TUNNEL_ENCAP_LIMIT;
        else if (Rand < 86)
            pOpt->Type = OPT6_BINDING_UPDATE;
        else if (Rand < 87)
            pOpt->Type = OPT6_BINDING_ACK;
        else if (Rand < 88)
            pOpt->Type = OPT6_BINDING_REQUEST;
        else if (Rand < 89)
            pOpt->Type = OPT6_ENDPOINT_ID;
        else if (Rand < 90)
            pOpt->Type = OPT6_NSAP_ADDR;
        else if (Rand < 92)
            ; // leave it random
        else
            pOpt->Type &= ~0xc0; // action=skip and continue

        if ((Length < sizeof *pOpt) || (pOpt->Type == OPT6_PAD_1)) {
            Length -= 1;
            Data += 1;
            continue;
        }

        //
        // Pick an option length.
        //
        Rand = RandomNumber(0, 100);
        if (Rand < 40)
            pOpt->DataLength = (uchar) MIN(Length - 2, 255);
        else if (Rand < 90)
            pOpt->DataLength = (uchar) RandomNumber(0, MIN(Length - 2, 255));
        else
            ; // leave it random

        OptLen = MIN(pOpt->DataLength + 2, Length);
        Length -= OptLen;
        Data += OptLen;
    }
}

void
SomewhatRandomICMP(
    IPv6Header *Packet,
    uint PacketLength,
    ICMPv6Header *pHdr)
{
    uint Rand;

    Rand = RandomNumber(0, 100);
    if (Rand < 20)
        pHdr->Type = ICMPv6_ECHO_REQUEST;
    else if (Rand < 25)
        pHdr->Type = ICMPv6_DESTINATION_UNREACHABLE;
    else if (Rand < 30)
        pHdr->Type = ICMPv6_PACKET_TOO_BIG;
    else if (Rand < 35)
        pHdr->Type = ICMPv6_TIME_EXCEEDED;
    else if (Rand < 40)
        pHdr->Type = ICMPv6_PARAMETER_PROBLEM;
    else if (Rand < 45)
        pHdr->Type = ICMPv6_ECHO_REPLY;
    else if (Rand < 50)
        pHdr->Type = ICMPv6_MULTICAST_LISTENER_QUERY;
    else if (Rand < 55)
        pHdr->Type = ICMPv6_MULTICAST_LISTENER_REPORT;
    else if (Rand < 60)
        pHdr->Type = ICMPv6_MULTICAST_LISTENER_DONE;
    else if (Rand < 65)
        pHdr->Type = ICMPv6_ROUTER_SOLICIT;
    else if (Rand < 70)
        pHdr->Type = ICMPv6_ROUTER_ADVERT;
    else if (Rand < 75)
        pHdr->Type = ICMPv6_NEIGHBOR_SOLICIT;
    else if (Rand < 80)
        pHdr->Type = ICMPv6_NEIGHBOR_ADVERT;
    else if (Rand < 85)
        pHdr->Type = ICMPv6_REDIRECT;
    else if (Rand < 90)
        pHdr->Type = ICMPv6_ROUTER_RENUMBERING;
    else
        ; // Leave it random.

    if (RandomNumber(0, 100) < 50)
        pHdr->Code = 0;

    if (((uint)((uchar *)pHdr - (uchar *)Packet) <= PacketLength) && 
        (RandomNumber(0, 100) < 98)) {
        //
        // Calculate a correct checksum.
        //
        pHdr->Checksum = 0;
        pHdr->Checksum = ChecksumPacket(Packet,
                                        PacketLength,
                                        (uchar *)pHdr,
                                        IP_PROTOCOL_ICMPv6);
    }
}

uint
SomewhatRandomRoutingHeader(
    IPv6Header *Packet,
    IPv6RoutingHeader *pHdr,
    uint DataLength)
{
    uint MaxAddresses;
    uint NumAddresses;
    uint HdrLen;
    IPv6Addr *Addresses;
    uint i;

    //
    // Note that DataLength >= sizeof *pHdr which is also EXT_LEN_UNIT.
    //
    MaxAddresses = (DataLength - sizeof *pHdr) / sizeof(IPv6Addr);

    if (RandomNumber(0, 100) < 5) {
        //
        // Use a random length that might be too big.
        //
    }
    else {
        //
        // Pick a random length that is even.
        //
        pHdr->HeaderExtLength = 2 * RandomNumber(0, MaxAddresses + 1);
    }

    HdrLen = (pHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;
    NumAddresses = pHdr->HeaderExtLength / 2;

    if (RandomNumber(0, 100) < 95)
        pHdr->RoutingType = 0;

    if (RandomNumber(0, 100) < 95)
        pHdr->SegmentsLeft = RandomNumber(0, NumAddresses + 1);

    NumAddresses = MIN(NumAddresses, MaxAddresses);
    Addresses = (IPv6Addr *) (pHdr + 1);

    for (i = 0; i < NumAddresses; i++) {
        IPv6Addr Preferred;

        if (RandomNumber(0, 100) < 25)
            Preferred = Packet->Source;
        else
            Preferred = Packet->Dest;

        if (RandomNumber(0, 100) < 50)
            Preferred.s6_dwords[3] ^= 0xdeadbeef;

        SomewhatRandomAddress(&Addresses[i], &Preferred);
    }

    return MIN(HdrLen, DataLength);
}

void
SomewhatRandomHeaders(
    IPv6Header *Packet,
    uint PacketLength,
    uint DataLength)
{
    uchar *pNextHeader = &Packet->NextHeader;
    uchar *Data = (uchar *)(Packet + 1);
    uint Rand;
    int FirstHeader = TRUE;

    for (;;) {
        Rand = RandomNumber(0, 100);
        if (Rand < 1) {
            //
            // Random next header value.
            //
            break;
        }
        else if (Rand < 10) {
            //
            // No next header.
            //
            *pNextHeader = IP_PROTOCOL_NONE;
            break;
        }
        else if (Rand < 20) {
            FragmentHeader *pHdr;

            //
            // Insert Fragment header.
            //
            *pNextHeader = IP_PROTOCOL_FRAGMENT;
            if (DataLength < sizeof *pHdr)
                break;

            pHdr = (FragmentHeader *) Data;
            SomewhatRandomFragmentHeader(pHdr);
            pNextHeader = &pHdr->NextHeader;
            Data += sizeof *pHdr;
            DataLength -= sizeof *pHdr;
        }
        else if (Rand < 40) {
            IPv6RoutingHeader *pHdr;
            uint HdrLen;

            //
            // Insert Routing header.
            //
            *pNextHeader = IP_PROTOCOL_ROUTING;
            if (DataLength < sizeof(IPv6RoutingHeader))
                break;

            pHdr = (IPv6RoutingHeader *) Data;
            HdrLen = SomewhatRandomRoutingHeader(Packet, pHdr, DataLength);
            pNextHeader = &pHdr->NextHeader;
            Data += HdrLen;
            DataLength -= HdrLen;
        }
        else if (Rand < 60) {
            IPv6OptionsHeader *pHdr;
            uint HdrLen;

            //
            // Insert Destination Options header.
            //
            *pNextHeader = IP_PROTOCOL_DEST_OPTS;
            if (DataLength < sizeof(IPv6OptionsHeader))
                break;

            pHdr = (IPv6OptionsHeader *) Data;
            HdrLen = SomewhatRandomHeaderLength(pHdr, DataLength);
            SomewhatRandomOptions(pHdr, HdrLen, IP_PROTOCOL_DEST_OPTS);
            pNextHeader = &pHdr->NextHeader;
            Data += HdrLen;
            DataLength -= HdrLen;
        }
        else if (Rand < 80) {
            IPv6OptionsHeader *pHdr;
            uint HdrLen;

            if (!FirstHeader) {
                //
                // Generally only want Hop-by-Hop if it's first.
                //
                if (RandomNumber(0, 100) < 95)
                    continue;
            }

            //
            // Insert Hop-by-Hop Options header.
            //
            *pNextHeader = IP_PROTOCOL_HOP_BY_HOP;
            if (DataLength < sizeof(IPv6OptionsHeader))
                break;

            pHdr = (IPv6OptionsHeader *) Data;
            HdrLen = SomewhatRandomHeaderLength(pHdr, DataLength);
            SomewhatRandomOptions(pHdr, HdrLen, IP_PROTOCOL_HOP_BY_HOP);
            pNextHeader = &pHdr->NextHeader;
            Data += HdrLen;
            DataLength -= HdrLen;
        }
        else {
            ICMPv6Header *pHdr;

            //
            // Insert ICMP header.
            //
            *pNextHeader = IP_PROTOCOL_ICMPv6;
            if (DataLength < sizeof(ICMPv6Header))
                break;

            pHdr = (ICMPv6Header *) Data;
            SomewhatRandomICMP(Packet, PacketLength, pHdr);
            break;
        }

        FirstHeader = FALSE;
    }
}

void
SendSomewhatRandomPacket(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest)
{
    struct {
        IPv6Header H;
        uchar Payload[IPv6_MINIMUM_MTU - sizeof(IPv6Header)];
    } Packet;
    uint PacketLength;
    uint PayloadLength;
    uint Rand;

    //
    // Select a length for the random packet
    // and fill with random bytes to start.
    //
    PacketLength = RandomNumber(0, IPv6_MINIMUM_MTU+1);
    RandomBytes((uchar *)&Packet, PacketLength);

    //
    // Random Version/Traffic Class/Flow Label,
    // Except the version should be 6 with high probability.
    //
    if (RandomNumber(0, 1000) != 0)
        Packet.H.VersClassFlow = IP_VERSION |
            (Packet.H.VersClassFlow &~ IP_VER_MASK);

    //
    // With high probability, the PayloadLength should be correct.
    // But sometimes it should be too small or too large.
    //
    Rand = RandomNumber(0, 100);
    if ((Rand < 5) || (PacketLength < sizeof(IPv6Header))) {
        //
        // Leave random PayloadLength in the header,
        // but calculate a sane value for our later work.
        //
        if (PacketLength < sizeof(IPv6Header))
            PayloadLength = 0;
        else
            PayloadLength = PacketLength - sizeof(IPv6Header);
    }
    else if (Rand < 10) {
        //
        // PayloadLength smaller than link-level packet length.
        //
        PayloadLength = RandomNumber(0, PacketLength - sizeof(IPv6Header) + 1);
        Packet.H.PayloadLength = htons(PayloadLength);
    }
    else {
        //
        // Correct PayloadLength.
        //
        PayloadLength = PacketLength - sizeof(IPv6Header);
        Packet.H.PayloadLength = htons(PayloadLength);
    }

    SomewhatRandomAddress(&Packet.H.Source, Source);
    SomewhatRandomAddress(&Packet.H.Dest, Dest);

    SomewhatRandomHeaders(&Packet.H,
                          sizeof(IPv6Header) + PayloadLength,
                          ((PacketLength < sizeof(IPv6Header)) ?
                           0 : PacketLength - sizeof(IPv6Header)));

    //
    // Send the packet.
    //
    if (send(s, (char *)&Packet, PacketLength, 0) < 0) {
        fprintf(stderr, "hdrtest: send failed\n");
        exit(1);
    }
}

void
TestSomewhatRandomPackets(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest)
{
    for (;;) {
        Sleep(SEND_PAUSE);
        SendSomewhatRandomPacket(s, Source, Dest);
    }
}

void
SendSmallFragment(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest,
    uint FragmentId)
{
    struct {
        IPv6Header H;
        FragmentHeader FH;
    } Packet;

    Packet.H.VersClassFlow = IP_VERSION;
    Packet.H.PayloadLength = htons(sizeof Packet - sizeof Packet.H);
    Packet.H.NextHeader = IP_PROTOCOL_FRAGMENT;
    Packet.H.HopLimit = 128;
    Packet.H.Source = *Source;
    Packet.H.Dest = *Dest;

    Packet.FH.NextHeader = IP_PROTOCOL_NONE;
    Packet.FH.Reserved = 0;
    Packet.FH.OffsetFlag = htons(FRAGMENT_FLAG_MASK);
    Packet.FH.Id = htonl(FragmentId);

    if (send(s, (char *)&Packet, sizeof Packet, 0) < 0) {
        fprintf(stderr, "hdrtest: send failed\n");
        exit(1);
    }
}

void
TestManySmallFragments(
    SOCKET s,
    IN6_ADDR *Source,
    IN6_ADDR *Dest)
{
    uint FragmentId = 0;

    //
    // Sends lots of fragments, with Id 0.
    // And sends lots of fragments with different Ids.
    // No pauses between sends here - pound the receiver.
    //
    for (;;) {
        SendSmallFragment(s, Source, Dest, 0);
        SendSmallFragment(s, Source, Dest, ++FragmentId);
    }
}
