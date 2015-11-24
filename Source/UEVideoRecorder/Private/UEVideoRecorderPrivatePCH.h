#include "Engine.h"

#pragma warning(default : 4003 4238 4495 4497 4610 4706)

#ifdef DEBUG
#undef NDEBUG
#endif

#include "ViewportClientIncludes.h"
#include <tuple>
#include <functional>
#include <algorithm>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <locale>
#include <codecvt>
#include <cassert>