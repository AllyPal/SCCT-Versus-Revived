#pragma once
#include "pch.h"
#include <format>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include <timeapi.h>
#include <Windows.h>

class StringOperations
{
public:
    static std::string WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);

        return str;
    }

    static std::string toHexString(uintptr_t address) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << address;
        return ss.str();
    }

    static std::wstring toHexStringW(uintptr_t address) {
        std::wstringstream ss;
        ss << "0x" << std::hex << std::uppercase << address;
        return ss.str();
    }

    static std::string toString(uintptr_t address) {
        std::stringstream ss;
        ss << address;
        return ss.str();
    }
};

