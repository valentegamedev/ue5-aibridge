// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class AIBRIDGE_API OggDebugUtils
{
private:
	// Stores partial packet across page boundaries
	static TArray<uint8> ContinuedPacket;
public:
	OggDebugUtils();
	~OggDebugUtils();
	static void DumpOggPage(const TArray<uint8>& PageData);
	static void DumpOpusPacket(const TArray<uint8>& Packet);
	static FString BytesToHex(const uint8* Data, int32 Count);
};
