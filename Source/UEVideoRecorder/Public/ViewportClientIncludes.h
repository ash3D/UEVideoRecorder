#pragma once
#include "Engine/GameViewportClient.h"
#include "AllowWindowsPlatformTypes.h"
#include <type_traits>
#include <memory>
#include <deque>
#include <iostream>
#include <string>
#include <mutex>
#include <cstdint>
#include <winerror.h>	// for HRESULT
#include "boost/iostreams/concepts.hpp"
#include "boost/iostreams/stream_buffer.hpp"
#include "VideoRecorder.h"