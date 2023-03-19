#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <atomic>

class TimeOut : public std::exception {
public:
    const char* what() const noexcept override {
        return "Timeout";
    }
};

template<typename T>
class UnbufferedChannel {
public:
    void Put(const T& data) {
        std::unique_lock<std::mutex> lock(mutex_);
        put_cv_.wait(lock, [this]() { return empty_.load()  && !writer_active_.load() ; });

        writer_active_ = true;
        tmutex_.lock();
        data_ = data;
        tmutex_.unlock();
        empty_ = false;
        
        
        get_cv_.notify_one();

        sync_cv_.wait(lock, [this]() { return empty_.load() ; });
        writer_active_ = false;
        put_cv_.notify_one();
    }

    T Get(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeout.count() > 0) {
            if (!get_cv_.wait_for(lock, timeout, [this]() { return !empty_.load() ; })) {
                throw TimeOut();
            }
        } else {
            get_cv_.wait(lock, [this]() { return !empty_.load() ; });
        }
        
        // tmutex_.lock();
        T result = data_;
        // tmutex_.unlock();
        empty_ = true;
        sync_cv_.notify_one();

        return result;
    }

private:
    std::mutex mutex_;
    std::condition_variable put_cv_;
    std::condition_variable get_cv_;
    std::condition_variable sync_cv_;
    std::recursive_mutex tmutex_;
    std::atomic<bool> empty_ = true;
    T data_;
    std::atomic<bool> writer_active_ = false;
};
