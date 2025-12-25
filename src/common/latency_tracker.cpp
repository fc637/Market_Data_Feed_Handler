
#include <algorithm>
#include "protocol.h"

LatencyTracker& LatencyTracker::instance() {
    static LatencyTracker inst;
    return inst;
}

void LatencyTracker::record_kernel_to_user(uint64_t ns) {
    std::lock_guard<std::mutex> lock(mtx_);
    samples_.push_back(ns);
}

void LatencyTracker::record_userspace(uint64_t ns) {
    std::lock_guard<std::mutex> lock(mtx_);
    samples_.push_back(ns);
}

uint64_t LatencyTracker::p50() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (samples_.empty()) return 0;

    auto tmp = samples_;
    std::nth_element(tmp.begin(), tmp.begin() + tmp.size()/2, tmp.end());
    return tmp[tmp.size()/2];
}

uint64_t LatencyTracker::p99() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (samples_.empty()) return 0;

    auto tmp = samples_;
    size_t idx = tmp.size() * 99 / 100;
    std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
    return tmp[idx];
}
