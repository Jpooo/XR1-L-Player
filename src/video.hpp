#pragma once
#include "video_common.hpp"
#ifdef _WIN32
#include "video_windows.hpp"
#else
#include "video_linux.hpp"
#endif
