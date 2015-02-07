

#ifndef FontPlatformData_h
#define FontPlatformData_h

#if PLATFORM(WIN_OS)
#include "FontPlatformDataChromiumWin.h"
#elif defined(__linux__)
#include "FontPlatformDataLinux.h"
#endif

#endif  // FontPlatformData_h
