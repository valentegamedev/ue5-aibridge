// Georgy Treshchev 2025.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RuntimeVADProviderBase.generated.h"

/**
 * Base class for all Voice Activity Detection providers
 * This class can't be instantiated directly and should be subclassed
 */
UCLASS(Abstract, HideDropdown, BlueprintType, Category = "Voice Activity Detector")
class RUNTIMEAUDIOIMPORTER_API URuntimeVADProviderBase : public UObject
{
    GENERATED_BODY()

public:
    URuntimeVADProviderBase();

    /**
     * Reset the VAD provider state
     * @return True if reset was successful
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    virtual bool Reset();

    /**
     * Process audio data for voice activity detection
     * @param PCMData Audio data in float format
     * @param SampleRate Sample rate of the audio data
     * @return VAD result (1 for speech, 0 for silence, -1 for error/insufficient data)
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    virtual int32 ProcessAudio(const TArray<float>& PCMData, int32 SampleRate);

    /**
     * Get the required sample rate for this VAD provider
     * @return Required sample rate in Hz
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    virtual int32 GetRequiredSampleRate() const;

    /**
     * Get the frame duration in milliseconds for this provider
     * @return Frame duration in milliseconds
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    virtual float GetFrameDurationMs() const;

    /**
     * Check if speech is currently active (for providers that track speech state internally)
     * @return True if speech is active
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    virtual bool IsSpeechActive() const;

    /**
     * Called when speech starts (for providers that need to track speech state)
     */
    virtual void OnSpeechStarted();

    /**
     * Called when speech ends (for providers that need to track speech state)
     */
    virtual void OnSpeechEnded();
};