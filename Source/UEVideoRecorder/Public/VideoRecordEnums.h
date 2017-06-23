#pragma once

UENUM(BlueprintType)
enum class VideoFormat : uint8
{
	AUTO,
	_8	UMETA(DisplayName = "8 bit"),
	_10	UMETA(DisplayName = "10 bit (if supported)"),
};

UENUM(BlueprintType)
enum class Codec : uint8
{
	H264,
	HEVC,
};

UENUM(BlueprintType)
enum class Preset : uint8	// UE4 currenly supports only uint8, switch to signed when it will be supported
{
	placebo,
	veryslow,
	slower,
	slow,
	medium,
	fast,
	faster,
	veryfast,
	superfast,
	ultrafast,
	Default = 255
};