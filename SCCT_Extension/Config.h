#pragma once

#include <string>

class Config {
public:
    static int frameRateLimit_client;
    static int frameRateLimit_hosting;
    static int frameTimingMode;
    static bool applyAnimationFix;
    static bool applyWidescreenFix;
    static bool frameRateLimit_client_unlock;

    static bool useDirectConnect;
    static std::wstring directConnectIp;
    static std::wstring directConnectPort;
    static bool mouseInputFix;

    // Constructor to initialize the config from a wide string path
    static void Initialize(std::wstring& configFilePath);
    static void ProcessCommandLine();
    static void Serialize();
};

