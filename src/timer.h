#pragma once

#include <chrono>

template <typename ct = std::chrono::steady_clock>
class Timer
{
private:
    typename ct::time_point tp;

public:
    Timer()
        : tp{ct::now()}
    {}

    void reset() {
        tp = ct::now();
    }

    template <typename T = std::chrono::nanoseconds>
    auto elapsed() const
    {
        auto duration = std::chrono::duration_cast<T>(ct::now() - tp);
        return duration.count();
    }

    template <typename T = std::chrono::nanoseconds>
    auto elapsedReset()
    {
        auto ret = elapsed<T>();
        reset();
        return ret;
    }
};