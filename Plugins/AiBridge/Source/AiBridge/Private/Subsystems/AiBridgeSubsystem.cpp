// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeSubsystem.h"
#include "Authentication/JwtAuthenticationService.h"
#include "Util/OggOpusStreamParser.h"
#include "WebSocket/WebSocketConnection.h"

void UAiBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (!IsValid(AuthService)) { 
		AuthService = NewObject<UJwtAuthenticationService>(this);
	}
	
	if (!IsValid(WebSocket))
	{
		WebSocket = NewObject<UWebSocketConnection>(this);
	}
	
	if (!IsValid(OpusParser)) { 
		OpusParser = NewObject<UOggOpusStreamParser>(this);
		
		TWeakObjectPtr<UAiBridgeSubsystem> WeakThis = this;
		
		OpusParser->OnStreamStart.BindLambda(
			[WeakThis](uint32 Serial, const TArray<uint8>& Page)
			{
				if (!WeakThis.IsValid()) return;
				
				auto& Buffer = WeakThis->OpusPages.FindOrAdd(Serial); // reference
			});
		
		OpusParser->OnPageReceived.BindLambda(
			[WeakThis](uint32 Serial, const TArray<uint8>& Page)
			{
				if (!WeakThis.IsValid()) return;
				
				auto& Buffer = WeakThis->OpusPages.FindOrAdd(Serial); // reference
				Buffer.Append(Page);
			});
		
		OpusParser->OnStreamEnd.BindLambda(
			[WeakThis](uint32 Serial)
			{
				if (!WeakThis.IsValid()) return;
				
				WeakThis->HandleOpusPages(Serial);
				WeakThis->OpusPages.FindAndRemoveChecked(Serial);
			});
	}
	
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Initialized."), *StaticClass()->GetName());
}

void UAiBridgeSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Deinitialized."), *StaticClass()->GetName());
	Super::Deinitialize();
}

void UAiBridgeSubsystem::SendWakeUpCallAsync() const
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No World available."), *StaticClass()->GetName());
		return;
	}
    
	FTimerHandle TimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		[this]()
		{
			FString HealthCheckUrl = ApiBaseUrl;
			HealthCheckUrl.RemoveFromEnd(TEXT("/"));
			HealthCheckUrl += TEXT("/health");
			
			UE_LOG(LogTemp, Warning, TEXT("[%s] Sending wake-up call to: %s"), *StaticClass()->GetName(), *HealthCheckUrl);

			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
				FHttpModule::Get().CreateRequest();

			Request->SetURL(HealthCheckUrl);
			Request->SetVerb(TEXT("GET"));
			Request->SetTimeout(10.0f);

			Request->OnProcessRequestComplete().BindLambda(
				[this](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					if (!Response.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Wake-up failed: No response."), *StaticClass()->GetName());
						return;
					}

					int32 Code = Response->GetResponseCode();

					if (bWasSuccessful || Code == 404)
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Cloud Run service warmed up successfully."), *StaticClass()->GetName());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Wake-up call failed."), *StaticClass()->GetName());
					}
				}
			);

			Request->ProcessRequest();
		},
		0.1f,
		false
	);
}

void UAiBridgeSubsystem::UnwrapAudioChunk(const TArray<uint8>& Data, FString& OutRequestId, TArray<uint8>& OutAudioData)
{
	// Validate input
	checkf(Data.Num() > 0, TEXT("Audio data cannot be null or empty"));
	
	// Check for wrapped format
	if (Data.Num() > 2 && Data[0] == AUDIO_DATA_MARKER)
	{
		uint8 RequestIdLength = Data[1];

		checkf(
			Data.Num() >= 2 + RequestIdLength,
			TEXT("Invalid wrapped audio format: data length %d < %d"),
			Data.Num(),
			2 + RequestIdLength
		);

		// Extract RequestId (UTF8)
		const uint8* RequestIdPtr = Data.GetData() + 2;
		FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(RequestIdPtr), RequestIdLength);
		OutRequestId = FString(Converter.Length(), Converter.Get());

		// Extract Audio Data
		int32 AudioOffset = 2 + RequestIdLength;
		int32 AudioSize = Data.Num() - AudioOffset;

		OutAudioData.SetNumUninitialized(AudioSize);
		FMemory::Memcpy(
			OutAudioData.GetData(),
			Data.GetData() + AudioOffset,
			AudioSize
		);

		return;
	}

	// STRICT MODE: not wrapped = error
	checkf(false, TEXT("Audio data is not wrapped with RequestId. All audio must be wrapped."));
}

