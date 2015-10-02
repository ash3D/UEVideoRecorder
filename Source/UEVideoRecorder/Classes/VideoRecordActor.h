#pragma once
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
	void StartRecord(const FString &filename);

	UFUNCTION(BlueprintCallable, Category = "VideoRecord")
	void StopRecord();

	UFUNCTION(BlueprintCallable, Category = "Screenshot")
	void Screenshot(const FString &filename);
};
