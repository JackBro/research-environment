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
// Various subroutines for Internet Protocol Version 6 
//


#include "oscfg.h"
#include "ndis.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdikrnl.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "fragment.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "mld.h"

uint IPv6TickCount = 0;

uint RandomValue = 0;


//* SeedRandom - Provide a seed value.
//
//  Called to provide a seed value for the random number generator.
//
void
SeedRandom(uint Seed)
{
    int i;

    //
    // Incorporate the seed into our random value.
    //
    RandomValue ^= Seed;

    //
    // Stir the bits.
    //
    for (i = 0; i < 100; i++)
        (void) Random();
}


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
    // BUGBUG: What about concurrent calls?
    //
    RandomValue = (1664525 * RandomValue) + 1013904223;

    return RandomValue;
}


//* RandomNumber
//
//  Returns a number randomly selected from a range.
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
    Number = (uint)(((ULONGLONG)Random() * Number) >> 32); // Randomize spread.
    Number += Min;

    return Number;
}


//* GetDataAtPacketOffset - Get data pointer corresponding to an offset.
//
//  Calculates the data pointer and amount of contiguous data present at
//  that point for a given offset into the packet.  Note that this offset
//  is from the current packet position, NOT the beginning of the packet.
//
void
GetDataAtPacketOffset(
    IPv6Packet *Packet,  // Packet (single, not chain!) in question.
    int Offset,          // Offset into packet to get data pointer for.
    void **Data,         // Where to return data pointer.
    uint *ContigSize)    // Where to return size of contiguous data region.
{
    PNDIS_BUFFER Buffer;
    void *VirtAddr;
    uint BufLen, TotLen, Current, Contig;

    //
    // Offset is relative to current packet position.
    //
    Offset += Packet->Position;
    ASSERT(Offset >= 0);

    ASSERT(Packet->NdisPacket != NULL);

    //
    // Run down secondary buffer chain until we have reached
    // the buffer corresponding to the desired offset.
    //
    NdisGetFirstBufferFromPacket(Packet->NdisPacket, &Buffer, &VirtAddr,
                                 &BufLen, &TotLen);
    ASSERT((uint)Offset <= TotLen);
    Current = BufLen;
    while (Current <= (uint)Offset) {
        NdisGetNextBuffer(Buffer, &Buffer);
        ASSERT(Buffer != NULL);
        NdisQueryBuffer(Buffer, &VirtAddr, &BufLen);
        Current += BufLen;
    }

    Contig = Current - Offset;
    *ContigSize = Contig;
    *Data = (uchar *)VirtAddr + (BufLen - Contig);
}


//* CopyToBufferChain - Copy received packet to NDIS buffer chain.
//
//  Copies from a received packet to an NDIS buffer chain.
//  The received packet data comes in two flavors: if SrcPacket is
//  NULL, then SrcData is used. SrcOffset specifies an offset
//  into SrcPacket; it is only used when SrcPacket is non-NULL.
//
//  Length limits the number of bytes copied. The number of bytes
//  copied may also be limited by the destination & source.
//
uint  // Returns: number of bytes copied.
CopyToBufferChain(
    PNDIS_BUFFER DstBuffer,
    uint DstOffset,
    PNDIS_PACKET SrcPacket,
    uint SrcOffset,
    uchar *SrcData,
    uint Length)
{
    PNDIS_BUFFER SrcBuffer;
    uchar *DstData;
    uint DstSize, SrcSize;
    uint BytesCopied, BytesToCopy;

    //
    // Skip DstOffset bytes in the destination buffer chain.
    // NB: DstBuffer might be NULL to begin with; that's legitimate.
    //
    for (;;) {
        if (DstBuffer == NULL)
            return 0;

        NdisQueryBuffer(DstBuffer, &DstData, &DstSize);

        if (DstOffset < DstSize) {
            DstData += DstOffset;
            DstSize -= DstOffset;
            break;
        }

        DstOffset -= DstSize;
        NdisGetNextBuffer(DstBuffer, &DstBuffer);
    }

    if (SrcPacket != NULL) {
        //
        // Skip SrcOffset bytes into SrcPacket.
        // NB: SrcBuffer might be NULL to begin with; that's legitimate.
        //
        NdisQueryPacket(SrcPacket, NULL, NULL, &SrcBuffer, NULL);

        for (;;) {
            if (SrcBuffer == NULL)
                return 0;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);

            if (SrcOffset < SrcSize) {
                SrcData += SrcOffset;
                SrcSize -= SrcOffset;
                break;
            }

            SrcOffset -= SrcSize;
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        }
    } else {
        //
        // Using SrcData instead of SrcPacket/SrcOffset.
        // In this case, we need not initialize SrcBuffer
        // because the copy loop below will never attempt
        // to advance to another SrcBuffer.
        //
        SrcSize = Length;
    }

    //
    // Perform the copy, advancing DstBuffer and SrcBuffer as needed.
    // Normally Length is initially non-zero, so no reason
    // to check Length first.
    //
    for (BytesCopied = 0;;) {

        BytesToCopy = MIN(MIN(Length, SrcSize), DstSize);
        RtlCopyMemory(DstData, SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;

        Length -= BytesToCopy;
        if (Length == 0)
            break;  // All done.

        DstData += BytesToCopy;
        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            //
            // We ran out of room in our current destination buffer.
            // Proceed to next buffer on the chain.
            //
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (DstBuffer == NULL)
                break;

            NdisQueryBuffer(DstBuffer, &DstData, &DstSize);
        }

        SrcData += BytesToCopy;
        SrcSize -= BytesToCopy;
        if (SrcSize == 0) {
            //
            // We ran out of data in our current source buffer.
            // Proceed to the next buffer on the chain.
            //
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (SrcBuffer == NULL)
                break;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        }
    }

    return BytesCopied;
}