void UAiBridgeSubsystem::HandleOnTextMessage(const FString& Msg)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Msg);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("JSON parsed successfully"));
		
		FString RequestId;
		if (JsonObject->TryGetStringField("requestId", RequestId))
		{
			FString Type;
			if (JsonObject->TryGetStringField("type", Type) && Type == "AiResponse")
			{
				FString Text;
				if (JsonObject->TryGetStringField("text", Text))
				{
					OnAiResponse.Broadcast(RequestId, Text);
				}
			}
		}
	}
}

void UAiBridgeSubsystem::HandleOnBinaryMessage(const TArray<uint8>& Data)
{
	UE_LOG(LogTemp, Log, TEXT("[On Binary] %d bytes"), Data.Num());
                
                
	FString RequestId;
	TArray<uint8> AudioData;

	UnwrapAudioChunk(Data, RequestId, AudioData);

	UE_LOG(LogTemp, Log, TEXT("RequestId: %s | Audio bytes: %d"), *RequestId, AudioData.Num());
                
	// Debug: Log first bytes to see what we're receiving
	FString PersonaName = TEXT("Daniel");
	if (AudioData.Num() >= 4)
	{
		FString ByteString = FString::Printf(TEXT("%02X-%02X-%02X-%02X"),
			AudioData[0], AudioData[1], AudioData[2], AudioData[3]);

		UE_LOG(LogTemp, Log, TEXT("[%s] First 4 bytes: %s (expecting OggS: 4F-67-67-53)"),
			*PersonaName,
			*ByteString);
	}
	
	OpusParser->PushBytes(AudioData);
	OnBinaryMessage.Broadcast(AudioData);
}

void UAiBridgeSubsystem::HandleOpusPages(uint32 Serial)
{
	auto& Buffer = OpusPages.FindOrAdd(Serial); // reference
	OnOpusData.Broadcast(Buffer);
}

void UAiBridgeSubsystem::Connect(FString pApiKeyProvider, FString pApiBaseUrl)
{
	ApiKeyProvider = pApiKeyProvider;
	ApiBaseUrl = pApiBaseUrl;
	
	SendWakeUpCallAsync();
	
	if (ApiKeyProvider.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ApiKey is invalid."), *StaticClass()->GetName());
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[%s] ApiKey is valid."), *StaticClass()->GetName());
	
	if (!AuthService)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] JwtAuthenticationService not assigned."), *StaticClass()->GetName());
		return;
	}
	AuthService->Initialize(ApiBaseUrl);
	UE_LOG(LogTemp, Warning, TEXT("[%s] JwtAuthenticationService Initialized on ApiBaseUrl = %s"), *StaticClass()->GetName(), *ApiBaseUrl);
	
	AuthService->GetAuthToken(
		"UnifiedConnection",
		"player",
		ApiKeyProvider,
		[this](const FString& JwtToken)
		{
			CachedToken = JwtToken;
			bJwtReady = !JwtToken.IsEmpty();

			UE_LOG(LogTemp, Warning, TEXT("[%s] JWT ready. Token: %s"), *StaticClass()->GetName(), *CachedToken);
			
			EnsureConnection([this](bool Success) { 
				UE_LOG(LogTemp, Warning, TEXT("[%s] Connected to server: %d"), *StaticClass()->GetName(), Success);
			});
		}
	);
	
}

