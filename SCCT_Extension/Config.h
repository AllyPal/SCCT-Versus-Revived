#pragma once

#include <string>

class Config {
public:
    static int frameRateLimit;
    static bool applyAnimationFix;

    static bool useDirectConnect;
    static std::wstring directConnectIp;
    static std::wstring directConnectPort;

    // Constructor to initialize the config from a wide string path
    static void Initialize(std::wstring& configFilePath);
    static void ProcessCommandLine();
    static void Serialize();
};