//* CopyPacketToNdis - Copy from an IPv6Packet chain to an NDIS buffer.
//
//  This is the function we use to copy from a chain of IPv6Packets
//  to ONE NDIS buffer.  The caller specifies the source and
//  destination, a maximum size to copy, and an offset into the first
//  packet to start copying from.  We copy as much as possible up to the
//  size, and return the size copied.
//
//  Note that SrcOffset is relative to the beginning of the first packet in
//  the chain, and NOT the current 'Position' in that packet.
//
//  The source packet chain is not modified in any way by this routine.
//
uint  // Returns: Bytes copied.
CopyPacketToNdis(
    PNDIS_BUFFER DestBuf,  // Destination NDIS buffer chain.
    IPv6Packet *SrcPkt,    // Source packet chain.
    uint Size,             // Size in bytes to copy.
    uint DestOffset,       // Offset into dest buffer to start copying to.
    uint SrcOffset)        // Offset into packet chain to copy from.
{
    uint TotalBytesCopied = 0;  // Bytes we've copied so far.
    uint BytesCopied;           // Bytes copied out of each buffer.
    uint DestSize;              // Space left in destination.
    void *SrcData;              // Current source data pointer.
    uint SrcContig;             // Amount of Contiguous data from SrcData on.
    PNDIS_BUFFER SrcBuf;        // Current buffer in current packet.
    uint PacketSize;            // Total size of current packet.
    NTSTATUS Status;


    ASSERT(SrcPkt != NULL);

    //
    // The destination buffer can be NULL - this is valid, if odd.
    //
    if (DestBuf == NULL)
        return 0;

    //
    // Limit our copy to the smaller of the requested amount and the
    // available space in the destination buffer.
    //
    NdisQueryBuffer(DestBuf, NULL, &DestSize);
    DestSize = MIN(DestSize, Size);

    //
    // First, skip SrcOffset bytes into the source packet chain.
    //
    if ((SrcOffset == SrcPkt->Position) && (Size <= SrcPkt->ContigSize)) {
        //
        // One common case is that we want to start from the current Position.
        // REVIEW: This case common enough to be worth this check?
        //
        SrcContig = SrcPkt->ContigSize;
        SrcData = SrcPkt->Data;
        SrcBuf = NULL;
        PacketSize = SrcPkt->TotalSize;
    } else {
        //
        // Otherwise step through packets and buffer regions until
        // we find the desired spot.
        //
        PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        while (SrcOffset >= PacketSize) {
            // Skip a whole packet.
            SrcOffset -= PacketSize;
            SrcPkt = SrcPkt->Next;
            ASSERT(SrcPkt != NULL);
            PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        }
        //
        // Found the right packet in the chain, now find desired buffer.
        //
        PacketSize -= SrcOffset;
        if (SrcPkt->NdisPacket == NULL) {
            //
            // This packet must be just a single contiguous region.
            // Finding the right spot is a simple matter of arithmetic.
            //
            SrcContig = PacketSize;
            SrcData = (uchar *)SrcPkt->Data - SrcPkt->Position + SrcOffset;
            SrcBuf = NULL;
        } else {
            uchar *BufAddr;
            uint BufLen, TotLen;

            //
            // There may be multiple buffers comprising this packet.
            // Step through them until we arrive at the right spot.
            //
            NdisGetFirstBufferFromPacket(SrcPkt->NdisPacket, &SrcBuf, &BufAddr,
                                         &BufLen, &TotLen);
            ASSERT(SrcOffset < TotLen);
            ASSERT(PacketSize + SrcOffset <= TotLen);
            while (SrcOffset >= BufLen) {
                // Skip to the next buffer.
                SrcOffset -= BufLen;
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            }
            SrcContig = BufLen - SrcOffset;
            SrcData = BufAddr + BufLen - SrcContig;
        }
    }

    //
    // We're now at the point where we wish to start copying.
    //
    while (DestSize != 0) {
        uint BytesToCopy;

        BytesToCopy = MIN(DestSize, SrcContig);
        Status = TdiCopyBufferToMdl(SrcData, 0, BytesToCopy,
                                    DestBuf, DestOffset, &BytesCopied);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        ASSERT(BytesCopied == BytesToCopy);
        TotalBytesCopied += BytesToCopy;

        if (BytesToCopy < DestSize) {
            //
            // Not done yet, we ran out of either source packet or buffer.
            // Get next one and fix up pointers/sizes for the next copy.
            //
            DestBuf += BytesToCopy;
            PacketSize -= BytesToCopy;
            if (PacketSize == 0) {
                // Get next packet on chain.
                SrcPkt = SrcPkt->Next;
                ASSERT(SrcPkt != NULL);
                PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
                if (SrcPkt->NdisPacket == NULL) {
                    // Single contiguous region.
                    SrcData = SrcPkt->Data;
                    SrcContig = SrcPkt->ContigSize;
                } else {
                    // Potentially multiple buffers.
                    uint Foo;
                    NdisGetFirstBufferFromPacket(SrcPkt->NdisPacket, &SrcBuf,
                                                 &SrcData, &SrcContig, &Foo);
                    ASSERT(PacketSize <= Foo);
                }
            } else {
                // Get next buffer in packet.
                ASSERT(SrcBuf != NULL);
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
            }
        }
        DestSize -= BytesToCopy;
    }

    return TotalBytesCopied;
}


//* CopyPacketToBuffer - Copy from an IPv6Packet chain to a flat buffer.
//
//  Called during receive processing to copy from an IPv6Packet chain to a
//  flat buffer.  We skip SrcOffset bytes into the source chain, and then
//  copy Size bytes.
//
//  Note that SrcOffset is relative to the beginning of the packet, NOT
//  the current 'Position'.
//
//  The source packet chain is not modified in any way by this routine.
//
void  // Returns: Nothing.
CopyPacketToBuffer(
    uchar *DestBuf,      // Destination buffer (unstructured memory).
    IPv6Packet *SrcPkt,  // Source packet chain.
    uint Size,           // Size in bytes to copy.
    uint SrcOffset)      // Offset in SrcPkt to start copying from.
{
    uint SrcContig;
    void *SrcData;
    PNDIS_BUFFER SrcBuf;
    uint PacketSize;

#if DBG
    IPv6Packet *TempPkt;
    uint TempSize;
#endif

    ASSERT(DestBuf != NULL);
    ASSERT(SrcPkt != NULL);

#if DBG
    //
    // In debug versions check to make sure we're copying a reasonable size
    // and from a reasonable offset.
    //
    TempPkt = SrcPkt;
    TempSize = TempPkt->Position + TempPkt->TotalSize;
    TempPkt = TempPkt->Next;
    while (TempPkt != NULL) {
        TempSize += TempPkt->TotalSize;
        TempPkt = TempPkt->Next;
    }

    ASSERT(SrcOffset <= TempSize);
    ASSERT((SrcOffset + Size) <= TempSize);
#endif

    //
    // First, skip SrcOffset bytes into the source packet chain.
    //
    if ((SrcOffset == SrcPkt->Position) && (Size <= SrcPkt->ContigSize)) {
        //
        // One common case is that we want to start from the current Position.
        // REVIEW: This case common enough to be worth this check?
        //
        SrcContig = SrcPkt->ContigSize;
        SrcData = SrcPkt->Data;
        SrcBuf = NULL;
        PacketSize = SrcPkt->TotalSize;
    } else {
        //
        // Otherwise step through packets and buffer regions until
        // we find the desired spot.
        //
        PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        while (SrcOffset >= PacketSize) {
            // Skip a whole packet.
            SrcOffset -= PacketSize;
            SrcPkt = SrcPkt->Next;
            ASSERT(SrcPkt != NULL);
            PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        }
        //
        // Found the right packet in the chain, now find desired buffer.
        //
        PacketSize -= SrcOffset;
        if (SrcPkt->NdisPacket == NULL) {
            //
            // This packet must be just a single contiguous region.
            // Finding the right spot is a simple matter of arithmetic.
            //
            SrcContig = PacketSize;
            SrcData = (uchar *)SrcPkt->Data - SrcPkt->Position + SrcOffset;
            SrcBuf = NULL;
        } else {
            uchar *BufAddr;
            uint BufLen, TotLen;

            //
            // There may be multiple buffers comprising this packet.
            // Step through them until we arrive at the right spot.
            //
            NdisGetFirstBufferFromPacket(SrcPkt->NdisPacket, &SrcBuf, &BufAddr,
                                         &BufLen, &TotLen);
            ASSERT(SrcOffset < TotLen);
            ASSERT(PacketSize + SrcOffset <= TotLen);
            while (SrcOffset >= BufLen) {
                // Skip to the next buffer.
                SrcOffset -= BufLen;
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            }
            SrcContig = BufLen - SrcOffset;
            SrcData = BufAddr + BufLen - SrcContig;
        }
    }

    //
    // We're now at the point where we wish to start copying.
    //
    while (Size != 0) {
        uint BytesToCopy;

        BytesToCopy = MIN(Size, SrcContig);
        RtlCopyMemory(DestBuf, (uchar *)SrcData, BytesToCopy);

        if (BytesToCopy < Size) {
            //
            // Not done yet, we ran out of either source packet or buffer.
            // Get next one and fix up pointers/sizes for the next copy.
            //
            DestBuf += BytesToCopy;
            PacketSize -= BytesToCopy;
            if (PacketSize == 0) {
                // Get next packet on chain.
                SrcPkt = SrcPkt->Next;
                ASSERT(SrcPkt != NULL);
                PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
                if (SrcPkt->NdisPacket == NULL) {
                    // Single contiguous region.
                    SrcData = SrcPkt->Data;
                    SrcContig = SrcPkt->ContigSize;
                } else {
                    // Potentially multiple buffers.
                    uint Foo;
                    NdisGetFirstBufferFromPacket(SrcPkt->NdisPacket, &SrcBuf,
                                                 &SrcData, &SrcContig, &Foo);
                    ASSERT(PacketSize <= Foo);
                }
            } else {
                // Get next buffer in packet.
                ASSERT(SrcBuf != NULL);
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
            }
        }
        Size -= BytesToCopy;
    }
}


