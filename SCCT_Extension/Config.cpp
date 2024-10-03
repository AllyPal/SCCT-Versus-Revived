#include "pch.h"
#include "Config.h"
#include <fstream>
#include <iostream>
#include "include/nlohmann/json.hpp"
#include <vector>

static std::wstring configFilePathRef;
int Config::frameRateLimit_client;
int Config::frameRateLimit_hosting;
int Config::frameTimingMode;
bool Config::applyAnimationFix;
bool Config::fixFlashlight;
bool Config::widescreenAspectRatioFix;
float Config::widescreenFovCap;
bool Config::forceMaxRefreshRate;
bool Config::labs_borderlessFullscreen;

bool Config::useDirectConnect;
std::wstring Config::directConnectIp;
std::wstring Config::directConnectPort;

std::string Config::masterServerDns;

bool Config::mouseInputFix;
float Config::menuSensitivity;
float Config::baseMouseSensitivity;
bool Config::security_acg;
bool Config::security_dep;
bool Config::disableStickyCamContextMenu;

float Config::lod;

std::vector<std::string> Config::serverList;

void Config::Initialize(std::wstring& configFilePath) {
    configFilePathRef = configFilePath;
    frameRateLimit_client = 60;
    frameRateLimit_hosting = 60;
    frameTimingMode = 1;
    applyAnimationFix = true;
    fixFlashlight = true;
    widescreenAspectRatioFix = true;
    widescreenFovCap = 105.0;
    forceMaxRefreshRate = true;
    labs_borderlessFullscreen = false;
    useDirectConnect = false;
    directConnectIp = L"";
    directConnectPort = L"";
    masterServerDns = "scct-reloaded.duckdns.org:11000";
    mouseInputFix = true;
    menuSensitivity = 0.25;
    baseMouseSensitivity = 1.0;
    security_acg = false;
    security_dep = true;
    disableStickyCamContextMenu = true;
    lod = 3.0;

    std::ifstream configFile(configFilePathRef);

    if (configFile.is_open()) {
        try {
            nlohmann::json jsonConfig;
            configFile >> jsonConfig;

            frameRateLimit_client = jsonConfig.value("frameRateLimit_client", 60);
#ifndef _DEBUG
            frameRateLimit_client = std::clamp(frameRateLimit_client, 30, 1000);
#endif
            frameRateLimit_hosting = jsonConfig.value("frameRateLimit_hosting", 60);
#ifndef _DEBUG
            frameRateLimit_hosting = std::clamp(frameRateLimit_hosting, 30, 90);
#endif
            frameTimingMode = jsonConfig.value("frameTimingMode", 1);
            applyAnimationFix = jsonConfig.value("applyAnimationFix", true);
            fixFlashlight = jsonConfig.value("fixFlashlight", true);
            widescreenAspectRatioFix = jsonConfig.value("widescreenAspectRatioFix", true);
            widescreenFovCap = jsonConfig.value("widescreenFovCap", 105.0);
            forceMaxRefreshRate = jsonConfig.value("forceMaxRefreshRate", true);
            labs_borderlessFullscreen = jsonConfig.value("labs_borderlessFullscreen", false);
            mouseInputFix = jsonConfig.value("mouseInputFix", true);
            menuSensitivity = jsonConfig.value("menuSensitivity", 0.25);
            baseMouseSensitivity = jsonConfig.value("baseMouseSensitivity", 1.0);
            serverList = jsonConfig.value("serverList", std::vector<std::string> {});
            security_acg = jsonConfig.value("security_acg", false);
            security_dep = jsonConfig.value("security_dep", true);
            disableStickyCamContextMenu = jsonConfig.value("disableStickyCamContextMenu", true);
            masterServerDns = jsonConfig.value("masterServerDns", "scct-reloaded.duckdns.org:11000");
            lod = jsonConfig.value("lod", 3.0);
            if (lod < 1.0) {
                lod = 1.0;
            }

            // Update the file with any new fields
            Serialize();
        }
        catch (const std::exception& e) {
            std::cerr << "Error reading config file: " << e.what() << std::endl;
        }
    }
    else {
        std::cerr << "Could not open config file. Using default values." << std::endl;
        Serialize(); // Write defaults to the file if it doesn't exist
    }
    ProcessCommandLine();
}

void Config::ProcessCommandLine()
{
    std::wstring commandLine = GetCommandLineW();
    std::wstring connectFlag = L"-join ";

    size_t pos = commandLine.find(connectFlag);

    if (pos != std::wstring::npos) {
        useDirectConnect = true;
        size_t start = pos + connectFlag.length();
        size_t colonPos = commandLine.find(L':', start);
        if (colonPos != std::wstring::npos) {
            directConnectIp = commandLine.substr(start, colonPos - start);
            directConnectPort = commandLine.substr(colonPos + 1);
        }
        else {
            directConnectIp = commandLine.substr(start);
        }

        if (directConnectIp.length() > 15 || directConnectIp.length() <= 7) {
            std::wcout << L"Invalid IP address format." << std::endl;
            useDirectConnect = false;
            directConnectIp.clear();
        }
    }

    if (useDirectConnect) {
        std::wcout << L"UseDirectConnect: true" << std::endl;
        std::wcout << L"IP: " << directConnectIp << std::endl;
        std::wcout << L"Port: " << directConnectPort << std::endl;
    }
    else {
        std::wcout << L"UseDirectConnect: false" << std::endl;
    }

    std::wcout << L"mouseInputFix: " << mouseInputFix << std::endl;
}

bool Config::Serialize() {
    std::ofstream configFile(configFilePathRef);

    if (configFile) {
        try {
            nlohmann::json jsonConfig;

            jsonConfig["frameRateLimit_client"] = frameRateLimit_client;
            jsonConfig["frameRateLimit_hosting"] = frameRateLimit_hosting;
            jsonConfig["frameTimingMode"] = frameTimingMode;
            jsonConfig["applyAnimationFix"] = applyAnimationFix;
            jsonConfig["fixFlashlight"] = fixFlashlight;
            jsonConfig["mouseInputFix"] = mouseInputFix;
            jsonConfig["menuSensitivity"] = menuSensitivity;
            jsonConfig["baseMouseSensitivity"] = baseMouseSensitivity;
            jsonConfig["widescreenAspectRatioFix"] = widescreenAspectRatioFix;
            jsonConfig["widescreenFovCap"] = widescreenFovCap;
            jsonConfig["forceMaxRefreshRate"] = forceMaxRefreshRate;
            jsonConfig["labs_borderlessFullscreen"] = labs_borderlessFullscreen;
            jsonConfig["serverList"] = serverList;
            jsonConfig["security_acg"] = security_acg;
            jsonConfig["security_dep"] = security_dep;
            jsonConfig["disableStickyCamContextMenu"] = disableStickyCamContextMenu;
            jsonConfig["masterServerDns"] = masterServerDns;
            jsonConfig["lod"] = lod;

            configFile << jsonConfig.dump(4);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error writing config file: " << e.what() << std::endl;
        }
    }
    else {
        std::cerr << "Could not open config file for writing. Check path and permissions." << std::endl;
    }
    return false;
}