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