//* CopyFlatToNdis - Copy a flat buffer to an NDIS_BUFFER chain.
//
//  A utility function to copy a flat buffer to an NDIS buffer chain.  We
//  assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
//  in a debug build we'll debugcheck if this isn't true.  We return a pointer
//  to the buffer where we stopped copying, and an offset into that buffer.
//  This is useful for copying in pieces into the chain.
//
PNDIS_BUFFER  // Returns: Pointer to next buffer in chain to copy into.
CopyFlatToNdis(
    PNDIS_BUFFER DestBuf,  // Destination NDIS buffer chain.
    uchar *SrcBuf,         // Source buffer (unstructured memory).
    uint Size,             // Size in bytes to copy.
    uint *StartOffset,     // Pointer to Offset info first buffer in chain.
                           // Filled on return with offset to copy into next.
    uint *BytesCopied)     // Location into which to return # of bytes copied.
{
    NTSTATUS Status = 0;

    *BytesCopied = 0;

    Status = TdiCopyBufferToMdl(SrcBuf, 0, Size, DestBuf, *StartOffset,
                                BytesCopied);

    *StartOffset += *BytesCopied;

    //
    // Always return the first buffer, since the TdiCopy function handles
    // finding the appropriate buffer based on offset.
    //
    return(DestBuf);
}


//* CopyNdisToFlat - Copy an NDIS_BUFFER chain to a flat buffer.
//
//  Copy (a portion of) an NDIS buffer chain to a flat buffer.
//  Returns the Next buffer/offset, for subsequent calls.
//
void
CopyNdisToFlat(
    void *DstData,
    PNDIS_BUFFER SrcBuffer,
    uint SrcOffset,
    uint Length,
    PNDIS_BUFFER *NextBuffer,
    uint *NextOffset)
{
    void *SrcData;
    uint SrcSize;
    uint Bytes;

    for (;;) {
        NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        if (SrcSize < SrcOffset) {
            SrcOffset -= SrcSize;
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            continue;
        }

        Bytes = SrcSize - SrcOffset;
        if (Bytes > Length)
            Bytes = Length;

        RtlCopyMemory(DstData, (uchar *)SrcData + SrcOffset, Bytes);

        (uchar *)DstData += Bytes;
        SrcOffset += Bytes;
        Length -= Bytes;

        if (Length == 0)
            break;

        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        SrcOffset = 0;
    }

    *NextBuffer = SrcBuffer;
    *NextOffset = SrcOffset;
}


//
// Checksum support.
// On NT, there are architecture-specific assembly routines for the core
// calculation.
//

//* ChecksumPacket - Calculate the Internet checksum of a packet.
//
//  Calculates the checksum of packet data. The data may be supplied
//  either with the Packet/Offset arguments, or (if Packet is NULL)
//  the Data argument. In either case, Length specifies how much
//  data to checksum.
//
//  The Packet is assumed to contain (at least) Offset + Length bytes.
//
//  Also calculates and adds-in the pseudo-header checksum,
//  using IP->Source, IP->Dest, Length, and NextHeader.
//  (With options, we may have IP->NextHeader != NextHeader.)
//
ushort
ChecksumPacket(
    PNDIS_PACKET Packet,  // Packet with data to checksum.
    uint Offset,          // Offset into packet where data starts.
    uchar *Data,          // If Packet is NULL, data to checksum.
    uint Length,          // Length of packet data.
    IPv6Addr *Source,     // Source address.
    IPv6Addr *Dest,       // Destination address.
    uchar NextHeader)     // Protocol type for pseudo-header.
{
    PNDIS_BUFFER Buffer;
    uint Checksum;
    uint PayloadLength;
    uint Size;
    uint TotalSummed;

    //
    // Start with the pseudo-header.
    //
    Checksum = Cksum(Source, sizeof *Source) + Cksum(Dest, sizeof *Dest);
    PayloadLength = net_long(Length);
    Checksum += (PayloadLength >> 16) + (PayloadLength & 0xffff);
    Checksum += (NextHeader << 8);

    if (Packet == NULL) {
        //
        // We do not have to initialize Buffer.
        // The checksum loop below will exit before trying to use it.
        //
        Size = Length;
    } else {
        UINT Unused;

        //
        // Skip over Offset bytes in the packet.
        //

        NdisGetFirstBufferFromPacket(Packet, &Buffer, &Data, &Size, &Unused);
        for (;;) {

            //
            // There is a boundary case here: the Packet contains
            // exactly Offset bytes total, and Length is zero.
            // Checking Offset <= Size instead of Offset < Size
            // makes this work.
            //
            if (Offset <= Size) {
                Data += Offset;
                Size -= Offset;
                break;
            }

            Offset -= Size;
            NdisGetNextBuffer(Buffer, &Buffer);
            NdisQueryBuffer(Buffer, &Data, &Size);
        }
    }
    for (TotalSummed = 0;;) {
        ushort Temp;

        //
        // Size might be bigger than we need,
        // if there is "extra" data in the packet.
        //
        if (Size > Length)
            Size = Length;

        Temp = Cksum(Data, Size);
        if (TotalSummed & 1) {
            // We're at an odd offset into the logical buffer,
            // so we need to swap the bytes that Cksum returns.
            Checksum += (Temp >> 8) + ((Temp & 0xff) << 8);
        } else {
            Checksum += Temp;
        }
        TotalSummed += Size;

        Length -= Size;
        if (Length == 0)
            break;
        NdisGetNextBuffer(Buffer, &Buffer);
        NdisQueryBuffer(Buffer, &Data, &Size);
    }

    //
    // Wrap in the carries to reduce Checksum to 16 bits.
    // (Twice is sufficient because it can only overflow once.)
    //
    Checksum = (Checksum >> 16) + (Checksum & 0xffff);
    Checksum += (Checksum >> 16);

    //
    // Take ones-complement and replace 0 with 0xffff.
    //
    Checksum = (ushort) ~Checksum;
    if (Checksum == 0)
        Checksum = 0xffff;

    return (ushort) Checksum;
}

