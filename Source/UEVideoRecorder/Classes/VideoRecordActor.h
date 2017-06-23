#pragma once
#include "GameFramework/Actor.h"
#include "VideoRecordActor.generated.h"

#define RAW_STRING_LITERALS 1

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

	// it seems that UE4 header generation tool currenly does not support raw string literals, use escape characters for now
	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void StartRecord(const FString &filename,
		UPARAM(DisplayName = "width (0 - autodetection)") int width, UPARAM(DisplayName = "height (0 - autodetection)") int height,
//		UPARAM(DisplayName = R"(format
//0 - autodetection
//1 - 8 bit
//2 - 10 bit (if supported))")
		UPARAM(DisplayName = "format\n0 - autodetection\n1 - 8 bit\n2 - 10 bit (if supported)")
		int format,
		UPARAM(DisplayName = "highFPS (30/60)") bool highFPS,
//		UPARAM(DisplayName = R"(codec
//0 - H264
//1 - H265/HEVC)")
		UPARAM(DisplayName = "codec\n0 - H264\n1 - H265/HEVC")
		int codec = 1,
		UPARAM(DisplayName = "crf (-1 - default)") int crf = -1,// UE4 currently unable to parse default values for int64 => use int for now
//		UPARAM(DisplayName = R"(preset
//-1 - default
// 0 - placebo
// 1 - veryslow
// 2 - slower
// 3 - slow
// 4 - medium
// 5 - fast
// 6 - faster
// 7 - veryfast
// 8 - superfast
// 9 - ultrafast)")
		UPARAM(DisplayName = "preset\n-1 - default\n 0 - placebo\n 1 - veryslow\n 2 - slower\n 3 - slow\n 4 - medium\n 5 - fast\n 6 - faster\n 7 - veryfast\n 8 - superfast\n 9 - ultrafast")
		int preset = -1);

	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void StopRecord();

	UFUNCTION(BlueprintCallable, Category = "Screenshot")
	void Screenshot(const FString &filename);

private:
	// use ViewportClient template param instead of explicit UVideoRecordGameViewportClient in order to allow using forward decl for UVideoRecordGameViewportClient in header
	template<class ViewportClient, typename ...Args, typename ...Params>
	inline void ViewportProxy(void (ViewportClient::*f)(Args ...), Params ...);
};
