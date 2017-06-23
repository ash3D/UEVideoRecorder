#include "UEVideoRecorderPrivatePCH.h"
#include "VideoRecordActor.h"
#include "VideoRecordGameViewportClient.h"


// Sets default values
AVideoRecordActor::AVideoRecordActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve preset if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AVideoRecordActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (const auto viewportIterator = TObjectIterator<UVideoRecordGameViewportClient>())
	{
		viewport = *viewportIterator;
		assert(viewport);
	}
	else
		UE_LOG(VideoRecorder, Error, TEXT("VideoRecordGameViewportClient is not registered. Any record operations will be ignored. Set VideoRecordGameViewportClient in \"Project Settings -> General Setting -> Game Viewport Client Class\" in order to enable video record and screenshot functionality."));
}

template<class ViewportClient, typename ...Args, typename ...Params>
inline void AVideoRecordActor::ViewportProxy(void (ViewportClient::*f)(Args ...args), Params ...params)
{
	if (viewport)
		(viewport->*f)(std::forward<Params>(params)...);
}

void AVideoRecordActor::CaptureGUI(bool enable)
{
	ViewportProxy(&UVideoRecordGameViewportClient::CaptureGUI, enable);
}

void AVideoRecordActor::StartRecord(const FString &filename, int width, int height, VideoFormat format, FPS fps, Codec codec, int crf, Preset preset)
{
	void (UVideoRecordGameViewportClient::*const StartRecord)(std::wstring, unsigned int, unsigned int, VideoFormat, FPS, Codec, int64_t, Preset) = &UVideoRecordGameViewportClient::StartRecord;
	ViewportProxy(StartRecord, *filename, width, height, format, fps, codec, crf, preset);
}

void AVideoRecordActor::StopRecord()
{
	ViewportProxy(&UVideoRecordGameViewportClient::StopRecord);
}

void AVideoRecordActor::Screenshot(const FString &filename)
{
	ViewportProxy(&UVideoRecordGameViewportClient::Screenshot, *filename);
}