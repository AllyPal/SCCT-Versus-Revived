#pragma once

#include <string>
#include <vector>

class Config {
public:
    static int frameRateLimit_client;
    static int frameRateLimit_hosting;
    static int frameTimingMode;
    static bool applyAnimationFix;
    static bool fixFlashlight;
    static bool widescreenAspectRatioFix;
    static float widescreenFovCap;
    static bool forceMaxRefreshRate;
    static bool labs_borderlessFullscreen;
    static std::vector<std::string> serverList;
    static bool security_acg;
    static bool security_dep;

    static bool disableStickyCamContextMenu;

    static bool useDirectConnect;
    static std::wstring directConnectIp;
    static std::wstring directConnectPort;

    static std::string masterServerDns;

    static bool mouseInputFix;
    static float baseMouseSensitivity;
    static float menuSensitivity;

    static float lod;

    // Constructor to initialize the config from a wide string path
    static void Initialize(std::wstring& configFilePath);
    static void ProcessCommandLine();
    static bool Serialize();
};

