// Georgy Treshchev 2025.

#include "VAD/RuntimeVADProviderBase.h"
#include "RuntimeAudioImporter.h"
#include "RuntimeAudioImporterDefines.h"

URuntimeVADProviderBase::URuntimeVADProviderBase()
{
}

bool URuntimeVADProviderBase::Reset()
{
	UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Reset() called on base VAD provider class"));
	return false;
}

int32 URuntimeVADProviderBase::ProcessAudio(const TArray<float>& PCMData, int32 SampleRate)
{
	UE_LOG(LogRuntimeAudioImporter, Error, TEXT("ProcessAudio() called on base VAD provider class"));
	return -1;
}

int32 URuntimeVADProviderBase::GetRequiredSampleRate() const
{
	return 16000;
}

float URuntimeVADProviderBase::GetFrameDurationMs() const
{
	return 0.0f;
}

bool URuntimeVADProviderBase::IsSpeechActive() const
{
	return false;
}

void URuntimeVADProviderBase::OnSpeechStarted()
{
}

void URuntimeVADProviderBase::OnSpeechEnded()
{
}