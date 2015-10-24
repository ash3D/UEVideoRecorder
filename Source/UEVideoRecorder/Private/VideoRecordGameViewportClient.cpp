#include "UEVideoRecorderPrivatePCH.h"
#include "VideoRecordGameViewportClient.h"

#if defined _MSC_VER && _MSC_VER < 1900
#define noexcept
#endif

#pragma region Traits
template<typename Signature>
struct FunctionTraits;

template<typename Ret, typename ...Args>
struct FunctionTraits<Ret(Args...)>
{
	typedef Ret ret;
	static const/*expr*/ auto arity = sizeof...(Args);
	template<size_t i>
	using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

#define DECL_MEMBER_FUNCTION_TRAITS(cv)															\
	template<class Class, typename Ret, typename ...Args>										\
	struct FunctionTraits<Ret (Class::*)(Args...) cv> : FunctionTraits<Ret (Class &, Args...)>	\
	{};

DECL_MEMBER_FUNCTION_TRAITS()
DECL_MEMBER_FUNCTION_TRAITS(const)
DECL_MEMBER_FUNCTION_TRAITS(volatile)
DECL_MEMBER_FUNCTION_TRAITS(const volatile)

#undef DECL_MEMBER_FUNCTION_TRAITS

#define DECL_HAS_MEMBER(Member)					\
	template<class Class, typename = void>		\
	struct Has_##Member : std::false_type {};	\
												\
	template<class Class>						\
	struct Has_##Member<Class, decltype(std::declval<Class>().Member, void()) : std::true_type {};

#define HAS_MEMBER(Class, Member) Has_##Member<Class>::value;

#define DECL_HAS_FUNCTION(Function)													\
	template<class Class>															\
	class HasFunction_##Function													\
	{																				\
		struct Ambiguator															\
		{																			\
			void Function() {}														\
		};																			\
																					\
		struct Derived : Class, Ambiguator {};										\
																					\
		template<void (Ambiguator::*)()>											\
		struct Absorber;															\
																					\
		template<class CheckedClass>												\
		static std::false_type Check(Absorber<&CheckedClass::Function> *);			\
																					\
		template<class CheckedClass>												\
		static std::true_type Check(...);											\
																					\
	public:																			\
		static const/*expr*/ bool value = decltype(Check<Derived>(nullptr))::value;	\
	};

#define HAS_FUNCTION(Class, Function) HasFunction_##Function<Class>::value
#pragma endregion

#pragma region Logger
	DECL_HAS_FUNCTION(Logf)
	DECL_HAS_FUNCTION(Logf_Internal)

	DEFINE_LOG_CATEGORY(VideoRecorder)

#if 0
namespace
{
	template<typename Signature>
	struct LogSignature;

	template<typename Ret, typename ...Args>
	struct LogSignature<Ret(Args)>
	{
		typedef Ret(*Log)(Args...);
	}

	template<typename ...Args, typename Ret, Ret(*Log)(Args...)>
	inline void Log(ELogVerbosity::Type verbosity, const wchar_t msgStr[])
	{
		Log(__FILE__, __LINE__, VideoRecorder.GetCategoryName(), verbosity, msgStr);
	}
}
#elif 0
#define LOG_ARGS __FILE__, __LINE__, VideoRecorder.GetCategoryName(), verbosity, msgStr.c_str()
#endif

template<bool = true, typename = void>
struct Logger
{
	static void Log(...) {}
};

#define DECL_LOGGER(Logf, condition)																	\
	template<bool True>																					\
	struct Logger<True, typename std::enable_if<True && HAS_FUNCTION(FMsg, Logf) && condition>::type>	\
	{																									\
		template<typename ...Args>																		\
		static auto Log(Args &&...args) -> decltype(FMsg::Logf(std::declval<Args>()...))				\
		{																								\
			return FMsg::Logf(std::forward<Args>(args)...);												\
		}																								\
	};

DECL_LOGGER(Logf, true)
DECL_LOGGER(Logf_Internal, !HAS_FUNCTION(FMsg, Logf))
#pragma endregion

#pragma region LogRedirect
std::streamsize LogRedirect::LogSink::write(const wchar_t msg[], std::streamsize count)
{
	msgStr.assign(msg, count);
	//UE_LOG(LogTemp, verbosity, msgStr.c_str());
#if NO_LOGGING
	if (verbosity == ELogVerbosity::Fatal)
		FError::LowLevelFatal(__FILE__, __LINE__, msgStr.c_str());
#else
#if 0
	Log<FMsg::Logf>(verbosity, msgStr.c_str());
#elif 0
	FMsg::Logf(__FILE__, __LINE__, VideoRecorder.GetCategoryName(), verbosity, msgStr.c_str());
#else
	Logger<>::Log(__FILE__, __LINE__, VideoRecorder.GetCategoryName(), verbosity, msgStr.c_str());
#endif
#endif
	return count;
}

LogRedirect::LogRedirect(std::wostream &src, ELogVerbosity::Type verbosity) :
	src(src),
	redirectStream(LogSink(verbosity)),
	oldbuff(src.rdbuf(&redirectStream))
{}

LogRedirect::~LogRedirect()
{
	src.rdbuf(oldbuff);
}
#pragma endregion

#if !LEGACY
#if ASYNC

using Microsoft::WRL::ComPtr;

/*
	Due to bug in VS 2013 it inaccessible via '::' if place it in anonimous namespace
	TODO: move it in anonimous namespace after migration to VS 2015
*/
static inline bool Unused(IUnknown *object)
{
	object->AddRef();
	return object->Release() == 1;
}

namespace
{
	inline void AssertHR(HRESULT hr)
	{
		assert(SUCCEEDED(hr));
	}

