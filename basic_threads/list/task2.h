#pragma once

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <vector>
#include <iostream>

/*
 * Thread-safe linked list.
 */
template<typename T>
class ThreadSafeList {
public:
    /*
     * Iterator class that allows for accessing elements of the list without needing to use the mutex.
     * It should guarantee exclusive access of the thread that created the iterator to the data it points to.
     * An iterator created in one thread cannot be used in another thread.
     */
    class Iterator {
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(ThreadSafeList<T>* list, typename std::list<T>::iterator it)
            : m_list(list), m_it(it)
        {
            m_list->m_readLock.lock_shared();
        }

        Iterator(const Iterator& other)
            : m_list(other.m_list), m_it(other.m_it)
        {
            m_list->m_readLock.lock_shared();
        }

        ~Iterator()
        {
            m_list->m_readLock.unlock_shared();
        }

        T& operator *() {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            return *m_it;
        }

        T operator *() const {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            return *m_it;
        }

        T* operator ->() {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            return &(*m_it);
        }

        const T* operator ->() const {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            return &(*m_it);
        }

        Iterator& operator ++() {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            ++m_it;
            return *this;
        }

        Iterator operator ++(int) {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            Iterator temp(*this);
            ++m_it;
            return temp;
        }

        Iterator& operator --() {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            --m_it;
            return *this;
        }

        Iterator operator --(int) {
            std::lock_guard<std::mutex> lock(m_list->m_mutex);
            Iterator temp(*this);
            --m_it;
            return temp;
        }

        bool operator ==(const Iterator& rhs) const {
            return m_it == rhs.m_it;
        }

        bool operator !=(const Iterator& rhs) const {
            return m_it != rhs.m_it;
        }

    private:
        ThreadSafeList<T>* m_list;
        typename std::list<T>::iterator m_it;
    };

    ThreadSafeList() {}

    /*
     * Get an iterator pointing to the first element in the list.
     */
    Iterator begin() {
        return Iterator(this, m_list.begin());
    }

    /*
     * Get an iterator pointing to the element after the last element in the list.
     */
    Iterator end() {
        return Iterator(this, m_list.end());
    }

    /*
     * Insert a new element into the list before the element pointed to by the iterator `position`.
     */
    void insert(Iterator position, const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.insert
    }