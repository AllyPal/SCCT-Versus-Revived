#pragma once
#include <string>
#include <map>

class Debug
{
public:
    static void Initialize();
    static void CommandHandlers(std::map<std::wstring, CommandHandler>* commandHandlers);
};
