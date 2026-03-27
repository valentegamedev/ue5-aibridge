// Fill out your copyright notice in the Description page of Project Settings.


#include "Util/OggDebugUtils.h"

TArray<uint8> OggDebugUtils::ContinuedPacket;

OggDebugUtils::OggDebugUtils()
{
}

OggDebugUtils::~OggDebugUtils()
{
}

FString OggDebugUtils::BytesToHex(const uint8* Data, int32 Count)
{
	FString Out;
	for (int32 i = 0; i < Count; i++)
	{
		Out += FString::Printf(TEXT("%02X "), Data[i]);
	}
	return Out;
}

void OggDebugUtils::DumpOggPage(const TArray<uint8>& PageData)
{
	if (PageData.Num() < 27)
	{
		UE_LOG(LogTemp, Warning, TEXT("Page too small."));
		return;
	}

	// Check capture pattern
	if (FMemory::Memcmp(PageData.GetData(), "OggS", 4) != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Ogg capture pattern."));
		return;
	}
	
	//
	
	uint8 Version = PageData[4];
	uint8 HeaderType = PageData[5];

	uint64 GranulePosition;
	FMemory::Memcpy(&GranulePosition, &PageData[6], 8);

	uint32 Serial;
	FMemory::Memcpy(&Serial, &PageData[14], 4);

	uint32 Sequence;
	FMemory::Memcpy(&Sequence, &PageData[18], 4);

	uint32 CRC;
	FMemory::Memcpy(&CRC, &PageData[22], 4);
	
	//

	uint8 PageSegments = PageData[26];
	
	//
	
	UE_LOG(LogTemp, Log, TEXT("=========== OGG PAGE ==========="));
	UE_LOG(LogTemp, Log, TEXT("Version: %d"), Version);
	UE_LOG(LogTemp, Log, TEXT("HeaderType: 0x%02X"), HeaderType);
	UE_LOG(LogTemp, Log, TEXT("GranulePosition: %llu"), GranulePosition);
	UE_LOG(LogTemp, Log, TEXT("BitstreamSerial: %u"), Serial);
	UE_LOG(LogTemp, Log, TEXT("SequenceNumber: %u"), Sequence);
	UE_LOG(LogTemp, Log, TEXT("CRC: 0x%08X"), CRC);
	UE_LOG(LogTemp, Log, TEXT("Segments: %d"), PageSegments);

	int32 Offset = 27;
	int32 TotalPayload = 0;

	for (int32 i = 0; i < PageSegments; i++)
	{
		uint8 SegmentSize = PageData[Offset + i];
		TotalPayload += SegmentSize;

		UE_LOG(LogTemp, Log,
			TEXT("  Segment %d: %d bytes%s"),
			i,
			SegmentSize,
			SegmentSize == 255 ? TEXT(" (continues)") : TEXT(""));
	}

	UE_LOG(LogTemp, Log, TEXT("Total Payload: %d bytes"), TotalPayload);
	UE_LOG(LogTemp, Log, TEXT("================================"));
	
	//
	
	int32 SegmentTableOffset = 27;
	int32 DataOffset = SegmentTableOffset + PageSegments;

	UE_LOG(LogTemp, Log, TEXT("----- Extra Details Page -----"));
	UE_LOG(LogTemp, Log, TEXT("Segments: %d"), PageSegments);

	int32 PayloadIndex = DataOffset;

	for (int32 i = 0; i < PageSegments; ++i)
	{
		uint8 SegmentSize = PageData[SegmentTableOffset + i];

		if (PayloadIndex + SegmentSize > PageData.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("Segment exceeds page size."));
			return;
		}

		// Append segment to continued packet
		ContinuedPacket.Append(
			&PageData[PayloadIndex],
			SegmentSize);

		PayloadIndex += SegmentSize;

		// If segment < 255 → packet ends here
		if (SegmentSize < 255)
		{
			UE_LOG(LogTemp, Log, TEXT("Complete Opus Packet: %d bytes"), ContinuedPacket.Num());

			DumpOpusPacket(ContinuedPacket);

			ContinuedPacket.Empty();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("--------------------"));
}

/*
void OggDebugUtils::DumpOggPage(const TArray<uint8>& Data)
{
	if (Data.Num() < 27)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough data for Ogg page"));
		return;
	}

	if (FMemory::Memcmp(Data.GetData(), "OggS", 4) != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Ogg capture pattern"));
		return;
	}

	uint8 Version = Data[4];
	uint8 HeaderType = Data[5];

	uint64 GranulePosition;
	FMemory::Memcpy(&GranulePosition, &Data[6], 8);

	uint32 Serial;
	FMemory::Memcpy(&Serial, &Data[14], 4);

	uint32 Sequence;
	FMemory::Memcpy(&Sequence, &Data[18], 4);

	uint32 CRC;
	FMemory::Memcpy(&CRC, &Data[22], 4);

	uint8 PageSegments = Data[26];

	UE_LOG(LogTemp, Log, TEXT("=========== OGG PAGE ==========="));
	UE_LOG(LogTemp, Log, TEXT("Version: %d"), Version);
	UE_LOG(LogTemp, Log, TEXT("HeaderType: 0x%02X"), HeaderType);
	UE_LOG(LogTemp, Log, TEXT("GranulePosition: %llu"), GranulePosition);
	UE_LOG(LogTemp, Log, TEXT("BitstreamSerial: %u"), Serial);
	UE_LOG(LogTemp, Log, TEXT("SequenceNumber: %u"), Sequence);
	UE_LOG(LogTemp, Log, TEXT("CRC: 0x%08X"), CRC);
	UE_LOG(LogTemp, Log, TEXT("Segments: %d"), PageSegments);

	int32 Offset = 27;
	int32 TotalPayload = 0;

	for (int32 i = 0; i < PageSegments; i++)
	{
		uint8 SegmentSize = Data[Offset + i];
		TotalPayload += SegmentSize;

		UE_LOG(LogTemp, Log,
			TEXT("  Segment %d: %d bytes%s"),
			i,
			SegmentSize,
			SegmentSize == 255 ? TEXT(" (continues)") : TEXT(""));
	}

	UE_LOG(LogTemp, Log, TEXT("Total Payload: %d bytes"), TotalPayload);
	UE_LOG(LogTemp, Log, TEXT("================================"));
}
*/

