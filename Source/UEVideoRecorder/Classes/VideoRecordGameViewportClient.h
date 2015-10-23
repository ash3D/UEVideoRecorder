#pragma once
#include "ViewportClientIncludes.h"
#include "VideoRecordGameViewportClient.generated.h"

#define LEGACY	0
#define ASYNC	1

#if LEGACY && ASYNC
#error ASYNC incompatible with LEGACY
#endif

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
UCLASS(meta = (ShortTooltip = "Captures frames for video recording and screenshots. Activated via selection it in \"Project Settings -> General Setting -> Game Viewport Client Class\"."))
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
	void StartRecord(std::wstring filename, unsigned int width, unsigned int height, const EncodeConfig &config = { -1 });
#if ASYNC
	void StopRecord();
	void Screenshot(std::wstring filename);
#endif

private:
	inline void StartRecordImpl(std::wstring &&filename, unsigned int width, unsigned int height, const EncodeConfig &config);

#if ASYNC
private:
	void Error();
#endif

private:
#if LEGACY
	using CVideoRecorder::Draw;
#else
	using CVideoRecorder::SampleFrame;
#endif
	using CVideoRecorder::StartRecord;
#if ASYNC
	// make private for ASYNC mode
	using CVideoRecorder::StopRecord;
	using CVideoRecorder::Screenshot;
#endif

private:
#if LEGACY
	TArray<FColor> frame;
#else
	class CFrame;
#if ASYNC
	std::deque<std::shared_ptr<CFrame>> frameQueue;
#endif
#endif
	bool captureGUI = false, failGUI = false, TryCaptureGUI(TArray<FColor> &frame, FIntPoint &frameSize);
};