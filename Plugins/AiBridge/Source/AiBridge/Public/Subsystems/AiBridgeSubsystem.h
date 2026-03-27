// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AiBridgeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAiResponse, const FString&, RequestId, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBinaryMessage, const TArray<uint8>&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpusData, const TArray<uint8>&, Data);


class UJwtAuthenticationService;
class UWebSocketConnection;
class UOggOpusStreamParser;
/**
 * 
 */
UCLASS()
class AIBRIDGE_API UAiBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	TMap<uint32, TArray<uint8>> OpusPages;
	
	FString ApiKeyProvider;
	FString ApiBaseUrl;
	
	UPROPERTY()
	TObjectPtr<UJwtAuthenticationService> AuthService;
	
	UPROPERTY()
	TObjectPtr<UWebSocketConnection> WebSocket;
	
	UPROPERTY()
	TObjectPtr<UOggOpusStreamParser> OpusParser;
	
	bool bJwtReady;
	FString CachedToken;
	bool bIsConnecting = false;
	
	static constexpr uint8 AUDIO_DATA_MARKER = 0xAD;
public:
	// Events
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnDisconnected OnDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnTextMessage OnTextMessage;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnBinaryMessage OnBinaryMessage;
	
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnOpusData OnOpusData;
	
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnAiResponse OnAiResponse;
	
private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void SendWakeUpCallAsync() const;
	void UnwrapAudioChunk(const TArray<uint8>& Data, FString& OutRequestId, TArray<uint8>& OutAudioData);
	
	
	void HandleOnTextMessage(const FString& Text);
	void HandleOnBinaryMessage(const TArray<uint8>& Data);
	
	void HandleOpusPages(uint32 Serial);
	
	bool bIsProcessingAudioRequest;
	FString ProcessingAudioRequestId;
	
public:
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(FString ApiKeyProvider, FString ApiBaseUrl);
	
	void EnsureConnection(TFunction<void(bool)> Callback);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void ProcessFakeBinaryData(TArray<uint8> Data);
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendStartAudioRequest();
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendEndOfAudioRequest();
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendTextRequest(const FString Text);
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendBinaryRequest(const TArray<uint8> Bytes);
};
