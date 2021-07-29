#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <vector>
#include <utility>
#include <ranges>

#include "utils.h"

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


/**
 * @brief Hepler class for simple profiling. (in nanoseconds)
 * Register start of new frame with newFrame(), and then run profile() after all parts that should be profiled in frame.
 */
class Profiler {
private:
    Timer<std::chrono::high_resolution_clock> timer;
    std::vector<unsigned long long> profiles;
    std::size_t profilePtr{0}, frameCount{0};

    Profiler() = default;
    Profiler(const Profiler&) = delete;

public:
    void newFrame() {
        profilePtr = 0;
        timer.reset();
        ++frameCount;
    }

    void profile() {
        if (profiles.size() <= profilePtr)
            profiles.emplace_back(timer.elapsedReset<std::chrono::microseconds>());
        else
            profiles.at(profilePtr) += timer.elapsedReset<std::chrono::microseconds>();
        ++profilePtr;
    }

    auto getAvgTimes() const {
        return util::collect(
            profiles | std::views::transform([=](const auto& p){
                return static_cast<double>(p) / frameCount;
            })
        );
    }

    auto getAvgTimesReset() {
        const auto times = getAvgTimes();
        frameCount = 0;
        profiles.clear();
        return times;
    }

    static Profiler& get() {
        static Profiler instance{};
        return instance;
    }
};

#endif // TIMER_H