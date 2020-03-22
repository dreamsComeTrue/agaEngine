// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "Logger.h"
#include "Common.h"

#include <chrono>

namespace aga
{
    void Logger::Log(LogLevel level, const String &message)
    {
        if (level < m_Level)
        {
            return;
        }

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        char time_buffer[100] = {0};
        std::strftime(time_buffer, sizeof(time_buffer), "%T", std::localtime(&now));

        char severity[10];

        switch (level)
        {
            case LogLevel::Debug:
                StrCopy(severity, "DEBUG");
                break;
            case LogLevel::Info:
                StrCopy(severity, "INFO");
                break;
            case LogLevel::Warning:
                StrCopy(severity, "WARNING");
                break;
            case LogLevel::Error:
                StrCopy(severity, "ERROR");
                break;
        }

        std::cout << time_buffer << " " << severity << ": " << message;
    }

    void Logger::EnableLogLevel(LogLevel level)
    {
        m_Level = level;
    }
}  // namespace aga