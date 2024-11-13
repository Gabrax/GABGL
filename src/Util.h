#pragma once
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

namespace Util{

    inline void measureExecutionTime(const std::atomic<bool>& initializationComplete, const std::chrono::high_resolution_clock::time_point& start) {
        while (!initializationComplete) {
            auto current = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current - start;
            std::cout << "\rElapsed time: " << elapsed.count() << " seconds" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Update every 100 ms
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "\rTotal time taken: " << elapsed.count() << " seconds" << '\n';
    }

    template <typename Func>
    inline void displayTimer(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        std::atomic<bool> initializationComplete = false;

        // Start the timer display in a separate thread
        std::thread timerThread(measureExecutionTime, std::ref(initializationComplete), start);

        // Execute the provided function or lambda
        func();

        initializationComplete = true;

        // Wait for the timer thread to finish
        timerThread.join();
    }

    
}
