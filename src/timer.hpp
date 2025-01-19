#include <chrono>

class Timer
{
    std::chrono::steady_clock::time_point start_time;
    std::chrono::seconds duration;

public:
    Timer(std::chrono::seconds duration) : duration(duration)
    {
        start_time = std::chrono::high_resolution_clock::now();
    }

    bool has_elapsed()
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time) >= duration;
    }
};