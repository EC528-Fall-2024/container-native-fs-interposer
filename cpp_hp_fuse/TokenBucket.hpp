#ifndef TOKEN_BUCKET_HPP
#define TOKEN_BUCKET_HPP

#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <signal.h>
#include <sys/time.h>

class TokenBucket {
    private :
    const int64_t capacity;
    const double fillRate;
    std::atomic<double> tokens;
    std::chrono::steady_clock::time_point lastUpdate;
    mutable std::mutex mutex;
    std::condition_variable cv;

    void AddTokens() {
        double current = tokens.load(std::memory_order_relaxed);
        double new_value = std::min(static_cast<double>(capacity), 
                                  current + (fillRate * 0.1)); // 0.1 because timer is 100ms
        tokens.store(new_value, std::memory_order_release);
        cv.notify_all();
    }

    static void TimerHandler(int signum, siginfo_t* info, void* context) {

        (void) signum;
        (void) context;
        
        TokenBucket* bucket = static_cast<TokenBucket*>(info->si_value.sival_ptr);
        bucket->AddTokens();
    }

    void SetupTimer() {
        // Set up the timer signal handler
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = TimerHandler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGRTMIN, &sa, nullptr);

        // Set up the timer
        timer_t timerid;
        struct sigevent sev;
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGRTMIN;
        sev.sigev_value.sival_ptr = this;
        timer_create(CLOCK_MONOTONIC, &sev, &timerid);

        // Configure timer interval 
        struct itimerspec its;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 100000000; // 100ms
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 100000000;
        timer_settime(timerid, 0, &its, nullptr);
    }



    public :
    TokenBucket(int64_t capacity_, double fillRate_)
        : capacity(capacity_), fillRate(fillRate_), tokens(capacity_)
    {
        SetupTimer();
    }

    void consume(int64_t requested) {
        std::unique_lock<std::mutex> lock(mutex);

        while (true) {
            double currentTokens = tokens.load(std::memory_order_acquire);

            if(currentTokens >= requested) {
                tokens.store(currentTokens - requested, std::memory_order_release);
                return;
            }   

            cv.wait(lock);
        } 
    }


};

class IOThrottler {
private :
    TokenBucket readBucket;
    TokenBucket writeBucket;

public :
    IOThrottler(int64_t read_bps, int64_t write_bps)
        : readBucket(read_bps, read_bps)
        , writeBucket(write_bps, write_bps)
    {}

    template<typename Func, typename... Args>
    void throttledRead(size_t size, Func&& readFn, Args&&... args) {
        readBucket.consume(size);
        std::forward<Func>(readFn)(std::forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    void throttledWrite(size_t size, Func&& readFn, Args&&... args) {
        writeBucket.consume(size);
        std::forward<Func>(readFn)(std::forward<Args>(args)...);
    }
};


#endif // TOKEN_BUCKET_HPP
