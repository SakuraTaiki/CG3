#include "FpsLimiter.h"

#include <thread>

void FpsLimiter::Initialize() {
    reference_ = std::chrono::steady_clock::now();
}

void FpsLimiter::Update() {
    using namespace std::chrono;

    const microseconds kMinTime(
        static_cast<int64_t>(1000000.0f / 60.0f)
    );

    const microseconds kMinCheckTime(
        static_cast<int64_t>(1000000.0f / 65.0f)
    );

    steady_clock::time_point now = steady_clock::now();
    microseconds elapsed = duration_cast<microseconds>(now - reference_);

    // かなり早く処理が終わったときだけ待機する。
    if (elapsed < kMinCheckTime) {
        while (steady_clock::now() - reference_ < kMinTime) {
            std::this_thread::sleep_for(microseconds(1));
        }
    }

    reference_ = steady_clock::now();
}