	inline void CheckHR(HRESULT hr)
	{
		if (FAILED(hr))
			throw hr;
	}

	/*
		Texture pool currently global for all game viewport clients.
		It is necessary to make sure that it is safe to use multiple game viewport clients with single pool (including thread safaty) before doing it.
	*/
	class CTexturePool
	{
		struct TTexture
		{
			ComPtr<ID3D11Texture2D> texture;
			unsigned long int idleTime;

		public:
			TTexture(ComPtr<ID3D11Texture2D> &&texture) : texture(std::move(texture)), idleTime() {}
		};
		std::forward_list<TTexture> pool;	// consider using std::vector instead
		static const/*expr*/ unsigned long int maxIdle = 10u;

	private:
		static inline bool Unused(decltype(pool)::const_reference texture);

	public:
		ComPtr<ID3D11Texture2D> GetTexture(ID3D11Device *device, DXGI_FORMAT format, unsigned int width, unsigned int height);
		void NextFrame();
	} texturePool;

	// propably will not be inlined if pass it to algos, functor/lambda needed in order to it happens
	inline bool CTexturePool::Unused(decltype(pool)::const_reference texture)
	{
		return ::Unused(texture.texture.Get());
	}

#define SEARCH_SMALLEST 1
	ComPtr<ID3D11Texture2D> CTexturePool::GetTexture(ID3D11Device *device, DXGI_FORMAT format, unsigned int width, unsigned int height)
	{
		const auto IsAcceptable = [format](decltype(pool)::const_reference texture)
		{
			return Unused(texture) && [&texture, format]
			{
				D3D11_TEXTURE2D_DESC desc;
				texture.texture->GetDesc(&desc);
				return desc.Format == format;
			}();
		};
#if SEARCH_SMALLEST
		const auto cached = std::min_element(pool.begin(), pool.end(), [format](decltype(pool)::const_reference left, decltype(pool)::const_reference right)
		{
			const bool leftUnused = Unused(left), rightUnused = Unused(right);
			if (leftUnused != rightUnused)
				return leftUnused;

			D3D11_TEXTURE2D_DESC leftDesc, rightDesc;
			left.texture->GetDesc(&leftDesc);
			right.texture->GetDesc(&rightDesc);
			if (leftDesc.Format != rightDesc.Format)
				return leftDesc.Format == format;
			return leftDesc.Width * leftDesc.Height < rightDesc.Width * rightDesc.Height;
		});
#else
		const auto cached = std::find_if(pool.begin(), pool.end(), IsAcceptable);
#endif

#if SEARCH_SMALLEST
		if (cached != pool.end() && IsAcceptable(*cached))
#else
		if (cached != pool.end())
#endif
		{
			cached->idleTime = 0;
			return cached->texture;
		}

		ComPtr<ID3D11Texture2D> texture;
		CheckHR(device->CreateTexture2D(&CD3D11_TEXTURE2D_DESC(format, width, height, 1, 0, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ), NULL, &texture));
		pool.emplace_front(std::move(texture));
		return pool.front().texture;
	}
#undef SEARCH_SMALLEST

