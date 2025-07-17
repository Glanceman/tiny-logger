#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "logger/tinyLogger.h"
#include <signal.h>
#include "logger/hazardPointerGuard.h"

volatile bool running = true;

void SignalHandler(int signum)
{
    std::cout << "Caught signal " << signum << ", exiting gracefully." << std::endl;
    running = false;
}

const int NUM_THREADS = 10;

void test_multi_thread_logger(TinyLogger& logger)
{

    // Consumers: log periodically
    std::vector<std::jthread> consumers;
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


struct PoorlyAlignedData
{
    char     c;
    uint16_t u;
    double   d;
    int16_t  i;
};

struct WellAlignedData
{
    double   d;
    uint16_t u;
    int16_t  i;
    char     c;
};

#pragma pack(push, 1)
struct PackedData
{
    double   d;
    uint16_t u;
    int16_t  i;
    char     c;
};
#pragma pack(pop)

struct MyStruct
{
    int8_t a;
    int8_t b;
    int16_t c;
};


void test_multi_thread_hazard_pointer()
{
    // Consumers: log periodically
    std::vector<std::jthread> consumers;
    for (int i = 0; i < 3; ++i)
    {
        // Capture logger by reference (safe as long as logger outlives threads)
        consumers.emplace_back([]() {
            while (running)
            {
                try{
                    HazardPointerGuard guard;
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                    std::ostringstream ss;
                    ss << std::this_thread::get_id();
                    std::string idstr = ss.str();
                    // Log the current thread's ID
                    std::cout << "Consumer " << idstr << " is working" << std::endl;
                }catch (const std::exception& e)
                {
                    std::cerr << "Exception in thread: " << e.what() << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                }
            }
        });
    }

}

int main()
{
    printf("PoorlyAlignedData c:%lu u:%lu d:%lu i:%lu size:%lu\n",
           offsetof(struct PoorlyAlignedData, c),
           offsetof(struct PoorlyAlignedData, u),
           offsetof(struct PoorlyAlignedData, d),
           offsetof(struct PoorlyAlignedData, i),
           sizeof(PoorlyAlignedData));
    printf("WellAlignedData d:%lu u:%lu i:%lu c:%lu size:%lu\n",
           offsetof(struct WellAlignedData, d),
           offsetof(struct WellAlignedData, u),
           offsetof(struct WellAlignedData, i),
           offsetof(struct WellAlignedData, c),
           sizeof(WellAlignedData));
    printf("PackedData d:%lu u:%lu i:%lu c:%lu size:%lu\n",
           offsetof(struct PackedData, d),
           offsetof(struct PackedData, u),
           offsetof(struct PackedData, i),
           offsetof(struct PackedData, c),
           sizeof(PackedData));


    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Test Hazard Pointer Guard
    test_multi_thread_hazard_pointer();
    // TinyLogger logger("TestLogger");
    // test_multi_thread_logger(logger);
    return 0;
}