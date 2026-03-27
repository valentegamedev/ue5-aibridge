// Georgy Treshchev 2025.

#if WITH_RUNTIMEAUDIOIMPORTER_CAPTURE_SUPPORT
#ifndef PLATFORM_TVOS
#define PLATFORM_TVOS 0
#endif
#if PLATFORM_IOS && !PLATFORM_TVOS
#include "Sound/IOS/AudioCaptureIOS.h"
#include "Async/Future.h"
#include "RuntimeAudioImporterDefines.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "IOS/IOSAppDelegate.h"
#endif

constexpr int32 kInputBus = 1;
constexpr int32 kOutputBus = 0;

static OSStatus RecordingCallback(void* inRefCon,
	AudioUnitRenderActionFlags* ioActionFlags,
	const AudioTimeStamp* inTimeStamp,
	UInt32 inBusNumber,
	UInt32 inNumberFrames,
	AudioBufferList* ioData)
{
	Audio::FAudioCaptureIOS* AudioCapture = (Audio::FAudioCaptureIOS*)inRefCon;
	return AudioCapture->OnCaptureCallback(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

bool Audio::FAudioCaptureIOS::GetCaptureDeviceInfo(Audio::FCaptureDeviceInfo& OutInfo, int32 DeviceIndex)
{
	OutInfo.DeviceName = TEXT("Default iOS Audio Device");
	OutInfo.InputChannels = NumChannels;
	OutInfo.PreferredSampleRate = SampleRate;

	return true;
}


bool Audio::FAudioCaptureIOS::
#if UE_VERSION_NEWER_THAN(5, 2, 9)
OpenAudioCaptureStream
#else
OpenCaptureStream
#endif
(const Audio::FAudioCaptureDeviceParams& InParams,
#if UE_VERSION_NEWER_THAN(5, 2, 9)
	FOnAudioCaptureFunction InOnCapture
#else
	FOnCaptureFunction InOnCapture
#endif
	, uint32 NumFramesDesired)
{
	if (bIsStreamOpen)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream because it is already open"));
		return true;
	}

#if ENGINE_MAJOR_VERSION >= 5
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	[AppDelegate SetFeature:EAudioFeature::Playback Active:true];
	[AppDelegate SetFeature:EAudioFeature::Record Active:true];
	[AppDelegate SetFeature:EAudioFeature::BackgroundAudio Active:true];

	UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Enabled iOS audio session features for recording"));
#endif

	auto CheckPermissionGranted = [this]() -> bool {
		const bool bPermissionGranted =
#if (defined(__IPHONE_17_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_17_0)
			[[AVAudioApplication sharedInstance]recordPermission] == AVAudioApplicationRecordPermissionGranted;
#else
			[[AVAudioSession sharedInstance]recordPermission] == AVAudioSessionRecordPermissionGranted;
#endif
		return bPermissionGranted;
		};

	// Always check permission status, even if it was denied before
	// This handles the case where user grants permission via Settings after initial denial
	if (!CheckPermissionGranted())
	{
		UE_LOG(LogRuntimeAudioImporter, Warning, TEXT("Permission to record audio on iOS is not granted. Requesting permission..."));

		TSharedPtr<TPromise<bool>> PermissionPromise = MakeShared<TPromise<bool>>();

#if (defined(__IPHONE_17_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_17_0)
		[AVAudioApplication requestRecordPermissionWithCompletionHandler : ^ (BOOL granted) {
			dispatch_async(dispatch_get_main_queue(), ^ {
				PermissionPromise->SetValue(granted);
				});
		}];
#else
		[[AVAudioSession sharedInstance]requestRecordPermission:^ (BOOL granted) {
			dispatch_async(dispatch_get_main_queue(), ^ {
				PermissionPromise->SetValue(granted);
				});
		}];
#endif

		TFuture<bool> PermissionFuture = PermissionPromise->GetFuture();

		// This will automatically block until the future is set
		bool bPermissionGranted = PermissionFuture.Get();

		if (!bPermissionGranted)
		{
			// Check one more time in case permission was granted via Settings
			if (!CheckPermissionGranted())
			{
				UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream because permission to record audio was not granted"));
				return false;
			}
			else
			{
				UE_LOG(LogRuntimeAudioImporter, Warning, TEXT("Permission to record audio was granted via Settings. Opening capture stream..."));
			}
		}
		else
		{
			UE_LOG(LogRuntimeAudioImporter, Warning, TEXT("Permission to record audio on iOS was granted. Opening capture stream..."));
		}
	}

	OnCapture = MoveTemp(InOnCapture);

	OSStatus Status = noErr;

	NSError* setCategoryError = nil;
	[[AVAudioSession sharedInstance]setCategory:AVAudioSessionCategoryPlayAndRecord withOptions : (AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetooth) error : &setCategoryError];
	if (setCategoryError != nil)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when setting audio session category"), static_cast<int32>(Status));
		return false;
	}

	[[AVAudioSession sharedInstance]setActive:YES error : &setCategoryError];
	if (setCategoryError != nil)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when setting audio session active"), static_cast<int32>(Status));
		return false;
	}

	// Create a dispatch semaphore to wait for initialization
	dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

	// Result variable to capture success/failure from async block
	__block bool bInitSucceeded = false;

	// Source of info "Technical Note TN2091 - Device input using the HAL Output Audio Unit"

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

		OSStatus Status_Internal = noErr;

	AudioComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	// We use processing element always for enable runtime changing HW AEC and AGC settings.
	// When it's disable the unit work as RemoteIO
	desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	AudioComponent InputComponent = AudioComponentFindNext(NULL, &desc);

	Status_Internal = AudioComponentInstanceNew(InputComponent, &IOUnit);
	if (Status_Internal != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when creating audio component instance"), static_cast<int32>(Status_Internal));
		bInitSucceeded = false;
		dispatch_semaphore_signal(semaphore); // Signal even on error
		return;
	}

	// Enable recording
	uint32 EnableIO = 1;
	Status_Internal = AudioUnitSetProperty(IOUnit,
		kAudioOutputUnitProperty_EnableIO,
		kAudioUnitScope_Input,
		kInputBus,
		&EnableIO,
		sizeof(EnableIO));

	if (Status_Internal != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when enabling input"), static_cast<int32>(Status_Internal));
		bInitSucceeded = false;
		dispatch_semaphore_signal(semaphore); // Signal even on error
		return;
	}

	// Disable output part
	EnableIO = 0;
	Status_Internal = AudioUnitSetProperty(IOUnit,
		kAudioOutputUnitProperty_EnableIO,
		kAudioUnitScope_Output,
		kOutputBus,
		&EnableIO,
		sizeof(EnableIO));

	if (Status_Internal != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when disabling output"), static_cast<int32>(Status_Internal));
		bInitSucceeded = false;
		dispatch_semaphore_signal(semaphore); // Signal even on error
		return;
	}

	AudioStreamBasicDescription StreamDescription = { 0 };
	const UInt32 BytesPerSample = sizeof(Float32);

	StreamDescription.mSampleRate = SampleRate;
	StreamDescription.mFormatID = kAudioFormatLinearPCM;
	StreamDescription.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	StreamDescription.mChannelsPerFrame = NumChannels;
	StreamDescription.mBitsPerChannel = 8 * BytesPerSample;
	StreamDescription.mBytesPerFrame = BytesPerSample * StreamDescription.mChannelsPerFrame;
	StreamDescription.mFramesPerPacket = 1;
	StreamDescription.mBytesPerPacket = StreamDescription.mFramesPerPacket * StreamDescription.mBytesPerFrame;

	// Configure output format
	Status_Internal = AudioUnitSetProperty(IOUnit,
		kAudioUnitProperty_StreamFormat,
		kAudioUnitScope_Output,
		kInputBus,
		&StreamDescription,
		sizeof(StreamDescription));

	if (Status_Internal != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when setting output format"), static_cast<int32>(Status_Internal));
		bInitSucceeded = false;
		dispatch_semaphore_signal(semaphore); // Signal even on error
		return;
	}

	// Setup capture callback
	AURenderCallbackStruct CallbackInfo;
	CallbackInfo.inputProc = RecordingCallback;
	CallbackInfo.inputProcRefCon = this;
	Status_Internal = AudioUnitSetProperty(IOUnit,
		kAudioOutputUnitProperty_SetInputCallback,
		kAudioUnitScope_Global,
		kInputBus,
		&CallbackInfo,
		sizeof(CallbackInfo));

	if (Status_Internal != noErr) {
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when setting callback"), static_cast<int32>(Status_Internal));
		bInitSucceeded = false;
		dispatch_semaphore_signal(semaphore); // Signal even on error
		return;
	}

	// Initialize audio unit with retry for error -66635
	// Calls to AudioUnitInitialize() can fail if called back-to-back on different ADM instances
	// with error -66635. Multiple attempts with delays are needed to avoid this issue.
	// Based on WebRTC implementation which uses up to 5 attempts.
	constexpr int32 kMaxNumberOfAudioUnitInitializeAttempts = 5;
	int32 failed_initialize_attempts = 0;
	
	Status_Internal = AudioUnitInitialize(IOUnit);
	while (Status_Internal != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Warning, TEXT("AudioUnitInitialize failed with error %d (attempt %d/%d)"), 
			static_cast<int32>(Status_Internal), failed_initialize_attempts + 1, kMaxNumberOfAudioUnitInitializeAttempts);
		
		++failed_initialize_attempts;
		if (failed_initialize_attempts >= kMaxNumberOfAudioUnitInitializeAttempts)
		{
			UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to open capture stream due to error %d when initializing audio unit after %d attempts"), 
				static_cast<int32>(Status_Internal), kMaxNumberOfAudioUnitInitializeAttempts);
			bInitSucceeded = false;
			dispatch_semaphore_signal(semaphore); // Signal even on error
			return;
		}
		
		// Sleep for 100ms before retrying (following WebRTC's approach)
		[NSThread sleepForTimeInterval:0.1];
		
		// Try initializing again
		Status_Internal = AudioUnitInitialize(IOUnit);
	}
	
	if (Status_Internal == noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Voice Processing I/O unit initialized successfully after %d attempts"), failed_initialize_attempts + 1);
	}

	// Configure unit processing
