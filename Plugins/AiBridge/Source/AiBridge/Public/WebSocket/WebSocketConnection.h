// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IWebSocket.h"
#include "WebSocketConnection.generated.h"

/**
 * 
 */
UCLASS()
class AIBRIDGE_API UWebSocketConnection : public UObject
{
	GENERATED_BODY()
public:
	
	
	~UWebSocketConnection();
	
	// State
	bool IsConnected() const;
	bool IsConnecting() const { return bIsConnecting; }

	// Core API
	void Connect(const FString& Url, const FString& ConnectionId, const FString& InToken, TFunction<void(bool)> Callback);
	void Disconnect();
	void SendText(const FString& Message);
	void SendBinary(const TArray<uint8>& Data);

	// Events
	TFunction<void()> OnConnected;
	TFunction<void()> OnDisconnected;
	TFunction<void(const FString&)> OnError;
	TFunction<void(const FString&)> OnTextMessage;
	TFunction<void(const TArray<uint8>&)> OnBinaryMessage;
	
private:
	TSharedPtr<IWebSocket> WebSocket;

	// State
	bool bIsConnecting = false;
	bool bIsDisconnecting = false;

	// Reconnect
	float ReconnectBaseDelay = .5f;
	float ReconnectMaxDelay = 10.0f;
	float CurrentReconnectDelay = .5f;
	int32 MaxReconnectAttempts = 10;
	int32 ReconnectAttempts = 0;
	bool bIsReconnecting = false;
	bool bAutoReconnect = true;
	FTimerHandle ReconnectTimerHandle;

	// Session
	FString LastUrl;
	FString JwtToken;

	bool bVerbose = true;

	// Internal
	void HandleConnected();
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleError(const FString& Error);
	void HandleOnMessage(const FString& Msg);
	void HandleOnBinary(const void* Data, SIZE_T Size, bool isLast);
	
	
	void BindDelegates();
	void AttemptReconnect();

	FString SanitizeUrl(const FString& Url);
};
