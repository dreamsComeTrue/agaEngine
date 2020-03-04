// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include <string>

#define LOG_DEBUG(message) aga::Logger::getInstance().Log(aga::Logger::LogLevel::Debug, message);
#define LOG_DEBUG_F(message)                                                                       \
    aga::Logger::getInstance().Log(aga::Logger::LogLevel::Debug,                                   \
                                   std::string(__FUNCTION__) + ": " + message);

#define LOG_INFO(message) aga::Logger::getInstance().Log(aga::Logger::LogLevel::Info, message);
#define LOG_INFO_F(message)                                                                        \
    aga::Logger::getInstance().Log(aga::Logger::LogLevel::Info,                                    \
                                   std::string(__FUNCTION__) + ": " + message);

#define LOG_WARNING(message)                                                                       \
    aga::Logger::getInstance().Log(aga::Logger::LogLevel::Warning, message);
#define LOG_WARNING_F(message)                                                                     \
    aga::Logger::getInstance().Log(aga::Logger::LogLevel::Warning,                                 \
                                   std::string(__FUNCTION__) + ": " + message);

#define LOG_ERROR(message) aga::Logger::getInstance().Log(aga::Logger::LogLevel::Error, message);
#define LOG_ERROR_F(message)                                                                       \
    aga::Logger::getInstance().Log(aga::Logger::LogLevel::Error,                                   \
                                   std::string(__FUNCTION__) + ": " + message);

namespace aga
{
    class Logger
    {
    public:
        enum LogLevel
        {
            Debug,
            Info,
            Warning,
            Error
        };

    public:
        static Logger &getInstance()
        {
            static Logger instance;
            return instance;
        }

    private:
        Logger()
        {
        }

    public:
        Logger(Logger const &) = delete;
        void operator=(Logger const &) = delete;

        void Log(LogLevel level, const std::string &message);
    };
}  // namespace aga