//* ConvertSecondsToTicks
//
//  Convert seconds to timer ticks.
//  A value of INFINITE_LIFETIME (0xffffffff) indicates infinity.
//
uint
ConvertSecondsToTicks(uint Seconds)
{
    uint Ticks;

    //
    // BUGBUG: Overflow?
    //
    if (Seconds == INFINITE_LIFETIME)
        Ticks = INFINITE_LIFETIME;
    else
        Ticks = IPv6TimerTicks(Seconds);

    return Ticks;
}

//* ConvertTicksToSeconds
//
//  Convert timer ticks to seconds.
//  A value of INFINITE_LIFETIME (0xffffffff) indicates infinity.
//
uint
ConvertTicksToSeconds(uint Ticks)
{
    uint Seconds;

    if (Ticks == INFINITE_LIFETIME)
        Seconds = INFINITE_LIFETIME;
    else
        Seconds = Ticks / IPv6_TICKS_SECOND;

    return Seconds;
}

//* ConvertMillisToTicks
//
//  Convert milliseconds to timer ticks.
//
uint
ConvertMillisToTicks(uint Millis)
{
    uint Ticks;

    //
    // BUGBUG: Overflow/underflow?
    //
    Ticks = IPv6TimerTicks(Millis) / 1000;
    if (Ticks == 0 && Millis != 0)
        Ticks = 1;

    return Ticks;
}

//* IPv6Timeout - Perform various housekeeping duties periodically.
//
//  Neighbor discovery, fragment reassembly, ICMP ping, etc. all have
//  time-dependent parts.  Check for timer expiration here.
//
void
IPv6Timeout(
    PKDPC MyDpcObject,  // The DPC object describing this routine.
    void *Context,      // The argument we asked to be called with.
    void *Unused1,
    void *Unused2)
{
    UNREFERENCED_PARAMETER(MyDpcObject);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Unused1);
    UNREFERENCED_PARAMETER(Unused2);

    //
    // Atomically increment our tick count.
    //
    InterlockedIncrement((LONG *)&IPv6TickCount);

    //
    // Handle per-interface timeouts.
    //
    InterfaceTimeout();

    // 
    // Process all multicast groups with timers running.  Timers are used to
    // response to membership queries sent to us by the first-hop router.
    //
    // We call MLDTimeout *before* NetTableTimeout,
    // so that when an interface is first created and a link-local address
    // is assigned, the initial MLD Report for the solicited-node multicast
    // address gets sent *before* the Neighbor Solicit for DAD.
    //
    if (QueryList != NULL)
        MLDTimeout();

    //
    // Handle per-NTE timeouts.
    //
    NetTableTimeout();

    //
    // Handle routing table timeouts.
    //
    RouteTableTimeout();

    //
    // If there's a possibility we have one or more outstanding echo requests,
    // call out to ICMPv6 to handle them.  Note that since we don't grab the
    // lock here, there may be none by the time we get there.  This just saves
    // us from always having to call out.
    //
    if (ICMPv6OutstandingEchos != NULL) {
        //
        // Echo requests outstanding.
        //
        ICMPv6EchoTimeout();
    }

    //
    // If we might have active reassembly records,
    // call out to handle timeout processing for them.
    //
    if (ReassemblyList.First != SentinelReassembly) {
        ReassemblyTimeout();
    }

    //
    // Check for expired binding cache entries.
    //
    if (BindingCache != NULL)
        BindingCacheTimeout();

    //
    // Check for expired site prefixes.
    //
    if (SitePrefixTable != NULL)
        SitePrefixTimeout();
}

//* AdjustPacketBuffer
//
//  Takes an NDIS Packet that has some spare bytes available
//  at the beginning and adjusts the size of that available space.
//
//  When we allocate packets, we often do not know a priori on which
//  link the packets will go out.  However it is much more efficient
//  to allocate space for the link-level header along with the rest
//  of the packet.  Hence we leave space for the maximum link-level header,
//  and each individual link layer uses AdjustPacketBuffer to shrink
//  that space to the size that it really needs.
//
//  AdjustPacketBuffer is needed because the sending calls (in both
//  the NDIS and TDI interfaces) do not allow the caller to specify
//  an offset of data to skip over.
//
//  Note that this code is NT-specific, because it knows about the
//  internal fields of NDIS_BUFFER structures.
//
void *
AdjustPacketBuffer(
    PNDIS_PACKET Packet,  // Packet to adjust.
    uint SpaceAvailable,  // Extra space available at start of first buffer.
    uint SpaceNeeded)     // Amount of space we need for the header.
{
    PMDL Buffer;
    uint Adjust;

    // Get first buffer on packet chain.
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);

    //
    // The remaining space in the packet should all be in the first buffer.
    //
    ASSERT(SpaceAvailable <= Buffer->ByteCount);

    Adjust = SpaceAvailable - SpaceNeeded;
    if (Adjust == 0) {
        //
        // There is exactly the right amount of space left.
        // This is the common case.
        //
    } else if ((int)Adjust > 0) {
        //
        // There is too much space left.
        // Because NdisSend doesn't have an Offset argument,
        // we need to temporarily "shrink" the buffer.
        //
        (uchar *)Buffer->MappedSystemVa += Adjust;
        Buffer->ByteCount -= Adjust;
        Buffer->ByteOffset += Adjust;

        if (Buffer->ByteOffset >= PAGE_SIZE) {
            ULONG FirstPage;

            //
            // Need to "remove" the first physical page
            // by shifting the array up one page.
            // Save it at the end of the array.
            //
            FirstPage = ((PULONG)(Buffer + 1))[0];
            RtlMoveMemory(&((PULONG)(Buffer + 1))[0],
                          &((PULONG)(Buffer + 1))[1],
                          Buffer->Size - sizeof *Buffer - sizeof(ULONG));
            ((PULONG)((uchar *)Buffer + Buffer->Size))[-1] = FirstPage;

            (uchar *)Buffer->StartVa += PAGE_SIZE;
            Buffer->ByteOffset -= PAGE_SIZE;
        }
    } else { // Adjust < 0
        //
        // Not enough space.
        // Shouldn't happen in the normal send path.
        // REVIEW: This is a potential problem when forwarding packets
        // from an interface with a short link-level header
        // to an interface with a longer link-level header.
        // Should the forwarding code take care of this?
        //
        ASSERTMSG("AdjustPacketBuffer: Adjust < 0", FALSE);
    }

    //
    // Save away the adjustment for the completion callback,
    // which needs to undo our work with UndoAdjustPacketBuffer.
    //
    PC(Packet)->pc_offset = Adjust;

    //
    // Return a pointer to the buffer.
    //
    return Buffer->MappedSystemVa;
}

