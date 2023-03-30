#pragma once
#include <atomic>


class SimpleMutex {
   public:
    SimpleMutex() = default;

    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            __builtin_ia32_pause();
        }
    }

    void unlock() { flag.clear(std::memory_order_release); }

   private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

class SharedMutex {
public:
    void lock_shared() {
        mutex_.lock();
        while (uniq_lck_) {
            mutex_.unlock();
            __builtin_ia32_pause();
            mutex_.lock();
        }
        shared_cnt_ += 1;
        mutex_.unlock();
    }

    void unlock_shared() {
        mutex_.lock();
        shared_cnt_ -= 1;
        mutex_.unlock();
    }

    void lock() {
        mutex_.lock();
        while (shared_cnt_ != 0 || uniq_lck_) {
            mutex_.unlock();
            __builtin_ia32_pause();
            mutex_.lock();
        }
        uniq_lck_ = true;
        mutex_.unlock();
    }

    void unlock() {
        mutex_.lock();
        uniq_lck_ = false;
        mutex_.unlock();
    }

   private:
    int shared_cnt_ = 0;
    bool uniq_lck_ = false;
    SimpleMutex mutex_;
};