void UAiBridgeSubsystem::EnsureConnection(TFunction<void(bool)> Callback)
{
	// 1. Already connected
    if (WebSocket!= nullptr && WebSocket->IsConnected())
    {
        Callback(true);
        return;
    }
	
    // 3. Start connection
    bIsConnecting = true;

    double StartTime = FPlatformTime::Seconds();
    
    AuthService->GetAuthToken(
        TEXT("UnifiedConnection"),
        TEXT("player"),
        ApiKeyProvider,
        [this, Callback, StartTime](const FString& JwtToken)
        {
            double JwtTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        	
        	UE_LOG(LogTemp, Warning, TEXT("[%s] JWT took %.0f ms"), *StaticClass()->GetName(), JwtTime);

            if (JwtToken.IsEmpty())
            {
            	UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to get JWT."), *StaticClass()->GetName());
                bIsConnecting = false;
                Callback(false);
                return;
            }
        	
            // Build URL
            FString WsScheme = ApiBaseUrl.StartsWith(TEXT("https")) ? TEXT("wss") : TEXT("ws");

            FString WsBase = ApiBaseUrl;
            WsBase.ReplaceInline(TEXT("http://"), TEXT(""));
            WsBase.ReplaceInline(TEXT("https://"), TEXT(""));

            FString BaseUrl = FString::Printf(TEXT("%s://%s%s"),
                *WsScheme,
                *WsBase.TrimEnd(),
                TEXT("/api/websocket")
            );

            FString FullUrl = FString::Printf(TEXT("%s?token=%s"),
                *BaseUrl,
                *FGenericPlatformHttp::UrlEncode(JwtToken)
            );
        	
        	double WsStart = FPlatformTime::Seconds();
        	
        	
        	TWeakObjectPtr<UAiBridgeSubsystem> WeakThis = this;
        	
        	WebSocket->OnDisconnected = [WeakThis]()
			{
        		if (!WeakThis.IsValid()) return;
        		
				UE_LOG(LogTemp, Log, TEXT("[disconnect]"));
			};
        	
        	WebSocket->OnBinaryMessage = [WeakThis](const TArray<uint8>& Data)
        	{
        		if (!WeakThis.IsValid()) return;
        		
        		UE_LOG(LogTemp, Log, TEXT("[On Binary] %d bytes"), Data.Num());
        		WeakThis->HandleOnBinaryMessage(Data);
        	};
        	
        	WebSocket->OnTextMessage = [WeakThis](const FString& Msg)
			{
        		if (!WeakThis.IsValid()) return;
        		
				UE_LOG(LogTemp, Log, TEXT("[On Text Message] %s "), *Msg);
				WeakThis->HandleOnTextMessage(Msg);
			};
        	
        	
            WebSocket->Connect(
                FullUrl,
                TEXT("UnifiedConnection"),
                JwtToken,
                [this, Callback, StartTime, WsStart, FullUrl](bool bConnected)
                {
                    double WsTime = (FPlatformTime::Seconds() - WsStart) * 1000.0;

                	UE_LOG(LogTemp, Warning, TEXT("[%s] WS took %.0f ms"), *StaticClass()->GetName(), WsTime);
                	
                    if (bConnected)
                    {
                        double Total = (FPlatformTime::Seconds() - StartTime) * 1000.0;

                    	UE_LOG(LogTemp, Warning, TEXT("[%s] Connected (%.0f ms total)"), *StaticClass()->GetName(), Total);

                        bIsConnecting = false;
                    	OnConnected.Broadcast();
                        Callback(true);
                    }
                    else
                    {
                    	UE_LOG(LogTemp, Error, TEXT("[%s] WebSocket failed."), *StaticClass()->GetName());

                        bIsConnecting = false;
                        Callback(false);
                    }
                }
            );
        }
    );
}

void UAiBridgeSubsystem::Disconnect()
{

}

void UAiBridgeSubsystem::ProcessFakeBinaryData(TArray<uint8> Data)
{
	WebSocket->OnBinaryMessage(Data);
}

void UAiBridgeSubsystem::SendSomething()
{
	TArray<TArray<uint8>> Chunks = {

		// Chunk 1
		{
			72, 3, 153, 18, 31, 14, 108, 99, 205, 154, 177, 88,
			20, 125, 37, 188, 23, 247, 28, 64
		},

		// Chunk 2
		{
			72, 13, 53, 188, 88, 59, 153, 77, 132, 251, 179, 64,
			148, 192, 116, 29, 150, 15, 229, 170, 182, 98, 235, 250,
			195, 248, 139, 243, 29, 214, 234, 22, 207, 148, 112
		},

		// Chunk 3
		{
			72, 128, 11, 175, 126, 19, 184, 64, 14, 119, 199, 235,
			200, 63, 149, 7, 0, 114, 164, 139, 209, 18, 128, 79,
			221, 35, 14, 251, 211, 246, 92, 132, 154, 38, 120, 200,
			60, 158, 2, 217, 204, 94, 157, 196
		}
	};
	
	for (const TArray<uint8>& Chunk : Chunks)
	{
		WebSocket->SendBinary(Chunk);
	}
}