//* UndoAdjustPacketBuffer
//
//  Undo the effects of AdjustPacketBuffer.
//
//  Note that this code is NT-specific, because it knows about the
//  internal fields of NDIS_BUFFER structures.
//
void
UndoAdjustPacketBuffer(
    PNDIS_PACKET Packet)  // Packet we may or may not have previously adjusted.
{
    uint Adjust;

    Adjust = PC(Packet)->pc_offset;
    if (Adjust != 0) {
        PMDL Buffer;

        //
        // We need to undo the adjustment made in AdjustPacketBuffer.
        // This may including shifting the array of page info.
        //

        // Get first buffer on packet chain.
        NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);

        if (Buffer->ByteOffset < Adjust) {
            ULONG FirstPage;

            (uchar *)Buffer->StartVa -= PAGE_SIZE;
            Buffer->ByteOffset += PAGE_SIZE;

            FirstPage = ((PULONG)((uchar *)Buffer + Buffer->Size))[-1];
            RtlMoveMemory(&((PULONG)(Buffer + 1))[1],
                          &((PULONG)(Buffer + 1))[0],
                          Buffer->Size - sizeof *Buffer - sizeof(ULONG));
            ((PULONG)(Buffer + 1))[0] = FirstPage;
        }

        (uchar *)Buffer->MappedSystemVa -= Adjust;
        Buffer->ByteCount += Adjust;
        Buffer->ByteOffset -= Adjust;
    }
}

//* CreateSolicitedNodeMulticastAddress
//
//  Given a unicast or anycast address, creates the corresponding
//  solicited-node multicast address.
//
void
CreateSolicitedNodeMulticastAddress(
    IPv6Addr *Addr,
    IPv6Addr *MCastAddr)
{
    RtlZeroMemory(MCastAddr, sizeof *MCastAddr);
    MCastAddr->u.Byte[0] = 0xff;
    MCastAddr->u.Byte[1] = ADE_LINK_LOCAL;
    MCastAddr->u.Byte[11] = 0x01;
    MCastAddr->u.Byte[12] = 0xff;
    MCastAddr->u.Byte[13] = Addr->u.Byte[13];
    MCastAddr->u.Byte[14] = Addr->u.Byte[14];
    MCastAddr->u.Byte[15] = Addr->u.Byte[15];
}

//* CreateV4CompatibleAddress
//
//  Given a v4 address, creates the corresponding IPv6
//  v4-compatible address.
//
void
CreateV4CompatibleAddress(
    IPv6Addr *Addr,
    IPAddr V4Addr)
{
    Addr->u.DWord[0] = 0;
    Addr->u.DWord[1] = 0;
    Addr->u.DWord[2] = 0;
    Addr->u.DWord[3] = V4Addr;
}

//* IsV4Compatible
//
//  Is this a v4-compatible address?
//
//  NB: Returns FALSE for the unspecified and loopback addresses.
//
int
IsV4Compatible(IPv6Addr *Addr)
{
    return ((Addr->u.DWord[0] == 0) &&
            (Addr->u.DWord[1] == 0) &&
            (Addr->u.DWord[2] == 0) &&
            (Addr->u.DWord[3] != 0) &&
            (Addr->u.DWord[3] != 0x01000000));
}

//* IsSolicitedNodeMulticast
//
//  Is this a solicited-node multicast address?
//  Checks very strictly for the proper format.
//  For example scope values smaller than 2 are not allowed.
//
int
IsSolicitedNodeMulticast(IPv6Addr *Addr)
{
    return ((Addr->u.Byte[0] == 0xff) &&
            (Addr->u.Byte[1] == ADE_LINK_LOCAL) &&
            (Addr->u.Word[1] == 0) &&
            (Addr->u.DWord[1] == 0) &&
            (Addr->u.Word[4] == 0) &&
            (Addr->u.Byte[10] == 0) &&
            (Addr->u.Byte[11] == 0x01) &&
            (Addr->u.Byte[12] == 0xff));
}

//* IsUniqueAddress
//
//  As best we can tell statically,
//  does this address uniquely specify a destination node?
//
int
IsUniqueAddress(IPv6Addr *Addr)
{
    //
    // BUGBUG: There are more reserved anycast addresses being specified.
    // REVIEW: Should we scan our ADEs looking for anycast addresses,
    // to fulfill the requirement of catching known anycast addresses?
    //
    return (!IsUnspecified(Addr) &&
            !IsMulticast(Addr) &&
            !IsSubnetRouterAnycast(Addr));
}

//* DetermineScopeId
//
//  Given an address and an associated interface, determine
//  the appropriate value for the scope identifier.
//
//  Returns the ScopeId.
//
uint
DetermineScopeId(IPv6Addr *Addr, Interface *IF)
{
    if (IsLinkLocal(Addr)) {
        // 
        // Address is link-specific.
        // Use interface index as scope identifier.
        //
        return IF->Index;
    }
    if (IsSiteLocal(Addr)) {
        //
        // Address is specific to some organizational boundaries.
        // Use the interface's site identifier as the scope identifier.
        //
        return IF->Site;
    }
    //
    // Address isn't "scoped".
    //
    return 0;
}

//* HasPrefix - Does an address have the given prefix?
//
int
HasPrefix(IPv6Addr *Addr, IPv6Addr *Prefix, uint PrefixLength)
{
    uchar *AddrBytes = Addr->u.Byte;
    uchar *PrefixBytes = Prefix->u.Byte;

    //
    // Check that initial integral bytes match.
    //
    while (PrefixLength > 8) {
        if (*AddrBytes++ != *PrefixBytes++)
            return FALSE;
        PrefixLength -= 8;
    }

    //
    // Check any remaining bits.
    // Note that if PrefixLength is zero now, we should not
    // dereference AddrBytes/PrefixBytes.
    //
    if ((PrefixLength > 0) &&
        ((*AddrBytes >> (8 - PrefixLength)) !=
         (*PrefixBytes >> (8 - PrefixLength))))
        return FALSE;

    return TRUE;
}

//* CopyPrefix
//
//  Copy an address prefix, zeroing the remaining bits
//  in the destination address.
//
void
CopyPrefix(IPv6Addr *Addr, IPv6Addr *Prefix, uint PrefixLength)
{
    uint PLBytes, PLRemainderBits, Loop;

    PLBytes = PrefixLength / 8;
    PLRemainderBits = PrefixLength % 8;
    for (Loop = 0; Loop < sizeof(IPv6Addr); Loop++) {
        if (Loop < PLBytes)
            Addr->u.Byte[Loop] = Prefix->u.Byte[Loop];
        else
            Addr->u.Byte[Loop] = 0;
    }
    if (PLRemainderBits) {
        Addr->u.Byte[PLBytes] = Prefix->u.Byte[PLBytes] &
            (0xff << (8 - PLRemainderBits));
    }
}

//* CommonPrefixLength
//
//  Calculate the length of the longest prefix common
//  to the two addresses.
//
uint
CommonPrefixLength(IPv6Addr *Addr, IPv6Addr *Addr2)
{
    int i, j;

    //
    // Find first non-matching byte.
    //
    for (i = 0; ; i++) {
        if (i == sizeof(IPv6Addr))
            return 8 * i;

        if (Addr->u.Byte[i] != Addr2->u.Byte[i])
            break;
    }

    //
    // Find first non-matching bit (there must be one).
    //
    for (j = 0; ; j++) {
        uint Mask = 1 << (7 - j);

        if ((Addr->u.Byte[i] & Mask) != (Addr2->u.Byte[i] & Mask))
            break;
    }

    return 8 * i + j;
}

