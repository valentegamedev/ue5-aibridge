// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RequestOrchestrator.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDataReceived, const TArray<uint8>&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAiResponseReceived, const FString&, RequestId, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTranscriptionReceived, const FString&, RequestId, const FString&, Text);

UCLASS()
class AIBRIDGE_API ARequestOrchestrator : public AActor
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default, meta = (AllowPrivateAccess = "true"))
	FString ApiBaseUrl;
public:	
	// Sets default values for this actor's properties
	ARequestOrchestrator();
	
	UPROPERTY(BlueprintAssignable, Category = "Orchestrator")
	FOnAudioDataReceived OnAudioDataReceived;
	
	UPROPERTY(BlueprintAssignable, Category = "Orchestrator")
	FOnAiResponseReceived OnAiResponseReceived;
	
	UPROPERTY(BlueprintAssignable, Category = "Orchestrator")
	FOnTranscriptionReceived OnTranscriptionReceived;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(TScriptInterface<IApiKeyProvider> ApiKeyProvider);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();
	
	UFUNCTION()
	void HandleAudioData(const TArray<uint8>& Data);
	
	UFUNCTION()
	void HandleAiResponse(const FString& RequestId, const FString& Text);
	
	UFUNCTION()
	void HandleOnTranscriptionReceived(const FString& RequestId, const FString& Text);
};
