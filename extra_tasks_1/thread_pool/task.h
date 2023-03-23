#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <chrono>

/*
 * Требуется написать класс ThreadPool, реализующий пул потоков, которые выполняют задачи из общей очереди.
 * С помощью метода PushTask можно положить новую задачу в очередь
 * С помощью метода Terminate можно завершить работу пула потоков.
 * Если в метод Terminate передать флаг wait = true,
 *  то пул подождет, пока потоки разберут все оставшиеся задачи в очереди, и только после этого завершит работу потоков.
 * Если передать wait = false, то все невыполненные на момент вызова Terminate задачи, которые остались в очереди,
 *  никогда не будут выполнены.
 * После вызова Terminate в поток нельзя добавить новые задачи.
 * Метод IsActive позволяет узнать, работает ли пул потоков. Т.е. можно ли подать ему на выполнение новые задачи.
 * Метод GetQueueSize позволяет узнать, сколько задач на данный момент ожидают своей очереди на выполнение.
 * При создании нового объекта ThreadPool в аргументах конструктора указывается количество потоков в пуле. Эти потоки
 *  сразу создаются конструктором.
 * Задачей может являться любой callable-объект, обернутый в std::function<void()>.
 */

class ThreadPool {
public:
    ThreadPool(size_t threadCount) {
        for (size_t i = 0; i < threadCount; ++i) {
            threads_.emplace_back([this](){
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        while (isActive_ && queueSize_ == 0) {
                            cv_.wait(lock, [this](){
                                return !isActive_.load() || queueSize_.load() != 0;
                            });
                        }
                        if (!isActive_ && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                        --queueSize_;
                    }
                    // std::unique_lock<std::mutex> lock(mutex_);
                    task();
                }
            });
        }
    }
    // ~ThreadPool() {
    //     if (isActive_)
    //         Terminate(true);
    // }
    void PushTask(const std::function<void()>& task) {
        if (!isActive_.load()) {
            throw std::runtime_error("Thread pool is terminated");
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.push(task);
            ++queueSize_;
        }
        cv_.notify_one();
    }

    void Terminate(bool wait) {
        std::unique_lock<std::mutex> tlock(tmutex_);
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!isActive_) {
                return;
            }
            isActive_ = false;
        }
        cv_.notify_all();
        if (wait) {
            // std::unique_lock<std::mutex> lock(mutex_);
            for (auto& thread : threads_) {
                thread.join();
            }
        } else {
            std::unique_lock<std::mutex> lock(mutex_);
            while (queueSize_ != 0) {
                tasks_.pop();
                --queueSize_;
            }
            for (auto& thread : threads_) {
                if (thread.joinable())
                    thread.detach();
            }
            // cv_.notify_all();
        }
    }

    bool IsActive() const {
        return isActive_.load();
    }

    size_t QueueSize() const {
        return queueSize_.load();
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::mutex tmutex_;
    std::condition_variable cv_;
    std::atomic<bool> isActive_ = true;
    std::atomic<size_t> queueSize_ = 0;
};