#if UE_VERSION_NEWER_THAN(4, 25, 0)
	SetHardwareFeatureEnabled(Audio::EHardwareInputFeature::EchoCancellation, InParams.bUseHardwareAEC);
	SetHardwareFeatureEnabled(Audio::EHardwareInputFeature::AutomaticGainControl, InParams.bUseHardwareAEC);
#endif

	bIsStreamOpen = (Status_Internal == noErr);

	// At the end of the initialization, set the result and signal the semaphore
	bInitSucceeded = (Status_Internal == noErr);
	bIsStreamOpen = bInitSucceeded;

	if (bInitSucceeded)
	{
		UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Audio unit initialization succeeded"));
	}
	else
	{
		UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Audio unit initialization failed"));
	}

	dispatch_semaphore_signal(semaphore);

	});

	// Wait for initialization to complete with a reasonable timeout (5 seconds)
    const dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC);
    const long result = dispatch_semaphore_wait(semaphore, timeout);
    
    if (result != 0) {
        UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Timeout waiting for audio unit initialization"));
        bIsStreamOpen = false;
        return false;
    }

	return bInitSucceeded;
}

bool Audio::FAudioCaptureIOS::CloseStream()
{
	StopStream();
	if (IOUnit)
	{
		AudioComponentInstanceDispose(IOUnit);
		IOUnit = nullptr;
	}
	CaptureBuffer.Empty();
	BufferSize = 0;
	bIsStreamOpen = false;

#if ENGINE_MAJOR_VERSION >= 5
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	[AppDelegate SetFeature:EAudioFeature::Playback Active:false];
	[AppDelegate SetFeature:EAudioFeature::Record Active:false];
	[AppDelegate SetFeature:EAudioFeature::BackgroundAudio Active:false];

	UE_LOG(LogRuntimeAudioImporter, Log, TEXT("Disabled iOS audio session features"));
#endif

	return true;
}