//* GetDataFromNdis
//
//  Retrieves data from an NDIS buffer chain.
//  If the desired data is contiguous, then just returns
//  a pointer directly to the data in the buffer chain.
//  Otherwise the data is copied to the supplied buffer
//  and returns a pointer to the supplied buffer.
//
//  Returns NULL only if the desired data (offset/size)
//  does not exist in the NDIS buffer chain of
//  if DataBuffer is NULL and the data is not contiguous.
//
uchar *
GetDataFromNdis(
    NDIS_BUFFER *SrcBuffer,
    uint SrcOffset,
    uint Length,
    uchar *DataBuffer)
{
    void *DstData;
    void *SrcData;
    uint SrcSize;
    uint Bytes;

    //
    // Look through the buffer chain
    // for the beginning of the desired data.
    //
    for (;;) {
        if (SrcBuffer == NULL)
            return NULL;

        NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        if (SrcOffset < SrcSize)
            break;

        SrcOffset -= SrcSize;
        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
    }

    //
    // If the desired data is contiguous,
    // then just return a pointer to it.
    //
    if (SrcOffset + Length <= SrcSize)
        return (uchar *)SrcData + SrcOffset;

    //
    // If our caller did not specify a buffer,
    // then we must fail.
    //
    if (DataBuffer == NULL)
        return NULL;

    //
    // Copy the desired data to the caller's buffer,
    // and return a pointer to the caller's buffer.
    //
    DstData = DataBuffer;
    for (;;) {
        Bytes = SrcSize - SrcOffset;
        if (Bytes > Length)
            Bytes = Length;

        RtlCopyMemory(DstData, (uchar *)SrcData + SrcOffset, Bytes);

        (uchar *)DstData += Bytes;
        Length -= Bytes;

        if (Length == 0)
            break;

        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        if (SrcBuffer == NULL)
            return NULL;
        NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        SrcOffset = 0;
    }

    return DataBuffer;
}

//* GetIPv6Header
//
//  Returns a pointer to the IPv6 header in an NDIS Packet.
//  If the header is contiguous, then just returns
//  a pointer to the data directly in the packet.
//  Otherwise the IPv6 header is copied to the supplied buffer,
//  and a pointer to the buffer is returned.
//
//  Returns NULL only if the NDIS packet is not big enough
//  to contain a header, or if HdrBuffer is NULL and the header
//  is not contiguous.
//
IPv6Header UNALIGNED *
GetIPv6Header(PNDIS_PACKET Packet, uint Offset, IPv6Header *HdrBuffer)
{
    PNDIS_BUFFER NdisBuffer;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    return (IPv6Header UNALIGNED *)
        GetDataFromNdis(NdisBuffer, Offset, sizeof(IPv6Header),
                        (uchar *) HdrBuffer);
}

//* PacketPullup - extend contiguous data region.
//
//  Pulls up more data from secondary packet buffers to create a contiguous
//  buffer of at least the given amount.
//
//  If a zero size contiguous region is requested, we always pullup the
//  entire next buffer.
//
uint  // Returns: new contiguous amount, or 0 if unable to satisfy request.
PacketPullup(
    IPv6Packet *Packet,  // Packet to pullup.
    uint Needed)         // Minimum amount of contiguous data to return.
{
    PNDIS_BUFFER Buffer;
    void *BufAddr;
    IPv6PacketAuxiliary *Aux;
    uint BufLen, TotLen, Offset, CopyNow, LeftToGo;

    if (Needed == 0) {
        //
        // Treat a request for zero bytes specially -
        // as a request to pullup the entire next buffer.
        // Asking for one byte has this effect
        // with the current implementation.
        //
        Needed = 1;
    }

    //
    // REVIEW: Callers should check these themselves before calling?
    //
    if (Needed <= Packet->ContigSize)
        return Packet->ContigSize;
    if (Needed > Packet->TotalSize)
        return 0;

    //
    // Check to see if we have any more buffers in this packet.
    // If not, we can't do a pullup.
    //
    if (Packet->NdisPacket == NULL)
        return 0;

    //
    // Run down secondary buffer chain until we have reached the buffer
    // containing the current position.  Note that if we entered with
    // the position field pointing one off the end of a buffer (a common
    // case), we'll stop at the beginning of the following buffer. 
    //
    NdisGetFirstBufferFromPacket(Packet->NdisPacket, &Buffer, &BufAddr,
                                 &BufLen, &TotLen);
    ASSERT(Packet->Position <= TotLen);
    for (Offset = BufLen; Offset <= Packet->Position; Offset += BufLen) {
        NdisGetNextBuffer(Buffer, &Buffer);
        NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
    }

    //
    // Reset our data pointer and contiguous region counter to insure
    // they reflect the current position in the secondary buffer chain.
    // 
    LeftToGo = Offset - Packet->Position;
    Packet->Data = (uchar *)BufAddr + (BufLen - LeftToGo);
    Packet->ContigSize = MIN(LeftToGo, Packet->TotalSize);

    //
    // The above repositioning may result in a contiguous region
    // that will satisfy the request.
    //
    if (Needed <= Packet->ContigSize)
        return Packet->ContigSize;

    //
    // Now get the buffer after the one containing the current position.
    //
    NdisGetNextBuffer(Buffer, &Buffer);
    NdisQueryBuffer(Buffer, &BufAddr, &BufLen);

    //
    // There are two possiblities here (depending upon which is MAX below).
    //
    // If this buffer adds enough data to satisfy the request, then we'll
    // return a contiguous region containing the remainder of the previous
    // buffer (from current position onward) plus all of this buffer.
    //
    // Otherwise, we're going to need to run through at least another buffer
    // to satisfy this request.  So that we only have to run down the chain
    // once, we'll limit the pullup to exactly the requested amount.
    //
    Needed = MIN(MAX(Packet->ContigSize + BufLen, Needed), Packet->TotalSize);

    //
    // Allocate and initialize an auxiliary data region.
    // The Data[] field follows the structure in memory.
    //
    Aux = ExAllocatePool(NonPagedPool, Needed + sizeof *Aux);
    if (Aux == NULL)
        return 0;
    Aux->Next = Packet->AuxList;
    Aux->Length = Needed;
    Aux->Position = Packet->Position;
    Packet->AuxList = Aux;

    RtlCopyMemory(Aux->Data, Packet->Data, Packet->ContigSize);
    LeftToGo = Needed - Packet->ContigSize;
    Offset = Packet->ContigSize;
    for (;;) {
        CopyNow = MIN(BufLen, LeftToGo);
        RtlCopyMemory((uchar *)Aux->Data + Offset, BufAddr, CopyNow);
        LeftToGo -= CopyNow;
        if (LeftToGo == 0)
            break;
        Offset += CopyNow;
        NdisGetNextBuffer(Buffer, &Buffer);
        NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
    }
    Packet->ContigSize = Needed;

    //
    // Point our packet's data pointer at the newly allocated
    // contiguous region.
    //
    Packet->Data = Aux->Data;
    return Packet->ContigSize;
}


//* PacketPullupCleanup
//
//  Cleanup auxiliary data regions that were created by PacketPullup.
//
void
PacketPullupCleanup(IPv6Packet *Packet)
{
    while (Packet->AuxList != NULL) {
        IPv6PacketAuxiliary *Aux = Packet->AuxList;
        Packet->AuxList = Aux->Next;
        ExFreePool(Aux);
    }
}


//* AdjustPacketParams
//
//  Adjust the pointers/value in the packet,
//  to move past some bytes in the packet.
//
void
AdjustPacketParams(
    IPv6Packet *Packet,
    uint BytesToSkip)
{
    ASSERT((BytesToSkip <= Packet->ContigSize) &&
           (Packet->ContigSize <= Packet->TotalSize));

    (uchar *)Packet->Data += BytesToSkip;
    Packet->ContigSize -= BytesToSkip;
    Packet->TotalSize -= BytesToSkip;
    Packet->Position += BytesToSkip;
}


