// Georgy Treshchev 2025.

#include "VAD/RuntimeVoiceActivityDetector.h"
#include "VAD/RuntimeDefaultVADProvider.h"
#include "RuntimeAudioImporter.h"
#include "Codecs/RAW_RuntimeCodec.h"
#include "Async/Async.h"

URuntimeVoiceActivityDetector::URuntimeVoiceActivityDetector()
    : bIsSpeechActive(false)
    , ConsecutiveVoiceFrames(0)
    , ConsecutiveSilenceFrames(0)
{
}

void URuntimeVoiceActivityDetector::BeginDestroy()
{
    CurrentProvider = nullptr;
    Super::BeginDestroy();
}

URuntimeVoiceActivityDetector* URuntimeVoiceActivityDetector::CreateRuntimeVoiceActivityDetector()
{
    URuntimeVoiceActivityDetector* VADDetector = NewObject<URuntimeVoiceActivityDetector>();
    VADDetector->UseDefaultVADProvider();
    return VADDetector;
}

bool URuntimeVoiceActivityDetector::SetVADProvider(TSubclassOf<URuntimeVADProviderBase> Provider)
{
    if (!Provider)
    {
        return false;
    }
    CurrentProvider.Reset(NewObject<URuntimeVADProviderBase>(this, Provider));
    return true;
}

URuntimeVADProviderBase* URuntimeVoiceActivityDetector::GetVADProvider() const
{
    return CurrentProvider.Get();
}

bool URuntimeVoiceActivityDetector::SetVADMode(ERuntimeVADMode Mode)
{
    if (URuntimeDefaultVADProvider* DefaultVADProvider = Cast<URuntimeDefaultVADProvider>(GetVADProvider()))
    {
        return DefaultVADProvider->SetVADMode(Mode);
    }
    else
    {
        UE_LOG(LogRuntimeAudioImporter, Warning, TEXT("Cannot set the VAD mode as the selected provider is not the default provider"));
        return false;
    }
}

bool URuntimeVoiceActivityDetector::UseDefaultVADProvider()
{
    return SetVADProvider(URuntimeDefaultVADProvider::StaticClass());
}

bool URuntimeVoiceActivityDetector::ResetVAD()
{
    if (!CurrentProvider)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("No VAD provider set"));
        return false;
    }

    bool bSuccess = CurrentProvider->Reset();
    if (bSuccess)
    {
        bIsSpeechActive = false;
        ConsecutiveVoiceFrames = 0;
        ConsecutiveSilenceFrames = 0;
        UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Successfully reset VAD"));
    }

    return bSuccess;
}

bool URuntimeVoiceActivityDetector::ProcessVAD(TArray<float> PCMData, int32 InSampleRate, int32 NumOfChannels)
{
    if (!CurrentProvider)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("No VAD provider set"));
        return false;
    }

    // Preprocess audio data
    int32 ProcessedSampleRate;
    if (!PreprocessAudioData(PCMData, InSampleRate, NumOfChannels, ProcessedSampleRate))
    {
        return false;
    }

    // Process with the current provider
    int32 VADResult = CurrentProvider->ProcessAudio(PCMData, ProcessedSampleRate);
    
    // Handle the result
    HandleVADResult(VADResult, CurrentProvider->GetFrameDurationMs());

    return VADResult == 1;
}

void URuntimeVoiceActivityDetector::HandleVADResult(int32 VADResult, float FrameDurationMs)
{
    auto ExecuteOnSpeechStarted = [this]()
    {
        URuntimeVoiceActivityDetector* VAD = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis = MakeWeakObjectPtr(VAD)]()
        {
            if (WeakThis.IsValid())
            {
                if (WeakThis->CurrentProvider)
                {
                    WeakThis->CurrentProvider->OnSpeechStarted();
                }
                WeakThis->OnSpeechStartedNative.Broadcast();
                WeakThis->OnSpeechStarted.Broadcast();
            }
        });
    };

    auto ExecuteOnSpeechEnded = [this]()
    {
        URuntimeVoiceActivityDetector* VAD = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis = MakeWeakObjectPtr(VAD)]()
        {
            if (WeakThis.IsValid())
            {
                if (WeakThis->CurrentProvider)
                {
                    WeakThis->CurrentProvider->OnSpeechEnded();
                }
                WeakThis->OnSpeechEndedNative.Broadcast();
                WeakThis->OnSpeechEnded.Broadcast();
            }
        });
    };

    if (VADResult == 1)
    {
        ConsecutiveVoiceFrames += static_cast<int32>(FrameDurationMs);
        ConsecutiveSilenceFrames = 0;

        if (!bIsSpeechActive && ConsecutiveVoiceFrames >= MinimumSpeechDuration)
        {
            bIsSpeechActive = true;
            ExecuteOnSpeechStarted();
        }
    }
    else if (VADResult == 0)
    {
        ConsecutiveVoiceFrames = 0;
        ConsecutiveSilenceFrames += static_cast<int32>(FrameDurationMs);

        if (bIsSpeechActive && ConsecutiveSilenceFrames >= SilenceDuration)
        {
            bIsSpeechActive = false;
            ExecuteOnSpeechEnded();
        }
    }
    // VADResult == -1 means insufficient data or error, don't change state
}

bool URuntimeVoiceActivityDetector::PreprocessAudioData(TArray<float>& PCMData, int32 InSampleRate, int32 NumOfChannels, int32& OutSampleRate)
{
    if (PCMData.Num() == 0)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("PCM data is empty"));
        return false;
    }

    if (InSampleRate <= 0 || NumOfChannels <= 0)
    {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Invalid sample rate or channel count"));
        return false;
    }

    Audio::FAlignedFloatBuffer AlignedPCMData{Audio::FAlignedFloatBuffer(PCMData)};

    // Resample if necessary
    int32 RequiredSampleRate = CurrentProvider->GetRequiredSampleRate();
    if (InSampleRate != RequiredSampleRate)
    {
        Audio::FAlignedFloatBuffer AlignedPCMData_Resampled;
        if (!FRAW_RuntimeCodec::ResampleRAWData(AlignedPCMData, 1, InSampleRate, RequiredSampleRate, AlignedPCMData_Resampled))
        {
            UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to resample audio data"));
            return false;
        }
        AlignedPCMData = MoveTemp(AlignedPCMData_Resampled);
        OutSampleRate = RequiredSampleRate;
    }
    else
    {
        OutSampleRate = InSampleRate;
    }

    // Mix channels if necessary (VAD only supports mono audio data)
    if (NumOfChannels > 1)
    {
        Audio::FAlignedFloatBuffer AlignedPCMData_Mixed;
        if (!FRAW_RuntimeCodec::MixChannelsRAWData(AlignedPCMData, InSampleRate, NumOfChannels, 1, AlignedPCMData_Mixed))
        {
            UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to mix audio data"));
            return false;
        }
        AlignedPCMData = MoveTemp(AlignedPCMData_Mixed);
    }

    PCMData = MoveTemp(AlignedPCMData);

    return true;
}