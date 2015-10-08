#include "UEVideoRecorderPrivatePCH.h"
#include <d3d11.h>
#include <wrl.h>

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

void UVideoRecordGameViewportClient::Draw(FViewport *viewport, FCanvas *sceneCanvas)
{
	Super::Draw(viewport, sceneCanvas);

	bool captureGUI = this->captureGUI;
	auto frameSize = Viewport->GetSizeXY();
	if (captureGUI && FSlateApplication::IsInitialized())
	{
		const TSharedPtr<SWindow> windowPtr = GetWindow();
		if (captureGUI = windowPtr.IsValid())
		{
			TSharedRef<SWidget> windowRef = windowPtr.ToSharedRef();
			FIntVector size;
			if (captureGUI = FSlateApplication::Get().TakeScreenshot(windowRef, frame, size))
				frameSize.X = size.X, frameSize.Y = size.Y;
		}
	}
	if (captureGUI != this->captureGUI && !failGUI)
	{
		std::wcerr << "Fail to capture frame with GUI. Performing capture without GUI." << std::endl;	// data race on std::wcerr possible
		failGUI = true;
	}

	Draw(frameSize.X, frameSize.Y, [=](std::decay_t<FunctionTraits<decltype(&CVideoRecorder::Draw)>::arg<3>>::argument_type data)
	{
#if 1
		if (captureGUI)
			memcpy(data, frame.GetData(), frame.Num() * frame.GetTypeSize());
		else
		{
			const auto ok = Viewport->ReadPixelsPtr(reinterpret_cast<FColor *>(data), FReadSurfaceDataFlags(), FIntRect(0, 0, frameSize.X, frameSize.Y));
			assert(ok);
		}
#else
		using Microsoft::WRL::ComPtr;
		const auto texture = reinterpret_cast<ID3D11Texture2D *>(Viewport->GetRenderTargetTexture()->GetNativeResource());
		ComPtr<ID3D11Device> device;
		texture->GetDevice(&device);
#endif
	});
}

void UVideoRecordGameViewportClient::StartRecord(const wchar_t filename[])
{
	StartRecord(filename, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
}

void UVideoRecordGameViewportClient::StartRecord(const wchar_t filename[], EncodePerformance performance, int64_t crf)
{
	StartRecord(filename, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, performance, crf);
}