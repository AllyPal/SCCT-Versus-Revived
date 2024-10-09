#pragma once
#include <Windows.h>
#include <chrono>
#include "GameStructs.h"

struct DisplayModePair {
    std::string modeWithFormat;
    std::string modeWithAsterisk;
    UINT width;
    UINT height;
    std::string aspectRatio;
};

struct ResolutionInfo {
    UINT width;
    UINT height;
    UINT refreshRate;
};

struct KnownAspectRatio {
    double width;
    double height;
    std::string name;
};

class Graphics
{
public:
	static void Initialize();
	static std::chrono::steady_clock::time_point lastFrameTime;
    static UcString* videoSettingsDisplayModes;
    static int GetResolutionCount();
};