//* PositionPacketAt
//
//  Adjust the pointers/values in the packet to reflect being at
//  a specific absolute position in the packet.
//
void
PositionPacketAt(
    IPv6Packet *Packet,
    uint NewPosition)
{
    if (Packet->NdisPacket == NULL) {
        //
        // This packet must be just a single contiguous region.
        // Finding the right spot is a simple matter of arithmetic.
        //
        (uchar *)Packet->Data -= Packet->Position;
        Packet->ContigSize += Packet->Position;
        Packet->TotalSize += Packet->Position;
        Packet->Position = 0;
        AdjustPacketParams(Packet, NewPosition);
    } else {
        PNDIS_BUFFER Buffer;
        void *BufAddr;
        uint BufLen, TotLen, Offset, LeftInBuf;
        
        //
        // There may be multiple buffers comprising this packet.
        // Step through them until we arrive at the right spot.
        //
        NdisGetFirstBufferFromPacket(Packet->NdisPacket, &Buffer, &BufAddr,
                                     &BufLen, &TotLen);
        ASSERT(NewPosition < TotLen);
        for (Offset = BufLen; Offset <= NewPosition; Offset += BufLen) {
            NdisGetNextBuffer(Buffer, &Buffer);
            NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
        }
        LeftInBuf = Offset - NewPosition;
        Packet->Data = (uchar *)BufAddr + (BufLen - LeftInBuf);
        Packet->TotalSize += Packet->Position - NewPosition;
        Packet->ContigSize = MIN(LeftInBuf, Packet->TotalSize);
        Packet->Position = NewPosition;
    }
}


//* GetPacketPositionFromPointer
//
//  Determines the Packet 'Position' (offset from start of packet)
//  corresponding to a given data pointer.
//
//  This isn't very efficient in some cases, so use sparingly.
//
uint
GetPacketPositionFromPointer(
    IPv6Packet *Packet,  // Packet containing pointer we're converting.
    uchar *Pointer)      // Pointer to convert to a position.
{
    PNDIS_BUFFER Buffer;
    uchar *BufAddr;
    uint BufLen, TotLen, Position;
    IPv6PacketAuxiliary *Aux;

    //
    // If the IPv6Packet has no associated NDIS_PACKET, then we only
    // have a single contiguous data region in which to look for the
    // pointer we've been given.
    //
    if (Packet->NdisPacket == NULL) {
        ASSERT(((uchar *)Packet->Data - Packet->Position <= Pointer) &&
               (Pointer < (uchar *)Packet->Data + Packet->ContigSize));
        //
        // Our pointer's position is just the difference between it
        // and the start of the data region.
        //
        return (Pointer - ((uchar *)Packet->Data - Packet->Position));
    }

    //
    // The next place to look is the chain of auxiliary buffers
    // allocated by PacketPullup.
    //
    for (Aux = Packet->AuxList; Aux != NULL; Aux = Aux->Next) {
        if ((Aux->Data <= Pointer) && (Pointer < Aux->Data + Aux->Length))
            return Aux->Position + (Pointer - Aux->Data);
    }

    //
    // The last thing we do is search the NDIS buffer chain.
    // This must succeed.
    //
    NdisGetFirstBufferFromPacket(Packet->NdisPacket, &Buffer, &BufAddr,
                                 &BufLen, &TotLen);
    Position = 0;
    while(!((BufAddr <= Pointer) && (Pointer < BufAddr + BufLen))) {
        Position += BufLen;
        NdisGetNextBuffer(Buffer, &Buffer);
        ASSERT(Buffer != NULL);
        NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
    }

    return (Position + (Pointer - BufAddr));
}


//* InitializePacketFromNdis
//
//  Initialize an IPv6Packet from an NDIS_PACKET.
//
void
InitializePacketFromNdis(
    IPv6Packet *Packet,
    PNDIS_PACKET NdisPacket,
    uint Offset)
{
    PNDIS_BUFFER NdisBuffer;

    RtlZeroMemory(Packet, sizeof *Packet);

    NdisGetFirstBufferFromPacket(NdisPacket, &NdisBuffer,
                                 &Packet->Data,
                                 &Packet->ContigSize,
                                 &Packet->TotalSize);

    AdjustPacketParams(Packet, Offset);
    Packet->NdisPacket = NdisPacket;
}


#include <stdio.h>

//* ParseNumber
//
//  Parses a number, given a base and a maximum.
//  The base should be 10 or 16.
//  The maximum should be 1<<8 or 1<<16.
//
//  Returns FALSE if the parsed number exceeds the maximum.
//
int
ParseNumber(WCHAR *S, uint Base, uint MaxNumber, uint *Number)
{
    uint number;
    uint digit;

    for (number = 0;; S++) {
        if ((L'0' <= *S) && (*S <= L'9'))
            digit = *S - L'0';
        else if (Base == 16) {
            if ((L'a' <= *S) && (*S <= L'f'))
                digit = *S - L'a' + 10;
            else if ((L'A' <= *S) && (*S <= L'F'))
                digit = *S - L'A' + 10;
            else
                break;
        } else
            break;

        number = number * Base + digit;
        if (number >= MaxNumber)
            return FALSE;
    }

    *Number = number;
    return TRUE;
}

