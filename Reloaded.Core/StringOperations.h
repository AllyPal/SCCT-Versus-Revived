#pragma once
#include "pch.h"
#include <format>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include <timeapi.h>
#include <Windows.h>
#include <codecvt>

class StringOperations
{
public:
    static std::string wStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);

        return str;
    }

    static std::wstring stringToWString(const std::string& str) {
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring wstr(size_needed, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], size_needed);

        return wstr;
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
