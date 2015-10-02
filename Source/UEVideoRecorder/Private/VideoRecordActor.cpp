#include "UEVideoRecorderPrivatePCH.h"
#include "VideoRecordActor.h"


// Sets default values
AVideoRecordActor::AVideoRecordActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AVideoRecordActor::BeginPlay()
{
	Super::BeginPlay();
	
	viewport = *TObjectIterator<UVideoRecordGameViewportClient>();
	assert(viewport);
}

void AVideoRecordActor::CaptureGUI(bool enable)
{
	viewport->CaptureGUI(enable);
}

void AVideoRecordActor::StartRecord(const FString &filename)
{
	viewport->StartRecord(*filename);
}

void AVideoRecordActor::StopRecord()
{
	viewport->StopRecord();
}

void AVideoRecordActor::Screenshot(const FString &filename)
{
	viewport->Screenshot(*filename);
}