// Georgy Treshchev 2025.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeVADProviderBase.h"
#include "RuntimeAudioImporterDefines.h"
#include "RuntimeAudioImporterTypes.h"

#include "RuntimeDefaultVADProvider.generated.h"

namespace FVAD_RuntimeAudioImporter
{
    struct Fvad;
}

/**
 * Default VAD provider using libfvad
 */
UCLASS(BlueprintType, Category = "Voice Activity Detector")
class RUNTIMEAUDIOIMPORTER_API URuntimeDefaultVADProvider : public URuntimeVADProviderBase
{
    GENERATED_BODY()

public:
    URuntimeDefaultVADProvider();

    //~ Begin UObject Interface
    virtual void BeginDestroy() override;
    //~ End UObject Interface

    //~ Begin URuntimeVADProviderBase Interface
    virtual bool Reset() override;
    virtual int32 ProcessAudio(const TArray<float>& PCMData, int32 SampleRate) override;
    virtual int32 GetRequiredSampleRate() const override { return 8000; }
    virtual float GetFrameDurationMs() const override { return FrameDurationMs; }
    virtual bool IsSpeechActive() const override { return bIsSpeechActive; }
    virtual void OnSpeechStarted() override;
    virtual void OnSpeechEnded() override;
    //~ End URuntimeVADProviderBase Interface

    /**
     * Set VAD mode (aggressiveness level)
     * @param Mode VAD mode to set
     * @return True if mode was set successfully
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    bool SetVADMode(ERuntimeVADMode Mode);

protected:
    /**
     * Initialize the VAD provider
     * @return True if initialization was successful
     */
    bool Initialize();
    
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    /** The libfvad instance */
    FVAD_RuntimeAudioImporter::Fvad* VADInstance;
#endif

    /** Applied sample rate */
    int32 AppliedSampleRate;

    /** Accumulated PCM data for processing */
    TArray<int16> AccumulatedPCMData;

    /** Frame duration in milliseconds */
    float FrameDurationMs;

    /** Whether speech is currently active */
    bool bIsSpeechActive;
};