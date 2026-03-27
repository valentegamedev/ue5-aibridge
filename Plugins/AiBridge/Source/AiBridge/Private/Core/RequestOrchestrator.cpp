// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/RequestOrchestrator.h"

#include "Auth/ApiKeyProvider.h"
#include "Subsystems/AiBridgeSubsystem.h"

// Sets default values
ARequestOrchestrator::ARequestOrchestrator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARequestOrchestrator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARequestOrchestrator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARequestOrchestrator::Connect(TScriptInterface<IApiKeyProvider> ApiKeyProvider)
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] Connecting to the backend."), *StaticClass()->GetName());
	
	FString ApiKey = IApiKeyProvider::Execute_GetOrchestratorApiKey(ApiKeyProvider.GetObject());
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ApiKey is invalid."), *StaticClass()->GetName());
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] ApiKey retrieved successfully."), *StaticClass()->GetName());
	
	UAiBridgeSubsystem* AiBridgeSubsystem = GetGameInstance()->GetSubsystem<UAiBridgeSubsystem>();

	if (AiBridgeSubsystem)
	{
		AiBridgeSubsystem->Connect(ApiKey, ApiBaseUrl);
		AiBridgeSubsystem->OnOpusData.AddDynamic(this, &ARequestOrchestrator::HandleAudioData);
		AiBridgeSubsystem->OnAiResponse.AddDynamic(this, &ARequestOrchestrator::HandleAiResponse);
	}
}

void ARequestOrchestrator::Disconnect()
{
}

void ARequestOrchestrator::HandleAudioData(const TArray<uint8>& Data)
{
	OnAudioDataReceived.Broadcast(Data);
}

void ARequestOrchestrator::HandleAiResponse(const FString& RequestId, const FString& Text)
{
	OnAiResponseReceived.Broadcast(RequestId, Text);
}