void OggDebugUtils::DumpOpusPacket(const TArray<uint8>& Packet)
{
	if (Packet.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty packet."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("------ Opus Packet ------"));
	UE_LOG(LogTemp, Log, TEXT("Size: %d bytes"), Packet.Num());

	// Detect OpusHead
	if (Packet.Num() >= 8 &&
		FMemory::Memcmp(Packet.GetData(), "OpusHead", 8) == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("OpusHead detected."));
		return;
	}

	// Detect OpusTags
	if (Packet.Num() >= 8 &&
		FMemory::Memcmp(Packet.GetData(), "OpusTags", 8) == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("OpusTags detected."));
		return;
	}

	// Otherwise assume audio packet
	uint8 TOC = Packet[0];
	uint8 Config = TOC >> 3;
	bool bStereo = (TOC & 0x4) != 0;
	uint8 FrameCountCode = TOC & 0x3;

	int32 FrameCount = 1;
	if (FrameCountCode == 0) FrameCount = 1;
	else if (FrameCountCode == 1) FrameCount = 2;
	else if (FrameCountCode == 2) FrameCount = 2;
	else if (FrameCountCode == 3 && Packet.Num() > 1)
		FrameCount = Packet[1] & 0x3F;

	UE_LOG(LogTemp, Log, TEXT("Audio Packet"));
	UE_LOG(LogTemp, Log, TEXT("Stereo: %s"), bStereo ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Log, TEXT("FrameCount: %d"), FrameCount);

	//UE_LOG(LogTemp, Log, TEXT("--------------------------"));
	
	//
	
	UE_LOG(LogTemp, Log, TEXT("------ Extra Details Packet ------"));
	UE_LOG(LogTemp, Log, TEXT("Size: %d bytes"), Packet.Num());
	UE_LOG(LogTemp, Log, TEXT("TOC: 0x%02X"), TOC);
	UE_LOG(LogTemp, Log, TEXT("Config: %d"), Config);
	UE_LOG(LogTemp, Log, TEXT("Stereo: %s"), bStereo ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Log, TEXT("FrameCountCode: %d"), FrameCountCode);
	UE_LOG(LogTemp, Log, TEXT("FrameCount: %d"), FrameCount);

	int32 PreviewBytes = FMath::Min(16, Packet.Num());
	UE_LOG(LogTemp, Log,
		TEXT("First %d bytes: %s"),
		PreviewBytes,
		*BytesToHex(Packet.GetData(), PreviewBytes));

	UE_LOG(LogTemp, Log, TEXT("--------------------------"));
}

/*
void OggDebugUtils::DumpOpusPacket(const TArray<uint8>& Packet)
{
	if (Packet.Num() == 0)
		return;

	uint8 TOC = Packet[0];

	uint8 Config = TOC >> 3;
	uint8 Stereo = (TOC >> 2) & 0x01;
	uint8 FrameCountCode = TOC & 0x03;

	int32 FrameCount = 1;

	switch (FrameCountCode)
	{
	case 0: FrameCount = 1; break;
	case 1: FrameCount = 2; break;
	case 2: FrameCount = 2; break;
	case 3:
		if (Packet.Num() > 1)
			FrameCount = Packet[1] & 0x3F;
		break;
	}

	UE_LOG(LogTemp, Log, TEXT("------ Opus Packet ------"));
	UE_LOG(LogTemp, Log, TEXT("Size: %d bytes"), Packet.Num());
	UE_LOG(LogTemp, Log, TEXT("TOC: 0x%02X"), TOC);
	UE_LOG(LogTemp, Log, TEXT("Config: %d"), Config);
	UE_LOG(LogTemp, Log, TEXT("Stereo: %s"), Stereo ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Log, TEXT("FrameCountCode: %d"), FrameCountCode);
	UE_LOG(LogTemp, Log, TEXT("FrameCount: %d"), FrameCount);

	int32 PreviewBytes = FMath::Min(16, Packet.Num());
	UE_LOG(LogTemp, Log,
		TEXT("First %d bytes: %s"),
		PreviewBytes,
		*BytesToHex(Packet.GetData(), PreviewBytes));

	UE_LOG(LogTemp, Log, TEXT("--------------------------"));
}
*/