#pragma once

UENUM(BlueprintType)
enum class VideoFormat : uint8
{
	AUTO,
	_8	UMETA(DisplayName = "8 bit"),
	_10	UMETA(DisplayName = "10 bit (if supported)"),
};

// UE4 currently unable to parse default values such as FPS::_30 for blueprint callable functions

//UENUM(BlueprintType)
//enum class FPS : uint8
//{
//	_25 = 25	UMETA(DisplayName = "25"),
//	_30 = 30	UMETA(DisplayName = "30"),
//	_60 = 60	UMETA(DisplayName = "60"),
//};
UENUM(BlueprintType)
enum class FPS : uint8
{
	twenty_five = 25	UMETA(DisplayName = "25"),
	thirty		= 30	UMETA(DisplayName = "30"),
	sixty		= 60	UMETA(DisplayName = "60"),
};

UENUM(BlueprintType)
enum class Codec : uint8
{
	H264,
	HEVC,
};

// UE4 currenly supports only uint8, switch to signed for presets when it will be supported

UENUM(BlueprintType)
enum class Preset : uint8
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

UENUM(BlueprintType)
enum class PresetNV : uint8
{
	slow,
	medium,
	fast,
	hp,
	hq,
	bd,
	ll,
	llhq,
	llhp,
	lossless,
	losslesshp,
	Default = 255
};