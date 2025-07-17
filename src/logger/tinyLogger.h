#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include "lockFreeQueue.h"
#include <fstream>
#include <format>
#include <string_view>
#include <chrono>
#include <thread>
#include <memory>
#include <filesystem>

class TinyLogger final
{
public:
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERR
    };

    explicit TinyLogger(std::string name, std::string path = "./logs/") :
        m_name(std::move(name)), m_logPath(std::move(path))
    {
        // Ensure the log directory exists
        std::filesystem::create_directories(m_logPath);
        std::cout << "Log directory created: " << m_logPath << std::endl;

        m_file.open(m_logPath + m_name + ".log", std::ios::out | std::ios::app);
        if (!m_file.is_open())
        {
            std::cerr << "Failed to open log file: " << m_logPath + m_name + ".log" << std::endl;
            return;
        }
        startLoggingThread();
        log(LogLevel::INFO, "Logger started");
    }

    ~TinyLogger()
    {
        m_isRunning = false;
        if (m_thread && m_thread->joinable())
        {
            m_thread->join();
        }

        // Flush remaining logs
        while (m_logQueue.length() > 0)
        {
            auto log = m_logQueue.pop();
            if (log)
            {
                if (m_file.is_open())
                {
                    m_file << log.value() << std::endl;
                }
                else
                {
                    std::cout << log.value() << std::endl;
                }
            }
        }

        if (m_file.is_open())
        {
            m_file.close();
        }
    }

    template <typename... Args>
    void log(LogLevel level, std::string_view message, Args&&... args)
    {
        std::string level_str = to_string(level);
        auto        now       = std::chrono::system_clock::now();
        // Format the time point into a string
        // Example format: YYYY-MM-DD HH:MM:SS
        std::string formatted_time = std::format("{:%Y-%m-%d %H:%M:%S}", now);

        std::string content;
        if constexpr (sizeof...(args) > 0)
        {
            content = std::vformat(std::string(message), std::make_format_args(args...));
        }
        else
        {
            content = std::string(message);
        }

        std::string msg = "[" + formatted_time + "] [" + level_str + "]: " + content;
        m_logQueue.push(msg);
    }

private:
    void startLoggingThread()
    {
        m_isRunning = true;
        m_thread    = std::make_unique<std::jthread>([this]() {
            while (m_isRunning)
            {
                auto log = m_logQueue.pop();
                if (log)
                {
                    if (!m_file.is_open())
                    {
                        m_file.open(m_logPath + m_name + ".log", std::ios::app);
                    }
                    m_file << log.value() << std::endl;
                    m_file.flush(); // Ensure immediate write
                }
                else
                {
                    // Small delay to prevent busy waiting when queue is empty
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    std::string to_string(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

private:
    LockFreeQueue<std::string>   m_logQueue;
    std::string                  m_name;
    std::string                  m_logPath;
    std::ofstream                m_file;
    volatile bool                m_isRunning{false};
    std::unique_ptr<std::jthread> m_thread;
};