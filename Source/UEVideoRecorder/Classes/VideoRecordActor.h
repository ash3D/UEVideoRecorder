#pragma once
#include "VideoRecordEnums.h"
#include "GameFramework/Actor.h"
#include "VideoRecordActor.generated.h"

UCLASS(meta = (ShortTooltip = "This is a proxy that provides interface to video recording and screenshot taking functionality."))
class AVideoRecordActor : public AActor
{
	GENERATED_BODY()
	class UVideoRecordGameViewportClient *viewport;

public:	
	// Sets default values for this actor's properties
	AVideoRecordActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void CaptureGUI(bool enable);

	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void StartRecord(const FString &filename,
		UPARAM(DisplayName = "width (0 - autodetection)") int width, UPARAM(DisplayName = "height (0 - autodetection)") int height,
		VideoFormat format = VideoFormat::AUTO,
		FPS fps = FPS::thirty,
		Codec codec = Codec::HEVC,
		UPARAM(DisplayName = "crf (-1 - default)") int crf = -1,// UE4 currently unable to parse default values for int64 => use int for now
		Preset preset = Preset::Default);

	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void StopRecord();

	UFUNCTION(BlueprintCallable, Category = "Screenshot")
	void Screenshot(const FString &filename);

private:
	// use ViewportClient template param instead of explicit UVideoRecordGameViewportClient in order to allow using forward decl for UVideoRecordGameViewportClient in header
	template<class ViewportClient, typename ...Args, typename ...Params>
	inline void ViewportProxy(void (ViewportClient::*f)(Args ...), Params ...);
};
