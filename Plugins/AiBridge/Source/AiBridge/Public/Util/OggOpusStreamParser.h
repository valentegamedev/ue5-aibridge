// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OggOpusStreamParser.generated.h"
DECLARE_DELEGATE_TwoParams(FOnOggStreamStart, uint32 /*Serial*/, const TArray<uint8>& /*Page*/);
DECLARE_DELEGATE_TwoParams(FOnOggPageReceived, uint32 /*Serial*/, const TArray<uint8>& /*Page*/);
DECLARE_DELEGATE_OneParam(FOnOggStreamEnd, uint32 /*Serial*/);

UCLASS()
class AIBRIDGE_API UOggOpusStreamParser: public UObject
{
	GENERATED_BODY()
public:

	void PushBytes(TArray<uint8> Data);

	FOnOggStreamStart OnStreamStart;
	FOnOggPageReceived OnPageReceived;
	FOnOggStreamEnd OnStreamEnd;

private:

	bool TryExtractPage(TArray<uint8>& OutPage);

	int32 FindCapturePattern();

	uint32 ParseSerial(const TArray<uint8>& Page);

	uint8 ParseHeaderType(const TArray<uint8>& Page);

private:

	TArray<uint8> Buffer;

	TSet<uint32> ActiveStreams;
};
