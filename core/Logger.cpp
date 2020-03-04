// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "Logger.h"

#include <chrono>
#include <iostream>
#include <string.h>

namespace aga
{
    void Logger::Log(LogLevel level, const std::string &message)
    {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        char timeBuffer[100] = {0};
        std::strftime(timeBuffer, sizeof(timeBuffer), "%T", std::localtime(&now));

        char severity[10];

        switch (level)
        {
            case LogLevel::Debug:
                strcpy(severity, "DEBUG");
                break;
            case LogLevel::Info:
                strcpy(severity, "INFO");
                break;
            case LogLevel::Warning:
                strcpy(severity, "WARNING");
                break;
            case LogLevel::Error:
                strcpy(severity, "ERROR");
                break;
        }

        std::cout << timeBuffer << " " << severity << ": " << message;
    }
}  // namespace aga