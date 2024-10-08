#pragma once
#include "pch.h"
#include "Config.h"
#include "include/nlohmann/json.hpp"
#include "logger.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>
#include "GameStructs.h"

class CodeCaves
{
private:
	
public:
	static void Initialize();
	static bool IsListenServer();
	static void EnableProcessSecurity();
	static void OncePerFrame();
	static LvIn* lvIn;
};
