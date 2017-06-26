#pragma once
#include "ViewportClientIncludes.h"
#include "VideoRecordEnums.h"
#include "VideoRecordGameViewportClient.generated.h"

#define LEGACY		0
#define FORCE_SYNC	0

#if LEGACY && FORCE_SYNC
#error FORCE_SYNC incompatible with LEGACY
#endif

#if !(LEGACY || FORCE_SYNC)
#define ENABLE_ASYNC
#endif

static_assert(std::is_same<TCHAR, wchar_t>::value, "Unicode mode must be set.");

#ifdef ENABLE_ASYNC
namespace WRL = Microsoft::WRL;
// forward decl
namespace std
{
	class excetption;
}
#endif

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

	UVideoRecordGameViewportClient();

#ifdef ENABLE_ASYNC
	~UVideoRecordGameViewportClient();
#endif

	void Draw(FViewport *viewport, FCanvas *sceneCanvas) override;

public:
	void CaptureGUI(bool enable) { captureGUI = enable; }
	void StartRecord(std::wstring filename, unsigned int width, unsigned int height, ::VideoFormat format, ::FPS fps, ::Codec codec, int64_t crf = -1, ::Preset preset = ::Preset::Default);
#ifdef ENABLE_ASYNC
	void StopRecord();
	void Screenshot(std::wstring filename);
#endif

private:
	inline void StartRecordImpl(std::wstring &&filename, unsigned int width, unsigned int height, Format format, FPS fps, Codec codec, int64_t crf, Preset preset);

#ifdef ENABLE_ASYNC
private:
	void Error(), Error(HRESULT hr), Error(const std::exception &error);
#endif

private:
#if LEGACY
	using CVideoRecorder::Draw;
#else
	using CVideoRecorder::SampleFrame;
#endif
	using CVideoRecorder::StartRecord;
#ifdef ENABLE_ASYNC
	// make private for ASYNC mode
	using CVideoRecorder::StopRecord;
	using CVideoRecorder::Screenshot;
#endif

private:
#if !LEGACY
	template<bool async>
	class CFrame;
#ifdef ENABLE_ASYNC
	std::deque<std::shared_ptr<CFrame<true>>> frameQueue;
	class CTexturePool
	{
		struct TTexture
		{
			WRL::ComPtr<ID3D11Texture2D> texture;
			unsigned long int idleTime;

		public:
			TTexture(WRL::ComPtr<ID3D11Texture2D> &&texture) : texture(std::move(texture)), idleTime() {}
		};
		std::forward_list<TTexture> pool;	// consider using std::vector instead
		static constexpr unsigned long int maxIdle = 10u;

	private:
		static inline bool Unused(decltype(pool)::const_reference texture);

	public:
		WRL::ComPtr<ID3D11Texture2D> GetTexture(ID3D11Device *device, DXGI_FORMAT format, unsigned int width, unsigned int height);
		void NextFrame();
	} texturePool;
	const bool async = DetectAsyncMode();
	static bool DetectAsyncMode();
#endif
#endif
	bool captureGUI = false;
};