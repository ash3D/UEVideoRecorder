#pragma once
#include "Engine/GameViewportClient.h"
#include "AllowWindowsPlatformTypes.h"
#include <type_traits>
#include <iostream>
#include <string>
#include "boost/iostreams/concepts.hpp"
#include "boost/iostreams/stream_buffer.hpp"
#include "VideoRecorder.h"
#include "VideoRecordGameViewportClient.generated.h"

static_assert(std::is_same<TCHAR, wchar_t>::value, "Unicode mode must be set.");

DECLARE_LOG_CATEGORY_EXTERN(VideoRecorder, Log, All);

namespace bios = boost::iostreams;

class LogRedirect
{
	class LogSink : public bios::wsink
	{
		ELogVerbosity::Type verbosity;
		std::wstring msgStr;

	public:
		LogSink(ELogVerbosity::Type verbosity) : verbosity(verbosity) {}

	public:
		std::streamsize write(const wchar_t msg[], std::streamsize count);
	};

	std::wostream &src;
	bios::stream_buffer<LogSink> redirectStream;
	std::wstreambuf *const oldbuff;

public:
	LogRedirect(std::wostream &src, ELogVerbosity::Type verbosity);
	~LogRedirect();
};

template<std::wostream &Src, ELogVerbosity::Type Verbosity>
struct LogRedirectProxy : private LogRedirect
{
	LogRedirectProxy() : LogRedirect(Src, Verbosity) {}
};

/**
*
*/
UCLASS()
class UVideoRecordGameViewportClient :
	public UGameViewportClient,
	public/*private*/ LogRedirectProxy<std::wcerr, ELogVerbosity::Error>,
	public/*private*/ LogRedirectProxy<std::wclog, ELogVerbosity::Log>,
	public CVideoRecorder
{
	GENERATED_BODY()

	void Draw(FViewport *viewport, FCanvas *sceneCanvas) override;

public:
	void CaptureGUI(bool enable) { captureGUI = enable; }
	void StartRecord(const wchar_t filename[]);

private:
	using CVideoRecorder::Draw;
	using CVideoRecorder::StartRecord;

private:
	TArray<FColor> frame;
	bool captureGUI = false, failGUI = false;
};