//* ParseV6Address
//
//  Parses the string S as an IPv6 address. See RFC 1884.
//  The basic string representation consists of 8 hex numbers
//  separated by colons, with a couple embellishments:
//  - a string of zero numbers (at most one) may be replaced
//  with a double-colon. Double-colons are allowed at beginning/end
//  of the string.
//  - the last 32 bits may be represented in IPv4-style dotted-octet notation.
//  For example,
//     ::
//     ::1
//     ::157.56.138.30
//     ::ffff:156.56.136.75
//     ff01::
//     ff02::2
//     0:1:2:3:4:5:6:7
//  The parser is generally pretty anal about checking the syntax.
//
//  Returns FALSE if there is a syntax error.
//
int
ParseV6Address(
    IN WCHAR *S,
    OUT WCHAR **Terminator,
    OUT IPv6Addr *Addr
    )
{
    enum { Start, InNumber, AfterDoubleColon } state = Start;
    WCHAR *number = NULL;
    int sawHex;
    int numColons = 0, numDots = 0;
    int sawDoubleColon = 0;
    int i = 0;
    WCHAR c;
    uint num;

    // There are a several difficulties here. For one, we don't know
    // when we see a double-colon how many zeroes it represents.
    // So we just remember where we saw it and insert the zeroes
    // at the end. For another, when we see the first digits
    // of a number we don't know if it is hex or decimal. So we
    // remember a pointer to the first character of the number
    // and convert it after we see the following character.

    while (c = *S) {

        switch (state) {
        case Start:
            if (c == L':') {

                // this case only handles double-colon at the beginning

                if (numDots > 0)
                    goto Finish;
                if (numColons > 0)
                    goto Finish;
                if (S[1] != L':')
                    goto Finish;

                sawDoubleColon = 1;
                numColons = 2;
                Addr->u.Word[i++] = 0; // pretend it was 0::
                S++;
                state = AfterDoubleColon;

            } else
        case AfterDoubleColon:
            if ((L'0' <= c) && (c <= L'9')) {

                sawHex = FALSE;
                number = S;
                state = InNumber;

            } else if (((L'a' <= c) && (c <= L'f')) ||
                       ((L'A' <= c) && (c <= L'F'))) {

                if (numDots > 0)
                    goto Finish;

                sawHex = TRUE;
                number = S;
                state = InNumber;

            } else
                goto Finish;
            break;

        case InNumber:
            if ((L'0' <= c) && (c <= L'9')) {

                // remain in InNumber state

            } else if (((L'a' <= c) && (c <= L'f')) ||
                       ((L'A' <= c) && (c <= L'F'))) {

                if (numDots > 0)
                    goto Finish;

                sawHex = TRUE;
                // remain in InNumber state;

            } else if (c == L':') {

                if (numDots > 0)
                    goto Finish;
                if (numColons > 6)
                    goto Finish;

                if (S[1] == L':') {

                    if (sawDoubleColon)
                        goto Finish;
                    if (numColons > 5)
                        goto Finish;

                    sawDoubleColon = numColons+1;
                    numColons += 2;
                    S++;
                    state = AfterDoubleColon;

                } else {
                    numColons++;
                    state = Start;
                }

            } else if (c == L'.') {

                if (sawHex)
                    goto Finish;
                if (numDots > 2)
                    goto Finish;
                if (numColons > 6)
                    goto Finish;
                numDots++;
                state = Start;

            } else
                goto Finish;
            break;
        }

        // If we finished a number, parse it.

        if ((state != InNumber) && (number != NULL)) {

            // Note either numDots > 0 or numColons > 0,
            // because something terminated the number.

            if (numDots == 0) {
                if (!ParseNumber(number, 16, 1<<16, &num))
                    return FALSE;
                Addr->u.Word[i++] = net_short((ushort) num);
            } else {
                if (!ParseNumber(number, 10, 1<<8, &num))
                    return FALSE;
                Addr->u.Byte[2*i + numDots-1] = (uchar) num;
            }
        }

        S++;
    }

Finish:
    // Check that we have a complete address.

    if (numDots == 0)
        ;
    else if (numDots == 3)
        numColons++;
    else
        return FALSE;

    if (sawDoubleColon)
        ;
    else if (numColons == 7)
        ;
    else
        return FALSE;

    // Parse the last number, if necessary.

    if (state == InNumber) {

        if (numDots == 0) {
            if (!ParseNumber(number, 16, 1<<16, &num))
                return FALSE;
            Addr->u.Word[i] = net_short((ushort) num);
        } else {
            if (!ParseNumber(number, 10, 1<<8, &num))
                return FALSE;
            Addr->u.Byte[2*i + numDots] = (uchar) num;
        }

    } else if (state == AfterDoubleColon) {

        Addr->u.Word[i] = 0; // pretend it was ::0

    } else
        return FALSE;

    // Insert zeroes for the double-colon, if necessary.

    if (sawDoubleColon) {

        RtlMoveMemory(&Addr->u.Word[sawDoubleColon + 8 - numColons],
                      &Addr->u.Word[sawDoubleColon],
                      (numColons - sawDoubleColon) * sizeof(ushort));
        RtlZeroMemory(&Addr->u.Word[sawDoubleColon],
                      (8 - numColons) * sizeof(ushort));
    }

    *Terminator = S;
    return TRUE;
}

#if DBG
//* FormatV6AddressWorker - Print an IPv6 address to a buffer.
//
//  We assume the buffer has room for the longest possible address,
//  which is 40 characters.
//
void
FormatV6AddressWorker(char *Sz, IPv6Addr *Addr)
{
    //
    // Check for IPv6-compatible and IPv4-mapped addresses
    //
    if ((Addr->u.Word[0] == 0) && (Addr->u.Word[1] == 0) &&
        (Addr->u.Word[2] == 0) && (Addr->u.Word[3] == 0) &&
        (Addr->u.Word[4] == 0) &&
        ((Addr->u.Word[5] == 0) || (Addr->u.Word[5] == 0xffff)) &&
        (Addr->u.Word[6] != 0)) {

        Sz += sprintf(Sz, "::%s%u.%u.%u.%u",
                      Addr->u.Word[5] == 0 ? "" : "ffff:",
                      Addr->u.Byte[12], Addr->u.Byte[13],
                      Addr->u.Byte[14], Addr->u.Byte[15]);
    } else {
        int maxFirst, maxLast;
        int curFirst, curLast;
        int i;

        //
        // Find largest contiguous substring of zeroes
        // A substring is [First, Last), so it's empty if First == Last.
        //

        maxFirst = maxLast = 0;
        curFirst = curLast = 0;

        for (i = 0; i < 8; i++) {

            if (Addr->u.Word[i] == 0) {
                // Extend current substring
                curLast = i+1;

                // Check if current is now largest
                if (curLast - curFirst > maxLast - maxFirst) {

                    maxFirst = curFirst;
                    maxLast = curLast;
                }
            } else {
                // Start a new substring
                curFirst = curLast = i+1;
            }
        }

        //
        // Ignore a substring of length 1.
        //
        if (maxLast - maxFirst <= 1)
            maxFirst = maxLast = 0;

        //
        // Write colon-separated words.
        // A double-colon takes the place of the longest string of zeroes.
        // All zeroes is just "::".
        //
        for (i = 0; i < 8; i++) {
            uint word;

            // Skip over string of zeroes
            if ((maxFirst <= i) && (i < maxLast)) {

                Sz += sprintf(Sz, "::");
                i = maxLast-1;
                continue;
            }

            // Need colon separator if not at beginning.
            if ((i != 0) && (i != maxLast))
                Sz += sprintf(Sz, ":");

            // Esthetically, I don't like 3 digit words.
            word = net_short(Addr->u.Word[i]);
            Sz += sprintf(Sz, word < 0x100 ? "%x" : "%04x", word);
        }
    }
}

//* FormatV6Address - Print an IPv6 address to a buffer.
//
//  Returns a static buffer containing the address.
//  Because the static buffer is not locked,
//  this function is only useful for debug prints.
//
//  Returns char * instead of WCHAR * because %ws
//  is not usable at DPC level in DbgPrint.
//
char *
FormatV6Address(IPv6Addr *Addr)
{
    static char Buffer[40];

    FormatV6AddressWorker(Buffer, Addr);
    return Buffer;
}
#endif // DBG


#if DBG
long DebugLogSlot = 0;
struct DebugLogEntry DebugLog[DEBUG_LOG_SIZE];

//* LogDebugEvent - Make an entry in our debug log that some event happened.
//
// The debug event log allows for "real time" logging of events
// in a circular queue kept in non-pageable memory.  Each event consists
// of an id number and a arbitrary 32 bit value.
//
void
LogDebugEvent(uint Event,  // The event that took place.
              int Arg)     // Any interesting 32 bits associated with event.
{
    uint Slot;

    //
    // Get the next slot in the log in a muliprocessor safe manner.
    //
    Slot = InterlockedIncrement(&DebugLogSlot) & (DEBUG_LOG_SIZE - 1);

    //
    // Add this event to the log along with a timestamp.
    //
    KeQueryTickCount(&DebugLog[Slot].Time);
    DebugLog[Slot].Event = Event;
    DebugLog[Slot].Arg = Arg;
}

#endif // DBG
