// Georgy Treshchev 2025.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IRuntimeVADProvider.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class URuntimeVADProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for Voice Activity Detection providers
 */
class RUNTIMEAUDIOIMPORTER_API IRuntimeVADProvider
{
    GENERATED_BODY()

public:
    /**
     * Initialize the VAD provider
     * @return True if initialization was successful
     */
    virtual bool Initialize() = 0;

    /**
     * Reset the VAD provider state
     * @return True if reset was successful
     */
    virtual bool Reset() = 0;

    /**
     * Process audio data for voice activity detection
     * @param PCMData Audio data in float format
     * @param SampleRate Sample rate of the audio data
     * @return VAD result (1 for speech, 0 for silence, -1 for error/insufficient data)
     */
    virtual int32 ProcessAudio(const TArray<float>& PCMData, int32 SampleRate) = 0;

    /**
     * Get the required sample rate for this VAD provider
     * @return Required sample rate in Hz
     */
    virtual int32 GetRequiredSampleRate() const = 0;

    /**
     * Get the provider name
     * @return Provider name
     */
    virtual FString GetProviderName() const = 0;

    /**
     * Check if the provider is available/supported
     * @return True if the provider is available
     */
    virtual bool IsAvailable() const = 0;

    /**
     * Get the frame duration in milliseconds for this provider
     * @return Frame duration in milliseconds
     */
    virtual float GetFrameDurationMs() const = 0;

    /**
     * Set provider-specific configuration
     * @param ConfigName Configuration parameter name
     * @param ConfigValue Configuration parameter value
     * @return True if configuration was set successfully
     */
    virtual bool SetConfiguration(const FString& ConfigName, const FString& ConfigValue) = 0;

    /**
     * Check if speech is currently active (for providers that track speech state internally)
     * @return True if speech is active
     */
    virtual bool IsSpeechActive() const = 0;

    /**
     * Called when speech starts (for providers that need to track speech state)
     */
    virtual void OnSpeechStarted() {}

    /**
     * Called when speech ends (for providers that need to track speech state)
     */
    virtual void OnSpeechEnded() {}
};