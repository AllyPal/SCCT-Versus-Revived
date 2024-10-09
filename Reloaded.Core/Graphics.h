#pragma once
#include <Windows.h>
#include <chrono>

struct DisplayModeOverride {
    wchar_t* displayText;
    int length;
    int alsoLength;
};

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
    static DisplayModeOverride* videoSettingsDisplayModes;
    static int GetResolutionCount();
};
