// Georgy Treshchev 2025.

#include "VAD/RuntimeDefaultVADProvider.h"
#include "RuntimeAudioImporter.h"
#include "Codecs/RAW_RuntimeCodec.h"

#if !UE_VERSION_OLDER_THAN(5, 1, 0)
#include "DSP/FloatArrayMath.h"
#endif

#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
#include "VADIncludes.h"
#endif

URuntimeDefaultVADProvider::URuntimeDefaultVADProvider()
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    : VADInstance(nullptr)
#endif
    , AppliedSampleRate(0)
    , FrameDurationMs(0)
    , bIsSpeechActive(false)
{
    Initialize();
}

void URuntimeDefaultVADProvider::BeginDestroy()
{
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    if (VADInstance)
    {
        FVAD_RuntimeAudioImporter::fvad_free(VADInstance);
        VADInstance = nullptr;
    }
#endif
    Super::BeginDestroy();
}

bool URuntimeDefaultVADProvider::Initialize()
{
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    if (VADInstance)
    {
        FVAD_RuntimeAudioImporter::fvad_free(VADInstance);
    }

    VADInstance = FVAD_RuntimeAudioImporter::fvad_new();
    if (VADInstance)
    {
        SetVADMode(ERuntimeVADMode::VeryAggressive);
        AppliedSampleRate = 0;
        AccumulatedPCMData.Reset();
        bIsSpeechActive = false;
        
        UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Successfully initialized Default VAD provider"));
        return true;
    }
    else
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Failed to create libfvad instance"));
        return false;
    }
#else
    UE_LOG(LogRuntimeAudioImporter, Error, TEXT("libfvad support is disabled"));
    return false;
#endif
}

bool URuntimeDefaultVADProvider::Reset()
{
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    if (!VADInstance)
    {
        return false;
    }

    FVAD_RuntimeAudioImporter::fvad_reset(VADInstance);
    SetVADMode(ERuntimeVADMode::VeryAggressive);
    AppliedSampleRate = 0;
    AccumulatedPCMData.Reset();
    bIsSpeechActive = false;
    
    UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Successfully reset Default VAD provider"));
    return true;
#else
    return false;
#endif
}

int32 URuntimeDefaultVADProvider::ProcessAudio(const TArray<float>& PCMData, int32 SampleRate)
{
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    if (!VADInstance)
    {
        return -1;
    }

    // Apply the sample rate to the VAD instance if it is different from the current sample rate
    if (AppliedSampleRate != SampleRate)
    {
        if (FVAD_RuntimeAudioImporter::fvad_set_sample_rate(VADInstance, SampleRate) != 0)
        {
            UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to set VAD sample rate"));
            return -1;
        }
        AppliedSampleRate = SampleRate;
    }

    // Convert float PCM data to int16 PCM data
    TArray<int16> Int16PCMData;
    {
#if UE_VERSION_OLDER_THAN(5, 1, 0)
        int16* Int16PCMDataPtr;
        FRAW_RuntimeCodec::TranscodeRAWData<float, int16>(PCMData.GetData(), PCMData.Num(), Int16PCMDataPtr);
        Int16PCMData.Append(Int16PCMDataPtr, PCMData.Num());
        FMemory::Free(Int16PCMDataPtr);
#else
        Int16PCMData.AddUninitialized(PCMData.Num());
        Audio::ArrayFloatToPcm16(MakeArrayView(PCMData), MakeArrayView(Int16PCMData));
#endif
    }

    // Append the new PCM data to the accumulated data
    AccumulatedPCMData.Append(Int16PCMData);

    // Calculate the length of the accumulated audio data in milliseconds
    float AudioDataLengthMs = static_cast<float>(AccumulatedPCMData.Num()) / static_cast<float>(AppliedSampleRate) * 1000;

    // Process the accumulated audio data if it reaches 10, 20, or 30 ms
    if (AudioDataLengthMs >= 10)
    {
        int32 ValidLength = [AudioDataLengthMs]()
        {
            if (AudioDataLengthMs >= 30)
            {
                return 30;
            }
            else if (AudioDataLengthMs >= 20)
            {
                return 20;
            }
            else if (AudioDataLengthMs >= 10)
            {
                return 10;
            }
            return 0;
        }();

        if (ValidLength == 0)
        {
            return -1;
        }

        // Calculate the number of samples to process
        int32 NumToProcess = ValidLength * AppliedSampleRate / 1000;

        // Process the VAD
        int32 VADResult = FVAD_RuntimeAudioImporter::fvad_process(VADInstance, AccumulatedPCMData.GetData(), NumToProcess);

        // Remove processed data from the accumulated buffer
        AccumulatedPCMData.RemoveAt(0, NumToProcess);
        FrameDurationMs = ValidLength;

        return VADResult;
    }

    return -1; // Not enough data yet
#else
    return -1;
#endif
}

bool URuntimeDefaultVADProvider::SetVADMode(ERuntimeVADMode Mode)
{
#if WITH_RUNTIMEAUDIOIMPORTER_VAD_SUPPORT
    if (!VADInstance)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to set VAD mode for %s as the VAD instance is not valid"), *GetName());
        return false;
    }
    if (FVAD_RuntimeAudioImporter::fvad_set_mode(VADInstance, VoiceActivityDetector::GetVADModeInt(Mode)) != 0)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to set VAD mode for %s as the mode is invalid"), *GetName());
        return false;
    }
    UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Successfully set VAD mode for %s to %s"), *GetName(), *UEnum::GetValueAsName(Mode).ToString());
    return true;
#else
    UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to set VAD mode for %s as VAD support is disabled"), *GetName());
    return false;
#endif
}

void URuntimeDefaultVADProvider::OnSpeechStarted()
{
    bIsSpeechActive = true;
}

void URuntimeDefaultVADProvider::OnSpeechEnded()
{
    bIsSpeechActive = false;
}