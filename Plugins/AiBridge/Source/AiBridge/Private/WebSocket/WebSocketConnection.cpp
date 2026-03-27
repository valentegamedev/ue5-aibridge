// Fill out your copyright notice in the Description page of Project Settings.


#include "WebSocket/WebSocketConnection.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

UWebSocketConnection::~UWebSocketConnection()
{
	if (WebSocket.IsValid()) {
		WebSocket->OnConnected().RemoveAll(this);
		WebSocket->OnConnectionError().RemoveAll(this);
		WebSocket->OnClosed().RemoveAll(this);
		WebSocket->OnMessage().RemoveAll(this);
		WebSocket->OnBinaryMessage().RemoveAll(this);
	}
}

bool UWebSocketConnection::IsConnected() const
{
	return WebSocket.IsValid() && WebSocket->IsConnected();
}

void UWebSocketConnection::Connect(const FString& Url, const FString& ConnectionId, const FString& InToken, TFunction<void(bool)> Callback)
{
	if (IsConnected() || bIsConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already connected or connecting"));
		Callback(false);
		return;
	}

	bIsConnecting = true;
	JwtToken = InToken;
	LastUrl = Url;

	FString SafeUrl = SanitizeUrl(Url);

	UE_LOG(LogTemp, Log, TEXT("🔌 Connecting to %s"), *SafeUrl);

	WebSocket = FWebSocketsModule::Get().CreateWebSocket(Url);
	
	TWeakObjectPtr<UWebSocketConnection> WeakThis(this);
	
	WebSocket->OnConnected().AddLambda([WeakThis, Callback]()
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->bIsDisconnecting = false;
		WeakThis->HandleConnected();
		Callback(true);
	});

	WebSocket->OnConnectionError().AddLambda(
		[WeakThis, Callback](const FString& Error)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleError(Error);
			Callback(false);
		}
	);
	
	WebSocket->OnClosed().AddLambda([WeakThis](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->HandleClosed(StatusCode, Reason, bWasClean);
	});
	
	WebSocket->OnMessage().AddLambda(
		[WeakThis](const FString& Msg)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnMessage(Msg);
		}
	);

	WebSocket->OnBinaryMessage().AddLambda(
		[WeakThis](const void* Data, SIZE_T Size, bool isLast)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnBinary(Data, Size, isLast);
		}
	);

	WebSocket->Connect();
	
}

void UWebSocketConnection::Disconnect()
{
	UE_LOG(LogTemp, Log, TEXT("UWebSocketConnection::Disconnect"));
	
	bAutoReconnect = false;
	bIsDisconnecting = true;

	GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
	if (WebSocket.IsValid())
	{
		WebSocket->Close();
	}
}

void UWebSocketConnection::HandleConnected()
{
	bIsConnecting = false;
	ReconnectAttempts = 0;
	CurrentReconnectDelay = ReconnectBaseDelay;

	if (bVerbose)
	{
		UE_LOG(LogTemp, Log, TEXT("✅ Connected"));
	}

	if (OnConnected) OnConnected();
}

void UWebSocketConnection::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogTemp, Warning, TEXT("OnClosed"));
	
	if (bVerbose)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔌 Disconnected: %d"), StatusCode);
	}
	
	if (WebSocket.IsValid())
	{
		WebSocket->OnConnected().RemoveAll(this);
		WebSocket->OnConnectionError().RemoveAll(this);
		WebSocket->OnClosed().RemoveAll(this);
		WebSocket->OnMessage().RemoveAll(this);
		WebSocket->OnBinaryMessage().RemoveAll(this);
	}
	
	WebSocket = nullptr;
	
	if (OnDisconnected) OnDisconnected();
	
	
	
	if (bAutoReconnect)
	{
		AttemptReconnect();
	}
}

void UWebSocketConnection::HandleError(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("WebSocket error: %s"), *Error);
	bIsConnecting = false;
	if (OnError) OnError(Error);
}

void UWebSocketConnection::HandleOnMessage(const FString& Msg)
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] Message: %s"), *StaticClass()->GetName(), *Msg);
	
	if (OnTextMessage) OnTextMessage(Msg);
}

