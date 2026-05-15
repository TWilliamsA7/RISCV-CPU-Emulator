// inc/platform/platform.hpp

#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#else
    #define PLATFORM_POSIX
#endif