#pragma once

#include <cstdint>
#include <mutex>
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
    std::mutex mutex_;
    std::chrono::duration<double, std::nano> time_waiting_for_mutex_;
    std::chrono::duration<double, std::nano> time_under_mutex_;

public:
    PrimeNumbersSet() {
        time_waiting_for_mutex_ = 0ms;
        time_under_mutex_ = 0ms;
    }

    // Проверка, что данное число присутствует в множестве простых чисел
    bool IsPrime(uint64_t number) const {
        return primes_.find(number) != primes_.end();
    }

    // Получить следующее по величине простое число из множества
    uint64_t GetNextPrime(uint64_t number) const {
        auto it = primes_.upper_bound(number);
        if (it == primes_.end())
            return 0;
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
        mutex_.lock();

        auto t2 = std::chrono::high_resolution_clock::now();
        auto t3 = std::chrono::high_resolution_clock::now();
        time_waiting_for_mutex_ = time_waiting_for_mutex_ + t2 - t1;

        for (int i = 0; i < R - L + 1; ++i)
            if (isPrime[i])
                primes_.insert(i + L);

        auto t4 = std::chrono::high_resolution_clock::now();
        time_under_mutex_ += t4 - t3;

        mutex_.unlock();

        return;
    }

    // Посчитать количество простых чисел в диапазоне [from, to)
    size_t GetPrimesCountInRange(uint64_t from, uint64_t to) const {
        auto l = primes_.lower_bound(from), r = primes_.lower_bound(to);
        return std::distance(l, r);
    }

    // Получить наибольшее простое число из множества
    uint64_t GetMaxPrimeNumber() const {
        return primes_.empty() ? 0 : *primes_.rbegin();
    }

    // Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeWaitingForMutex() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(time_waiting_for_mutex_);
    }

    // Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeUnderMutex() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(time_under_mutex_);
    }
};
