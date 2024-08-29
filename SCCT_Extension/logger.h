#pragma once

#include "pch.h"
#include "logger.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <Windows.h>
#include <thread>
#include <chrono>

class Logger
{
private:
	std::wstring logFilePath;
public:
	void log(const std::string& message);
	void log(const std::wstring& message);
	Logger(const std::wstring& dllPath);
};