bool Audio::FAudioCaptureIOS::StartStream()
{
	if (!IsStreamOpen() || IsCapturing() || !IOUnit)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Cannot start stream: Stream not open, already capturing, or IOUnit not initialized"));
		return false;
	}
	OSStatus status = AudioUnitReset(IOUnit, kAudioUnitScope_Global, 0);
	if (status != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Failed to reset audio unit: %d"), static_cast<int32>(status));
		return false;
	}
	status = AudioOutputUnitStart(IOUnit);
	bHasCaptureStarted = (status == noErr);

	if (!bHasCaptureStarted)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Failed to start audio unit: %d"), static_cast<int32>(status));
	}

	return bHasCaptureStarted;
}

bool Audio::FAudioCaptureIOS::StopStream()
{
	if (!IsStreamOpen() || !IsCapturing())
	{
		return false;
	}
	const bool bStopped = (AudioOutputUnitStop(IOUnit) == noErr);
	bHasCaptureStarted = !bStopped;
	return bStopped;
}

bool Audio::FAudioCaptureIOS::AbortStream()
{
	StopStream();
	CloseStream();
	return true;
}

bool Audio::FAudioCaptureIOS::GetStreamTime(double& OutStreamTime)
{
	OutStreamTime = 0.0f;
	return true;
}

bool Audio::FAudioCaptureIOS::IsStreamOpen() const
{
	return bIsStreamOpen;
}

