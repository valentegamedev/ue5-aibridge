// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LipSyncMetaHumanComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AIBRIDGE_API ULipSyncMetaHumanComponent : public UActorComponent
{

	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSyncMetaHumanComponent")
	USkeletalMeshComponent* TargetMesh;
	
	ULipSyncMetaHumanComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Set from your OVRLipSync system
	UFUNCTION(BlueprintCallable, Category = "LipSyncMetaHumanComponent")
	void SetVisemes(const TArray<float>& InVisemes);

protected:
	

	UPROPERTY(EditAnywhere)
	float SmoothingSpeed = 12.0f;

private:
	TArray<float> Visemes;

	// Current smoothed curves
	TMap<FName, float> CurrentCurves;

	// Temporary per-frame curves
	TMap<FName, float> TargetCurves;

	void BuildCurveMap();
	void ApplyCurves(float DeltaTime);
		
};
