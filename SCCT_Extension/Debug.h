#pragma once
class Debug
{
public:
    static void Initialize();
    static void CommandHandlers(std::map<std::wstring, CommandHandler>* commandHandlers);
};
