// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TextInputContext.h"
#include "TextInputRequest.generated.h"

USTRUCT(BlueprintType)
struct FTextInputRequest
{
	GENERATED_BODY()

	UPROPERTY() FString Type;
	UPROPERTY() FString Text;
	UPROPERTY() FString RequestId;

	UPROPERTY() int64 Timestamp = 0;
	UPROPERTY() bool bIsNpcInitiated = false;

	UPROPERTY() FTextInputContext Context;
};