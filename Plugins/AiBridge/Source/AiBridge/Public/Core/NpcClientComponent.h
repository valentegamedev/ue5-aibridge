// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NpcClientComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AIBRIDGE_API UNpcClientComponent : public UActorComponent
{


private:
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString SttProvider; //"sttProvider": "google"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	int MaxTokens; //"maxTokens": 500
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString LanguageCode; //"languageCode" : "en-US",
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString VoiceId; //"voiceId" : "EXAVITQu4vr4xnSDxMaL"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString LlmModel; //"llmModel": "gpt-4o-mini"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString LlmProvider; //"llmProvider" : "openai"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString TtsModel; //"ttsModel": "eleven_turbo_v2_5"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString TtsStreamingMode; //"ttsStreamingMode": "batch"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString TtsLanguageCode; //"ttsLanguageCode" : "en"
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=NPC, meta = (AllowPrivateAccess = "true"))
	FString SystemPrompt;
	
public:	
	// Sets default values for this component's properties
	UNpcClientComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	FString GetSttProvider() const
	{
		return SttProvider;
	}

	int GetMaxTokens() const
	{
		return MaxTokens;
	}

	FString GetLanguageCode() const
	{
		return LanguageCode;
	}

	FString GetVoiceId() const
	{
		return VoiceId;
	}

	FString GetLlmModel() const
	{
		return LlmModel;
	}

	FString GetLlmProvider() const
	{
		return LlmProvider;
	}

	FString GetTtsModel() const
	{
		return TtsModel;
	}

	FString GetTtsStreamingMode() const
	{
		return TtsStreamingMode;
	}

	FString GetTtsLanguageCode() const
	{
		return TtsLanguageCode;
	}

	FString GetSystemPrompt() const
	{
		return SystemPrompt;
	}
};
