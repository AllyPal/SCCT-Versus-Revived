#include "pch.h"
#include "Config.h"
#include <fstream>
#include <iostream>
#include "include/nlohmann/json.hpp"
#include <vector>

static std::wstring* configFilePathRef;
int Config::frameRateLimit_client;
bool Config::frameRateLimit_client_unlock;
int Config::frameRateLimit_hosting;
int Config::frameTimingMode;
bool Config::applyAnimationFix;
bool Config::applyWidescreenFix;

bool Config::useDirectConnect;
std::wstring Config::directConnectIp;
std::wstring Config::directConnectPort;
bool Config::mouseInputFix;

std::vector<std::string> Config::serverList;

void Config::Initialize(std::wstring& configFilePath) {
    configFilePathRef = &configFilePath;
    frameRateLimit_client = 60;
    frameRateLimit_client_unlock = false;
    frameRateLimit_hosting = 60;
    frameTimingMode = 1;
    applyAnimationFix = true;
    applyWidescreenFix = true;
    useDirectConnect = false;
    directConnectIp = L"";
    directConnectPort = L"";
    mouseInputFix = true;

    std::ifstream configFile(*configFilePathRef);

    if (configFile.is_open()) {
        try {
            nlohmann::json jsonConfig;
            configFile >> jsonConfig;

            frameRateLimit_client = jsonConfig.value("frameRateLimit_client", 60);
            frameRateLimit_client_unlock = jsonConfig.value("frameRateLimit_client_unlock", false);
            frameRateLimit_hosting = jsonConfig.value("frameRateLimit_hosting", 60);
            frameTimingMode = jsonConfig.value("frameTimingMode", 1);
            applyAnimationFix = jsonConfig.value("applyAnimationFix", true);
            applyWidescreenFix = jsonConfig.value("applyWidescreenFix", true);
            mouseInputFix = jsonConfig.value("mouseInputFix", true);
            serverList = jsonConfig.value("serverList", std::vector<std::string> {});

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

void Config::Serialize() {
    std::ofstream configFile(*configFilePathRef);

    if (configFile) {
        try {
            nlohmann::json jsonConfig;

            jsonConfig["frameRateLimit_client"] = frameRateLimit_client;

            jsonConfig["frameRateLimit_client_unlock"] = frameRateLimit_client_unlock;

            jsonConfig["frameRateLimit_hosting"] = frameRateLimit_hosting;

            jsonConfig["frameTimingMode"] = frameTimingMode;

            jsonConfig["applyAnimationFix"] = applyAnimationFix;

            jsonConfig["mouseInputFix"] = mouseInputFix;

            jsonConfig["applyWidescreenFix"] = applyWidescreenFix;

            jsonConfig["serverList"] = serverList;

            configFile << jsonConfig.dump(4);
        }
        catch (const std::exception& e) {
            std::cerr << "Error writing config file: " << e.what() << std::endl;
        }
    }
    else {
        std::cerr << "Could not open config file for writing. Check path and permissions." << std::endl;
    }
}