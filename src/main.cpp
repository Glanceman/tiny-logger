#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "tinyLogger.h"
#include "lockFreeQueue.h" // Potentially used in future
#include <signal.h>

volatile bool running = true;

void signal_handler(int signum)
{
    std::cout << "Caught signal " << signum << ", exiting gracefully." << std::endl;
    running = false;
}

const int NUM_THREADS = 10;

void test_multi_thread(TinyLogger& logger)
{
    // Register signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Consumers: log periodically
    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        // Capture logger by reference (safe as long as logger outlives threads)
        consumers.emplace_back([&logger]() {
            int count = 0;
            while (running)
            {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::ostringstream ss;
                ss << std::this_thread::get_id();
                std::string idstr = ss.str();
                // Log the current thread's ID and count
                logger.log(TinyLogger::LogLevel::INFO, "Consumer {0} is working, counter {1}", idstr, count);
                ++count;
            }
        });
    }

    // Wait for all threads to finish
    for (auto& thread : consumers)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

int main()
{
    TinyLogger logger("TestLogger");
    test_multi_thread(logger);
    return 0;
}