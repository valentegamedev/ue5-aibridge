// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NpcClientComponent.h"

// Sets default values for this component's properties
UNpcClientComponent::UNpcClientComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SttProvider = "google";
	MaxTokens = 500;
	LanguageCode = "en-US";
	VoiceId = "EXAVITQu4vr4xnSDxMaL";
	LlmModel = "gpt-4o-mini";
	LlmProvider = "openai";
	TtsModel = "eleven_turbo_v2_5";
	TtsStreamingMode = "batch";
	TtsLanguageCode = "en";
	SystemPrompt = "You are a professional customer service agent for XRLab.\n\nCOMPANY INFORMATION:\nSaxion XRLab is a Mixed Reality lab which focus on innovation using VR and AR solutions.\n\nPRODUCT KNOWLEDGE:\nWe offer development services for any kind of media which needs VR or AR. Including development using Unity and Unreal.\n\nCUSTOMER CONTEXT:\n\"No customer context available.\"\n\nSERVICE GUIDELINES:\n1. Greet customers warmly and professionally\n2. Listen actively to understand the issue\n3. Provide accurate information from the knowledge base\n4. If you don't know something, say so and offer to escalate\n5. Always confirm the customer's issue is resolved before ending\n6. Keep responses concise but complete\n\nESCALATION TRIGGERS:\n- Technical issues beyond basic troubleshooting\n- Billing disputes over ${serviceConfig.escalationThreshold}\n- Complaints about employee conduct\n- Legal or compliance questions\n\nWhen escalating, explain why and what will happen next.";
}


// Called when the game starts
void UNpcClientComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UNpcClientComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

