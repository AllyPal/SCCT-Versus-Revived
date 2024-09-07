#pragma once

#include <string>

class Config {
public:
    static int frameRateLimit;
    static int listenServerFrameRateLimit;
    static int frameTimingMode;
    static bool applyAnimationFix;
    static bool applyWidescreenFix;

    static bool useDirectConnect;
    static std::wstring directConnectIp;
    static std::wstring directConnectPort;
    static bool mouseInputFix;

    // Constructor to initialize the config from a wide string path
    static void Initialize(std::wstring& configFilePath);
    static void ProcessCommandLine();
    static void Serialize();
};

