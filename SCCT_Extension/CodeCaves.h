#pragma once
#include "pch.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "include/nlohmann/json.hpp"
#include "logger.h"
#include "Config.h"

class CodeCaves
{
private:
	
public:
	CodeCaves(Logger* logger2);
	void Initialize();
};


enum NetMode {
    NotMultiplayer,
    DedicatedServer,
    ListenServer,
    Client
};
struct LvIn {
    std::byte unspecified[0x1000];

    NetMode& netMode() {
        return *reinterpret_cast<NetMode*>(unspecified + (0x4A8));
    }
};
