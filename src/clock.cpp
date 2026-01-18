#include "clock.h"

void tickRateClock::calculateDeltaTime() {
    const chrono_impl::steady_clock::time_point currentTime = now();
    const chrono_impl::duration<float> deltaTime = chrono_impl::duration_cast<chrono_impl::duration<float>>(
        currentTime - lastTime);
    this->lastTime = currentTime;
    this->deltaTime = deltaTime.count();
}

void tickRateClock::calculateFrameSwapDeltaTime() {
    const chrono_impl::steady_clock::time_point currentTime = now();
    const chrono_impl::duration<float> frameSwapDeltaTime = chrono_impl::duration_cast<chrono_impl::duration<float>>(
        currentTime - lastFrameSwapTime);
    this->frameSwapDeltaTime = frameSwapDeltaTime.count();
}

void tickRateClock::initialize() {
    this->lastTime = now();
}

void tickRateClock::resetFrameSwapTime() {
    lastFrameSwapTime = now();
}

float tickRateClock::floatTime() const {
    const chrono_impl::steady_clock::time_point currentTime = now();
    const chrono_impl::duration<float> elapsedTime = chrono_impl::duration_cast<chrono_impl::duration<float>>(currentTime.time_since_epoch());
    return elapsedTime.count();
}

chrono_impl::steady_clock::time_point tickRateClock::now() {
    return chrono_impl::steady_clock::now();
}
