#pragma once
#include <Windows.h>
#include <chrono>

class Graphics
{
public:
	static void Initialize();
	// TODO: Move to engine
	static std::chrono::steady_clock::time_point lastFrameTime;
};

