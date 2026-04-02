// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/LipSyncMetaHumanComponent.h"

ULipSyncMetaHumanComponent::ULipSyncMetaHumanComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULipSyncMetaHumanComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetMesh)
	{
		TargetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}

	Visemes.SetNum(15); // OVR viseme count
}

void ULipSyncMetaHumanComponent::SetVisemes(const TArray<float>& InVisemes)
{
	if (InVisemes.Num() >= 15)
	{
		Visemes = InVisemes;
	}
}

void ULipSyncMetaHumanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TargetMesh) return;

	BuildCurveMap();
	ApplyCurves(DeltaTime);
}

void ULipSyncMetaHumanComponent::BuildCurveMap()
{
	TargetCurves.Empty();

	auto Add = [&](const FName& Name, float Value)
	{
		TargetCurves.FindOrAdd(Name) += Value;
	};
	

	// PP
	Add("mouthClose", Visemes[1]);

	// FF
	Add("mouthUpperUpLeft", Visemes[2] * 0.5f);
	Add("mouthUpperUpRight", Visemes[2] * 0.5f);
	Add("mouthLowerDownLeft", Visemes[2] * 0.5f);
	Add("mouthLowerDownRight", Visemes[2] * 0.5f);

	// TH
	Add("jawOpen", Visemes[3] * 0.3f);

	// DD
	Add("jawOpen", Visemes[4] * 0.3f);

	// KK
	Add("jawOpen", Visemes[5] * 0.4f);

	// CH
	Add("mouthPucker", Visemes[6] * 0.6f);

	// SS
	Add("mouthStretchLeft", Visemes[7] * 0.7f);
	Add("mouthStretchRight", Visemes[7] * 0.7f);

	// NN
	Add("jawOpen", Visemes[8] * 0.2f);

	// RR
	Add("mouthPucker", Visemes[9] * 0.4f);

	// AA
	Add("jawOpen", Visemes[10] * 1.0f);

	// E
	Add("mouthSmileLeft", Visemes[11] * 0.6f);
	Add("mouthSmileRight", Visemes[11] * 0.6f);
	Add("jawOpen", Visemes[11] * 0.4f);

	// I
	Add("mouthStretchLeft", Visemes[12] * 0.8f);
	Add("mouthStretchRight", Visemes[12] * 0.8f);

	// O
	Add("mouthFunnel", Visemes[13] * 0.7f);

	// U
	Add("mouthPucker", Visemes[14] * 0.9f);
}

void ULipSyncMetaHumanComponent::ApplyCurves(float DeltaTime)
{
	for (auto& Pair : TargetCurves)
	{
		const FName& Name = Pair.Key;
		float Target = FMath::Clamp(Pair.Value, 0.0f, 1.0f);

		float& Current = CurrentCurves.FindOrAdd(Name);

		Current = FMath::FInterpTo(Current, Target, DeltaTime, SmoothingSpeed);

		TargetMesh->SetMorphTarget(Name, Current);
	}

	// Optional: decay unused curves
	for (auto& Pair : CurrentCurves)
	{
		if (!TargetCurves.Contains(Pair.Key))
		{
			Pair.Value = FMath::FInterpTo(Pair.Value, 0.0f, DeltaTime, SmoothingSpeed);
			TargetMesh->SetMorphTarget(Pair.Key, Pair.Value);
		}
	}
	
}