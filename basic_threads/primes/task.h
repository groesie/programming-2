#pragma once

#include <cstdint>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std::chrono_literals;

/*
 * Класс PrimeNumbersSet -- множество простых чисел в каком-то диапазоне
 */
class PrimeNumbersSet {
    std::set<uint64_t> primes_;
    mutable std::shared_mutex mutex_;
    std::chrono::duration<double, std::nano> time_waiting_for_mutex_ = 0ms;
    std::chrono::duration<double, std::nano> time_under_mutex_ = 0ms;

public:
    PrimeNumbersSet() = default;

    // Проверка, что данное число присутствует в множестве простых чисел
    bool IsPrime(uint64_t number) const {
        std::shared_lock lock{mutex_};
        return primes_.find(number) != primes_.end();
    }

    // Получить следующее по величине простое число из множества
    uint64_t GetNextPrime(uint64_t number) const {
        std::shared_lock lock{mutex_};

        auto it = primes_.upper_bound(number);
        if (it == primes_.end())
            throw std::invalid_argument("No next prime");
        return *it;
    }

    /*
     * Найти простые числа в диапазоне [from, to) и добавить в множество
     * Во время работы этой функции нужно вести учет времени, затраченного на ожидание лока мюьтекса,
     * а также времени, проведенного в секции кода под локом
     */
    void AddPrimesInRange(uint64_t L, uint64_t R) {
        std::vector<bool> isPrime(R - L + 1, true);

        uint64_t lim = std::sqrt(R);
        for (uint64_t i = 2; i <= lim; ++i)
            for (uint64_t j = std::max(i * i, (L + i - 1) / i * i); j <= R; j += i)
                isPrime[j - L] = false;

        if (L == 1)
            isPrime[0] = false;

        auto t1 = std::chrono::high_resolution_clock::now();

        std::unique_lock lock{mutex_};

        auto t2 = std::chrono::high_resolution_clock::now();
        auto t3 = std::chrono::high_resolution_clock::now();
        time_waiting_for_mutex_ = time_waiting_for_mutex_ + t2 - t1;

        for (uint64_t i = 2; i < R - L + 1; ++i)
            if (isPrime[i])
                primes_.insert(i + L);

        auto t4 = std::chrono::high_resolution_clock::now();
        time_under_mutex_ += t4 - t3;
        // mutex_.unlock();
    }

    // Посчитать количество простых чисел в диапазоне [from, to)
    size_t GetPrimesCountInRange(uint64_t from, uint64_t to) const {
        std::shared_lock lock{mutex_};
        return std::count_if(primes_.begin(), primes_.end(), [from, to](uint64_t x) { return x >= from && x <= to; });
    }

    // Получить наибольшее простое число из множества
    uint64_t GetMaxPrimeNumber() const {
        std::shared_lock lock{mutex_};
        return primes_.empty() ? 0 : *primes_.rbegin();
    }

    // Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeWaitingForMutex() const {
        std::shared_lock lock{mutex_};
        return std::chrono::duration_cast<std::chrono::nanoseconds>(time_waiting_for_mutex_);
    }

    // Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeUnderMutex() const;
private:
    std::set<uint64_t> primes_;
    mutable std::mutex set_mutex_;
    std::atomic<uint64_t> nanoseconds_under_mutex_, nanoseconds_waiting_mutex_;
};
