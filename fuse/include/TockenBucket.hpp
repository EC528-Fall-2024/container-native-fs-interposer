#ifndef TOKEN_BUCKET_HPP
#define TOKEN_BUCKET_HPP

#include <atomic>
#include <cstdint>
#include <vector>
#include <memory>
#include <signal.h>
#include <time.h>
#include <err.h>
#include <string>
#include <memory>
#include <mutex>
#include <iostream>
#include <cstring>

const size_t tokenTroughput = 1 << 5;
const uint64_t initialtoken = 1 << 8;

class TokenBucket {
public:
    const size_t size;

private:
    std::atomic<uint64_t> token;
    uint64_t needs_tokens;

public:
    TokenBucket(size_t size, uint64_t initial)
        : size(size), token(initial), needs_tokens(0) {
        needs_tokens = (size / tokenTroughput) + (size % tokenTroughput == 0 ? 0 : 1);
    }

    bool enough_tokens() const {
        return token >= needs_tokens;
    }

    void add_tokens(uint64_t count = 1) {
        token += count;
    }

    // void consume_tokens(uint64_t count = 1) {
    //     token -= count;
    // }

    uint64_t get_token_count() const {
        return token;
    }

    uint64_t get_needs_tokens() const {
        return needs_tokens;
    }
};

std::vector<std::shared_ptr<TokenBucket>> active_buckets;
std::mutex active_buckets_mutex;

void signal_handler(int signum) {
    (void)signum;
    std::lock_guard<std::mutex> lock(active_buckets_mutex);
    for (auto& bucket : active_buckets) {
        bucket->add_tokens(1 << 8);
    }
}

bool setup_timer() {
    struct sigaction sa;
    struct itimerspec its;
    struct sigevent sev;
    timer_t timerid;
    
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGRTMIN, &sa, nullptr) == -1) {
        std::cerr << "Error: sigaction failed " << strerror(errno) << std::endl;
        return false;
    }

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1 ) {
        std::cerr << "Error: timer_create failed: " << strerror(errno) << std::endl;
        return false;
    }

    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, nullptr) == -1) {
        std::cerr << "Error: timer_settime failed: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

#endif // TOKEN_BUCKET_HPP