void UWebSocketConnection::HandleOnBinary(const void* Data, SIZE_T Size, bool isLast)
{
	TArray<uint8> Bytes;
	Bytes.Append((uint8*)Data, Size);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Binary Data: %d bytes"), *StaticClass()->GetName(), Bytes.Num());
	
	if (OnBinaryMessage) OnBinaryMessage(Bytes);
}

void UWebSocketConnection::BindDelegates()
{
	TWeakObjectPtr<UWebSocketConnection> WeakThis(this);
	
	WebSocket->OnConnected().AddLambda([WeakThis]()
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->bIsDisconnecting = false;
		WeakThis->HandleConnected();
	});

	WebSocket->OnConnectionError().AddLambda(
		[WeakThis](const FString& Error)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleError(Error);
		}
	);
	
	WebSocket->OnClosed().AddLambda([WeakThis](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->HandleClosed(StatusCode, Reason, bWasClean);
	});
	
	WebSocket->OnMessage().AddLambda(
		[WeakThis](const FString& Msg)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnMessage(Msg);
		}
	);

	WebSocket->OnBinaryMessage().AddLambda(
		[WeakThis](const void* Data, SIZE_T Size, bool isLast)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnBinary(Data, Size, isLast);
		}
	);
}

void UWebSocketConnection::AttemptReconnect()
{
	if (bIsDisconnecting) return;

	if (ReconnectAttempts >= MaxReconnectAttempts)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Max reconnect attempts reached"));
		bIsReconnecting = false;
		return;
	}

	bIsReconnecting = true;
	ReconnectAttempts++;

	float Delay = CurrentReconnectDelay;

	UE_LOG(LogTemp, Warning, TEXT("Reconnect attempt %d in %.1fs"), ReconnectAttempts, Delay);

	TWeakObjectPtr<UWebSocketConnection> WeakThis(this);
	
	UWorld* World = nullptr;
	
	if (const UObject* Outer = GetOuter())
	{
		World = Outer->GetWorld();
	}
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("World is null"));
		return;
	} else
	{
		UE_LOG(LogTemp, Log, TEXT("World is not null"));
	}
	World->GetTimerManager().SetTimer(
		ReconnectTimerHandle,
		[WeakThis]()
		{
			UE_LOG(LogTemp, Log, TEXT("✅ Trying to Reconnected"));
			
			if (!WeakThis.IsValid()) return;

			WeakThis->bIsConnecting = false;

			WeakThis->Connect(
				WeakThis->LastUrl,
				TEXT("Reconnect"),
				WeakThis->JwtToken,
				[WeakThis](bool bSuccess)
				{
					if (!WeakThis.IsValid()) return;

					if (bSuccess)
					{
						UE_LOG(LogTemp, Log, TEXT("✅ Reconnected"));

						WeakThis->bIsReconnecting = false;
						WeakThis->ReconnectAttempts = 0;
						WeakThis->CurrentReconnectDelay = WeakThis->ReconnectBaseDelay;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("❌ Reconnect failed"));

						// exponential backoff
						WeakThis->CurrentReconnectDelay = FMath::Min(
							WeakThis->CurrentReconnectDelay * 2.0f,
							WeakThis->ReconnectMaxDelay
						);

						WeakThis->bIsReconnecting = false;

						// try again
						WeakThis->AttemptReconnect();
					}
				}
			);

		},
		Delay,
		false
	);
}


void UWebSocketConnection::SendText(const FString& Message)
{
	if (!IsConnected()) return;

	WebSocket->Send(Message);
}

void UWebSocketConnection::SendBinary(const TArray<uint8>& Data)
{
	if (!IsConnected()) return;

	WebSocket->Send(Data.GetData(), Data.Num(), true);
}

FString UWebSocketConnection::SanitizeUrl(const FString& Url)
{
	FString Result = Url;

	int32 TokenIndex = Result.Find(TEXT("token="), ESearchCase::IgnoreCase);
	if (TokenIndex == INDEX_NONE)
	{
		return Result;
	}

	int32 ValueStart = TokenIndex + 6; // length of "token="

	// Find next '&' AFTER token value
	int32 AmpIndex = Result.Find(TEXT("&"), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);

	if (AmpIndex != INDEX_NONE)
	{
		// token is in the middle
		return Result.Left(ValueStart) + TEXT("[REDACTED]") + Result.Mid(AmpIndex);
	}
	else
	{
		// token is last parameter
		return Result.Left(ValueStart) + TEXT("[REDACTED]");
	}
}