	void CTexturePool::NextFrame()
	{
		pool.remove_if([](decltype(pool)::reference texture)
		{
			return Unused(texture) && ++texture.idleTime > maxIdle;
		});
	}
}

#pragma region CFrame
class UVideoRecordGameViewportClient::CFrame final : public CVideoRecorder::CFrame
{
	ComPtr<ID3D11Texture2D> stagingTexture;
	FrameData frameData;

private:
	FrameData GetFrameData() const override { return frameData; }

private:
	ComPtr<ID3D11DeviceContext> GetContext() const;

public:
	CFrame(CFrame::Opaque opaque, ID3D11Device *device, DXGI_FORMAT format, unsigned int width, unsigned int height);
	~CFrame() override;

public:
	void operator =(ID3D11Texture2D *src);
	bool TryMap();
};

ComPtr<ID3D11DeviceContext> UVideoRecordGameViewportClient::CFrame::GetContext() const
{
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	stagingTexture->GetDevice(&device);
	device->GetImmediateContext(&context);
	return context;
}

UVideoRecordGameViewportClient::CFrame::CFrame(CFrame::Opaque opaque, ID3D11Device *device, DXGI_FORMAT format, unsigned int width, unsigned int height) :
CVideoRecorder::CFrame(std::forward<CFrame::Opaque>(opaque)),
stagingTexture(texturePool.GetTexture(device, format, width, height)),
frameData()
{
	switch (format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		frameData.format = FrameData::Format::B8G8R8A8;
		break;
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		frameData.format = FrameData::Format::R10G10B10A2;
		break;
	default:
		throw std::logic_error("invalid texture format");
	}
	frameData.width = width;
	frameData.height = height;
}

UVideoRecordGameViewportClient::CFrame::~CFrame()
{
	if (frameData.pixels)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UnmapCommand,
			const ComPtr<ID3D11Texture2D>, texture, stagingTexture,
			{
				ComPtr<ID3D11Device> device;
				ComPtr<ID3D11DeviceContext> context;
				texture->GetDevice(&device);
				device->GetImmediateContext(&context);
				context->Unmap(texture.Get(), 0);
			});
	}
}

void UVideoRecordGameViewportClient::CFrame::operator =(ID3D11Texture2D *src)
{
	D3D11_TEXTURE2D_DESC desc;
	src->GetDesc(&desc);
	const CD3D11_BOX box(0, 0, 0, desc.Width, desc.Height, 1);
	GetContext()->CopySubresourceRegion(stagingTexture.Get(), 0, 0, 0, 0, src, 0, &box);
}

bool UVideoRecordGameViewportClient::CFrame::TryMap()
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	switch (const HRESULT hr = GetContext()->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped))
	{
	case S_OK:
		frameData.stride = mapped.RowPitch;
		frameData.pixels = mapped.pData;
		Ready();
		return true;
	case DXGI_ERROR_WAS_STILL_DRAWING:
		return false;
	default:
		throw hr;
	}
}
#pragma endregion

#define ERROR_MSG_PREFIX TEXT("An Error has occured: ")
#define ERROR_MSG_POSTFIX TEXT(". Any remaining pending frames being canceled.")

namespace
{
	template<typename Char = char>
	inline typename std::enable_if<std::is_same<Char, TCHAR>::value, const Char *>::type Str_char_2_TCHAR(const char *str)
	{
		return str;
	}

	template<typename Char>
	class c_str
	{
		std::basic_string<Char> str;

	public:
		c_str(decltype(str) &&str) noexcept : str(std::move(str)) {}
		operator const Char *() const noexcept { return str.c_str(); }
	};

	template<typename Char = char>
	typename std::enable_if<!std::is_same<Char, TCHAR>::value, c_str<TCHAR>>::type Str_char_2_TCHAR(const char *str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<TCHAR>> converter;
		return converter.from_bytes(str);
	}
}
#else
class UVideoRecordGameViewportClient::CFrame final : public CVideoRecorder::CFrame
{
	TArray<FColor> frame;
	FIntPoint frameSize;

private:
	FrameData GetFrameData() const override;

public:
	CFrame(CFrame::Opaque opaque, FViewport *viewport);
};

UVideoRecordGameViewportClient::CFrame::CFrame(CFrame::Opaque opaque, FViewport *viewport) :
CVideoRecorder::CFrame(std::forward<CFrame::Opaque>(opaque)),
frameSize(viewport->GetSizeXY())
{
	const auto ok = viewport->ReadPixels(frame, FReadSurfaceDataFlags(), FIntRect(0, 0, frameSize.X, frameSize.Y));
	assert(ok);
}

auto UVideoRecordGameViewportClient::CFrame::GetFrameData() const -> FrameData
{
	return{ FrameData::Format::B8G8R8A8, frameSize.X, frameSize.Y, frameSize.X * sizeof FColor, (const uint8_t *)frame.GetData() };
}
#endif
#endif