void UAiBridgeSubsystem::SendStartAudioRequest()
{
	FString RequestId = FGuid::NewGuid().ToString();
	FString JsonString = FString::Printf(TEXT(R"(
	{
	  "languageCode" : "en-US",
	  "voiceId" : "EXAVITQu4vr4xnSDxMaL",
	  "messages" : [ {
	    "role" : "system",
	    "content" : "You are a professional customer service agent for XRLab.\n\nCOMPANY INFORMATION:\nSaxion XRLab is a Mixed Reality lab which focus on innovation using VR and AR solutions.\n\nPRODUCT KNOWLEDGE:\nWe offer development services for any kind of media which needs VR or AR. Including development using Unity and Unreal.\n\nCUSTOMER CONTEXT:\n\"No customer context available.\"\n\nSERVICE GUIDELINES:\n1. Greet customers warmly and professionally\n2. Listen actively to understand the issue\n3. Provide accurate information from the knowledge base\n4. If you don't know something, say so and offer to escalate\n5. Always confirm the customer's issue is resolved before ending\n6. Keep responses concise but complete\n\nESCALATION TRIGGERS:\n- Technical issues beyond basic troubleshooting\n- Billing disputes over ${serviceConfig.escalationThreshold}\n- Complaints about employee conduct\n- Legal or compliance questions\n\nWhen escalating, explain why and what will happen next.",
	    "timestamp" : 0.0
	  } ],
	  "type" : "SessionStart",
	  "requestId" : "29d8dda2-878d-4ea6-b40c-f3b413110542",
	  "timestamp" : null,
	  "sttProvider" : "google",
	  "audioFormat" : "opus",
	  "sampleRate" : 16000,
	  "opusBitrate" : 64000,
	  "ttsStreamingMode" : "batch",
	  "llmProvider" : "openai",
	  "llmModel" : "gpt-4o-mini",
	  "ttsModel" : "eleven_turbo_v2_5",
	  "maxTokens" : 500,
	  "temperature" : 0.7,
	  "ttsOutputFormat" : "opus",
	  "enableMetrics" : false,
	  "customVocabulary" : null,
	  "customVocabularyBoost" : 10.0,
	  "voiceStability" : 0.5,
	  "voiceSimilarityBoost" : 0.75,
	  "voiceStyle" : 0.0,
	  "voiceUseSpeakerBoost" : true,
	  "voiceSpeed" : 1.0,
	  "ttsLanguageCode" : null,
	  "contextCacheName" : null
	}
	)"));
	WebSocket->SendText(JsonString);
	
}

void UAiBridgeSubsystem::SendEndOfAudioRequest()
{
	FString RequestId = FGuid::NewGuid().ToString();
	FString JsonString = FString::Printf(TEXT(R"(
        {
		  "type" : "EndOfSpeech",
		  "requestId" : "29d8dda2-878d-4ea6-b40c-f3b413110542",
		  "timestamp" : null
		}
    )"));
	WebSocket->SendText(JsonString);
	
	JsonString = FString::Printf(TEXT(R"(
        {
		  "type" : "EndOfAudio",
		  "requestId" : "29d8dda2-878d-4ea6-b40c-f3b413110542",
		  "timestamp" : null
		}
    )"));
	WebSocket->SendText(JsonString);
	
	
}

void UAiBridgeSubsystem::SendTextRequest(const FString Text)
{
	
	FString RequestId = FGuid::NewGuid().ToString();
	FString JsonString = FString::Printf(TEXT(R"(
        {
          "type": "textinput",
          "text": "%s",
          "requestId": "%s",
          "timestamp":1771596968241,
          "isNpcInitiated": false,
          "context": {
            "systemPrompt": "You are a professional customer service agent for XRLab.\n\nCOMPANY INFORMATION:\nSaxion XRLab is a Mixed Reality lab which focus on innovation using VR and AR solutions.\n\nPRODUCT KNOWLEDGE:\nWe offer development services for any kind of media which needs VR or AR. Including development using Unity and Unreal.\n\nCUSTOMER CONTEXT:\n\"No customer context available.\"\n\nSERVICE GUIDELINES:\n1. Greet customers warmly and professionally\n2. Listen actively to understand the issue\n3. Provide accurate information from the knowledge base\n4. If you don't know something, say so and offer to escalate\n5. Always confirm the customer's issue is resolved before ending\n6. Keep responses concise but complete\n\nESCALATION TRIGGERS:\n- Technical issues beyond basic troubleshooting\n- Billing disputes over ${serviceConfig.escalationThreshold}\n- Complaints about employee conduct\n- Legal or compliance questions\n\nWhen escalating, explain why and what will happen next.",
            "messages": [{
                "role": "user",
                "content": "Hi there! my name is Daniel"
              },
              {
                "role": "assistant",
                "content": "Hello traveler! What brings you here?"
              }],
            "voiceId": "EXAVITQu4vr4xnSDxMaL",
            "llmModel": "gpt-4o-mini",
            "llmProvider": "openai",
            "temperature": 0.7,
            "maxTokens": 500,
            "language": "en-US",
            "ttsStreamingMode": "batch",
            "ttsModel": "eleven_turbo_v2_5",
            "sttProvider": "google",

            "voiceStability": 0.5,
            "voiceSimilarityBoost": 0.75,
            "voiceStyle": 0.6,
            "voiceUseSpeakerBoost": true,
            "voiceSpeed": 1.0,
            "ttsLanguageCode": "en",

            "responseFormat": "json_object",
            "location": "europe-west4",

            "contextCacheName": "projects/my-project/locations/europe-west4/cachedContents/abc123"
          }
        }
    )"), *Text, *RequestId);
	WebSocket->SendText(JsonString);
}

void UAiBridgeSubsystem::SendBinaryRequest(const TArray<uint8> Bytes)
{
	WebSocket->SendBinary(Bytes);
}
