// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "Authentication/JwtAuthenticationService.h"
#include "AiBridgeWebSocketSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWebSocketConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWebSocketDisconnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketTextMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketBinaryMessage, const TArray<uint8>&, Data);

class UWebSocketConnection;
/**
 * 
 */
UCLASS()
class AIBRIDGE_API UAiBridgeWebSocketSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintReadWrite, Category = "WebSocket")
	bool sendWakeUpCall = true;
	
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

	// Connection
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect();

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();

	// Sending
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendText(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendBinary(const TArray<uint8>& Data);

	UFUNCTION(BlueprintPure, Category = "WebSocket")
	bool IsConnected() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketDisconnected OnDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketTextMessage OnTextMessage;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketBinaryMessage OnBinaryMessage;
	
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketBinaryMessage OnBinaryRawMessage;

	void EnsureConnection(TFunction<void(bool)> Callback);
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendSomething();
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendSomethingCrazy();
	
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void ProcessFakeBinaryData(TArray<uint8> Data);
	
private:
	
	FString ApiBaseUrl = "https://api-orchestrator-service-936031000571.europe-west4.run.app";
	
	bool bIsConnecting = false;
	UWebSocketConnection* WebSocket;
	
	UPROPERTY()
	UJwtAuthenticationService* AuthService;
	bool bJwtReady;
	FString CachedToken;
	
	void InitializeConnectionSequence();
	
	void SendWakeUpCallAsync() const;
	
	void PreFetchJwtToken();
	
	static constexpr uint8 AUDIO_DATA_MARKER = 0xAD;
	
	void UnwrapAudioChunk(
		const TArray<uint8>& Data,
		FString& OutRequestId,
		TArray<uint8>& OutAudioData);
	
	bool IsWrapped(const TArray<uint8>& Data);
	
	bool IsOggHeader(const TArray<uint8>& Data);
	
	int32 ReceivedStreamCount = 0;
	
	
	void OPUSDecode(const TArray<uint8>& Data);
	
	bool ParseOpusHead(const TArray<uint8>& Packet);
	
	int32 Channels;
	uint16 PreSkip;
	int32 SampleRate;
	int16 OutputGain;
	
	
};
