#include "tinyLogger.h"

void TinyLogger::startLoggingThread()
{
    m_isRunning = true;
    m_thread    = std::make_unique<std::jthread>([this]() {
        while (m_isRunning)
        {
            auto log = m_logQueue.pop();
            if (log)
            {
                std::cout << log.value() << std::endl; // Output to consoles
                std::cout.flush();                     // Ensure immediate output to console

                if (m_file.is_open())
                {
                    m_file << log.value() << std::endl;
                    m_file.flush(); // Ensure immediate write
                }
            }
            else
            {
                // Small delay to prevent busy waiting when queue is empty
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });
}