#ifndef CLOCK_H
#define CLOCK_H

#ifdef __ANDROID__
#include <chrono>
namespace chrono_impl = std::chrono;
#else
#include <boost/chrono.hpp>
namespace chrono_impl = boost::chrono;
#endif

struct tickRateClock {
    chrono_impl::steady_clock::time_point lastTime{};
    chrono_impl::steady_clock::time_point lastFrameSwapTime{};
    float deltaTime{};
    float frameSwapDeltaTime{};

    void calculateDeltaTime();

    void calculateFrameSwapDeltaTime();

    void initialize();
    void resetFrameSwapTime();

    float floatTime() const;

    static chrono_impl::steady_clock::time_point now();
};

#endif //CLOCK_H