bool Audio::FAudioCaptureIOS::IsCapturing() const
{
	return bHasCaptureStarted;
}

void Audio::FAudioCaptureIOS::OnAudioCapture(void* InBuffer, uint32 InBufferFrames, double StreamTime, bool bOverflow)
{
	OnCapture(static_cast<float*>(InBuffer), InBufferFrames, NumChannels, SampleRate, StreamTime, bOverflow);
}

bool Audio::FAudioCaptureIOS::GetInputDevicesAvailable(TArray<FCaptureDeviceInfo>& OutDevices)
{
	OutDevices.Reset();

	FCaptureDeviceInfo& DeviceInfo = OutDevices.AddDefaulted_GetRef();
	GetCaptureDeviceInfo(DeviceInfo, 0);

	return true;
}

#if UE_VERSION_NEWER_THAN(4, 25, 0)
void Audio::FAudioCaptureIOS::SetHardwareFeatureEnabled(Audio::EHardwareInputFeature FeatureType, bool bEnabled)
{
	if (IOUnit == nil)
	{
		return;
	}

	// we ignore result those functions because sometime we can't set this parameters
	OSStatus status = noErr;

	switch (FeatureType)
	{
	case Audio::EHardwareInputFeature::EchoCancellation:
	{
		const UInt32 EnableParam = (bEnabled ? 0 : 1);
		status = AudioUnitSetProperty(IOUnit,
			kAUVoiceIOProperty_BypassVoiceProcessing,
			kAudioUnitScope_Global,
			kInputBus,
			&EnableParam,
			sizeof(EnableParam)
		);
	}
	break;
	case Audio::EHardwareInputFeature::AutomaticGainControl:
	{
		const UInt32 EnableParam = (bEnabled ? 1 : 0);
		status = AudioUnitSetProperty(IOUnit,
			kAUVoiceIOProperty_VoiceProcessingEnableAGC,
			kAudioUnitScope_Global,
			kInputBus,
			&EnableParam,
			sizeof(EnableParam)
		);
	}
	break;
	}
}
#endif

OSStatus Audio::FAudioCaptureIOS::OnCaptureCallback(AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
	if (!IOUnit || !bIsStreamOpen)
	{
		return -1;
	}

	OSStatus status = noErr;

	const int32 NeededBufferSize = inNumberFrames * NumChannels * sizeof(float);
	if (CaptureBuffer.Num() == 0 || BufferSize < NeededBufferSize)
	{
		BufferSize = NeededBufferSize;
		AllocateBuffer(BufferSize);
	}

	AudioBufferList* BufferList = (AudioBufferList*)CaptureBuffer.GetData();
	for (int i = 0; i < BufferList->mNumberBuffers; ++i)
	{
		BufferList->mBuffers[i].mDataByteSize = (UInt32)BufferSize;
	}

	status = AudioUnitRender(IOUnit,
		ioActionFlags,
		inTimeStamp,
		inBusNumber,
		inNumberFrames,
		BufferList);

	if (status != noErr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Unable to process audio unit render callback for capture device: %d"), static_cast<int32>(status));
		return 0;
	}

	void* InBuffer = (void*)BufferList->mBuffers[0].mData; // only first channel ?!
	OnAudioCapture(InBuffer, inNumberFrames, 0.0, false); // need calculate timestamp

	return noErr;
}

void Audio::FAudioCaptureIOS::AllocateBuffer(int32 SizeInBytes)
{
	const size_t CaptureBufferBytes = sizeof(AudioBufferList) + NumChannels * (sizeof(AudioBuffer) + SizeInBytes);

	if (CaptureBufferBytes <= 0)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Invalid buffer size for allocation."));
		return;
	}

	CaptureBuffer.SetNumUninitialized(CaptureBufferBytes);

	AudioBufferList* list = (AudioBufferList*)CaptureBuffer.GetData();
	if (list == nullptr)
	{
		UE_LOG(LogRuntimeAudioImporter, Error, TEXT("Failed to allocate CaptureBuffer."));
		return;
	}

	uint8* CaptureBufferData = CaptureBuffer.GetData() + sizeof(AudioBufferList) + sizeof(AudioBuffer);

	list->mNumberBuffers = NumChannels;
	for (int32 Index = 0; Index < NumChannels; ++Index)
	{
		list->mBuffers[Index].mNumberChannels = 1;
		list->mBuffers[Index].mDataByteSize = (UInt32)SizeInBytes;
		list->mBuffers[Index].mData = CaptureBufferData;

		CaptureBufferData += (SizeInBytes + sizeof(AudioBuffer));
	}
}
#endif
#endif