void UVideoRecordGameViewportClient::Draw(FViewport *viewport, FCanvas *sceneCanvas)
{
	if (!captureGUI)
		Super::Draw(viewport, sceneCanvas);

#if LEGACY
	FIntPoint frameSize = Viewport->GetSizeXY();
	Draw(frameSize.X, frameSize.Y, [=](std::decay_t<FunctionTraits<decltype(&CVideoRecorder::Draw)>::arg<3>>::argument_type data)
	{
		const auto ok = Viewport->ReadPixelsPtr(reinterpret_cast<FColor *>(data), FReadSurfaceDataFlags(), FIntRect(0, 0, frameSize.X, frameSize.Y));
		assert(ok);
	});
#else
#ifdef GAME
#define GET_TEXTURE GetViewportRHI()->GetNativeBackBufferTexture()
#else
#define GET_TEXTURE GetRenderTargetTexture()->GetNativeResource()
#endif
#if ASYNC
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ProcessVideoRecordingCommand,
		UVideoRecordGameViewportClient &, viewportClient, *this,
		{
			try
			{
				// try to map ready frames
				while (!viewportClient.frameQueue.empty())
				{
					if (!viewportClient.frameQueue.front()->TryMap())
						break;
					viewportClient.frameQueue.pop_front();
				}

				viewportClient.SampleFrame([this](CFrame::Opaque opaque)
				{
					//ID3D11Texture2D *const rt = reinterpret_cast<ID3D11Texture2D *>(viewportClient.Viewport->GET_TEXTURE);
					ID3D11Texture2D *const rt = reinterpret_cast<ID3D11Texture2D *>(viewportClient.Viewport->GetViewportRHI() ? viewportClient.Viewport->GetViewportRHI()->GetNativeBackBufferTexture() : viewportClient.Viewport->GetRenderTargetTexture()->GetNativeResource());
					assert(rt);

					ComPtr<ID3D11Device> device;
					rt->GetDevice(&device);

					ComPtr<ID3D11DeviceContext> context;
					device->GetImmediateContext(&context);

					D3D11_TEXTURE2D_DESC desc;
					rt->GetDesc(&desc);

					viewportClient.frameQueue.push_back(std::make_shared<CFrame>(std::forward<CFrame::Opaque>(opaque), device.Get(), desc.Format, desc.Width, desc.Height));
					*viewportClient.frameQueue.back() = rt;

					return viewportClient.frameQueue.back();
				});

				texturePool.NextFrame();
			}
			catch (HRESULT hr)
			{
				UE_LOG(VideoRecorder, Error, ERROR_MSG_PREFIX TEXT("hr=%d") ERROR_MSG_POSTFIX, hr);
				viewportClient.Error();
			}
			catch (const std::exception &error)
			{
				UE_LOG(VideoRecorder, Error, ERROR_MSG_PREFIX TEXT("%s") ERROR_MSG_POSTFIX, (const TCHAR *)Str_char_2_TCHAR(error.what()));
				viewportClient.Error();
			}
		});
#else
	SampleFrame([this](CFrame::Opaque opaque)
	{
		auto frame = std::make_shared<CFrame>(std::forward<CFrame::Opaque>(opaque), Viewport);
		frame->Ready();
		return frame;
	});
#endif
#endif

	if (captureGUI)
		Super::Draw(viewport, sceneCanvas);
}

// 1 call site
inline void UVideoRecordGameViewportClient::StartRecordImpl(std::wstring &&filename, unsigned int width, unsigned int height, const EncodeConfig &config)
{
	if (!(width && height))
	{
		width = Viewport->GetSizeXY().X;
		height = Viewport->GetSizeXY().Y;
	}
	CVideoRecorder::StartRecord(std::move(filename), width, height, config);
}

#if ASYNC
void UVideoRecordGameViewportClient::StartRecord(std::wstring filename, unsigned int width, unsigned int height, const EncodeConfig &config)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_FIVEPARAMETER(
		StartRecordCommand,
		UVideoRecordGameViewportClient &, viewportClient, *this,
		std::wstring, filename, filename,
		const unsigned int, width, width,
		const unsigned int, height, height,
		const EncodeConfig, config, config,
		{
			viewportClient.StartRecordImpl(std::move(filename), width, height, config);
		});
}

void UVideoRecordGameViewportClient::StopRecord()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		StopRecordCommand,
		UVideoRecordGameViewportClient &, viewportClient, *this,
		{
			viewportClient.CVideoRecorder::StopRecord();
		});
}

void UVideoRecordGameViewportClient::Screenshot(std::wstring filename)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ScreenshotCommand,
		UVideoRecordGameViewportClient &, viewportClient, *this,
		std::wstring, filename, filename,
		{
			viewportClient.CVideoRecorder::Screenshot(std::move(filename));
		});
}
#else
void UVideoRecordGameViewportClient::StartRecord(std::wstring filename, unsigned int width, unsigned int height, const EncodeConfig &config)
{
	StartRecordImpl(std::move(filename), width, height, config);
}
#endif

#if ASYNC
void UVideoRecordGameViewportClient::Error()
{
	CVideoRecorder::StopRecord();
	std::for_each(frameQueue.cbegin(), frameQueue.cend(), std::mem_fn(&CFrame::Cancel));
	frameQueue.clear();
}
#endif