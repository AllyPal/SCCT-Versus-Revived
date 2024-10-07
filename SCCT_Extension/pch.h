// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
constexpr auto NOP = 0x90;
constexpr auto consoleKeyBind = 0x29;
constexpr int frameRateLimit_hosting_min = 30;
constexpr int frameRateLimit_hosting_max = 165;
constexpr int frameRateLimit_client_min = 30;
constexpr int frameRateLimit_client_max = 1000;

// add headers that you want to pre-compile here
#include "framework.h"

#endif //PCH_H
