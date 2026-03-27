// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatMessage.h"
#include "TextInputContext.generated.h"

USTRUCT(BlueprintType)
struct FTextInputContext
{
	GENERATED_BODY()

	UPROPERTY() FString SystemPrompt;
	UPROPERTY() TArray<FChatMessage> Messages;

	UPROPERTY() FString VoiceId;
	UPROPERTY() FString LlmModel;
	UPROPERTY() FString LlmProvider;

	UPROPERTY() float Temperature = 0.0f;
	UPROPERTY() int32 MaxTokens = 0;

	UPROPERTY() FString Language;
	UPROPERTY() FString TtsStreamingMode;
	UPROPERTY() FString TtsModel;
	UPROPERTY() FString SttProvider;

	UPROPERTY() float VoiceStability = 0.0f;
	UPROPERTY() float VoiceSimilarityBoost = 0.0f;
	UPROPERTY() float VoiceStyle = 0.0f;
	UPROPERTY() bool VoiceUseSpeakerBoost = false;
	UPROPERTY() float VoiceSpeed = 1.0f;
	UPROPERTY() FString TtsLanguageCode;

	UPROPERTY() FString ResponseFormat;
	UPROPERTY() FString Location;
	UPROPERTY() FString ContextCacheName;
};
