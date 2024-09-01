#include "pch.h"
#include "Config.h"
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"

static std::wstring* configFilePathRef;
int Config::frameRateLimit;
bool Config::applyAnimationFix;

bool Config::useDirectConnect;
std::wstring Config::directConnectIp;
std::wstring Config::directConnectPort;
bool Config::mouseInputFix;

void Config::Initialize(std::wstring& configFilePath) {
    configFilePathRef = &configFilePath;
    frameRateLimit = 90;
    applyAnimationFix = true;
    useDirectConnect = false;
    directConnectIp = L"";
    directConnectPort = L"";
    mouseInputFix = true;

    std::ifstream configFile(*configFilePathRef);

    if (configFile.is_open()) {
        try {
            nlohmann::json jsonConfig;
            configFile >> jsonConfig;

            frameRateLimit = jsonConfig.value("frameRateLimit", 90);
            applyAnimationFix = jsonConfig.value("applyAnimationFix", true);
            mouseInputFix = jsonConfig.value("mouseInputFix", true);

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

            jsonConfig["frameRateLimit"] = frameRateLimit;
            jsonConfig["applyAnimationFix"] = applyAnimationFix;
            jsonConfig["mouseInputFix"] = mouseInputFix;

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