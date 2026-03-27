// Georgy Treshchev 2025.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RuntimeAudioImporterDefines.h"
#include "RuntimeAudioImporterTypes.h"
#include "RuntimeVADProviderBase.h"
#include "UObject/StrongObjectPtr.h"
#include "Templates/SubclassOf.h"
#include "RuntimeVoiceActivityDetector.generated.h"

/**
 * Runtime Voice Activity Detector
 * Detects voice activity in audio data using pluggable providers
 */
UCLASS(BlueprintType, Category = "Voice Activity Detector")
class RUNTIMEAUDIOIMPORTER_API URuntimeVoiceActivityDetector : public UObject
{
    GENERATED_BODY()

public:
    URuntimeVoiceActivityDetector();

    //~ Begin UObject Interface
    virtual void BeginDestroy() override;
    //~ End UObject Interface

    /**
     * Create a new instance of the VAD
     *
     * @return Created VAD
     */
    UFUNCTION(BlueprintCallable, Category = "Voice Activity Detector")
    static URuntimeVoiceActivityDetector* CreateRuntimeVoiceActivityDetector();

    /**
     * Set the VAD provider to use for voice activity detection
     * @param Provider The VAD provider instance
     * @return True if the provider was successfully set
     */
    UFUNCTION(BlueprintCallable, meta = (Keywords = "Voice Activity Detector Provider"), Category = "Voice Activity Detector")
    bool SetVADProvider(TSubclassOf<URuntimeVADProviderBase> Provider);

    /**
     * Get the current VAD provider
     * @return The current VAD provider
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get VAD Provider", Keywords = "Voice Activity Detector Provider"), Category = "Voice Activity Detector")
    URuntimeVADProviderBase* GetVADProvider() const;

    /**
     * Changes the operating ("aggressiveness") mode of a VAD (Voice Activity Detector) instance
     * A more aggressive (higher mode) VAD is more restrictive in reporting speech
     * In other words, the probability of detecting voice activity increases with a higher mode
     * However, this also increases the rate of missed detections
     * VeryAggressive is used by default
     *
     * @note This function can be called only if VAD is enabled (see ToggleVAD)
     * @param Mode The VAD mode to set
     * @return Whether the VAD mode was successfully set
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set VAD Mode", Keywords = "Voice Activity Detector Mode"), Category = "Voice Activity Detector")
    bool SetVADMode(ERuntimeVADMode Mode);

    /**
     * Create and set the default VAD provider (libfvad)
     * @return True if the default provider was successfully created and set
     */
    UFUNCTION(BlueprintCallable, meta = (Keywords = "Voice Activity Detector Default Provider"), Category = "Voice Activity Detector")
    bool UseDefaultVADProvider();

    /**
     * Reset the current VAD provider
     * @return True if the VAD provider was successfully reset
     */
    UFUNCTION(BlueprintCallable, meta = (Keywords = "Voice Activity Detector Reset"), Category = "Voice Activity Detector")
    bool ResetVAD();

    /**
     * Process audio data for voice activity detection
     * @param PCMData PCM audio data in 32-bit floating point interleaved format
     * @param InSampleRate The sample rate of the provided PCM data
     * @param NumOfChannels The number of channels in the provided PCM data
     * @return True if voice activity was detected
     */
    UFUNCTION(BlueprintCallable, meta = (Keywords = "Voice Activity Detector Process"), Category = "Voice Activity Detector")
    bool ProcessVAD(TArray<float> PCMData, UPARAM(DisplayName = "Sample Rate") int32 InSampleRate, int32 NumOfChannels);

protected:
    /** The current VAD provider */
    TStrongObjectPtr<URuntimeVADProviderBase> CurrentProvider;

    /** Whether speech is currently active */
    bool bIsSpeechActive;

    /** Consecutive frames where voice activity was detected */
    int32 ConsecutiveVoiceFrames;

    /** Consecutive frames where no voice activity was detected */
    int32 ConsecutiveSilenceFrames;

    /**
     * Handle VAD processing results and manage speech state
     * @param VADResult Result from VAD processing
     * @param FrameDurationMs Duration of the processed frame
     */
    void HandleVADResult(int32 VADResult, float FrameDurationMs);

    /**
     * Prepare audio data for VAD processing (resampling, channel mixing)
     * @param PCMData PCM data
     * @param InSampleRate Input sample rate
     * @param NumOfChannels Number of input channels
     * @param OutSampleRate Output sample rate
     * @return True if preprocessing was successful
     */
    bool PreprocessAudioData(TArray<float>& PCMData, int32 InSampleRate, int32 NumOfChannels, int32& OutSampleRate);

public:
    // Delegates
    DECLARE_MULTICAST_DELEGATE(FOnSpeechStartedNative);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpeechStarted);
    DECLARE_MULTICAST_DELEGATE(FOnSpeechEndedNative);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpeechEnded);

    FOnSpeechStartedNative OnSpeechStartedNative;
    
    UPROPERTY(BlueprintAssignable, Category = "Voice Activity Detector|Delegates")
    FOnSpeechStarted OnSpeechStarted;

    FOnSpeechEndedNative OnSpeechEndedNative;
    
    UPROPERTY(BlueprintAssignable, Category = "Voice Activity Detector|Delegates")
    FOnSpeechEnded OnSpeechEnded;

    // Configuration properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Activity Detector|Configuration")
    int32 MinimumSpeechDuration = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Activity Detector|Configuration")
    int32 SilenceDuration = 500;
};