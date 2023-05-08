#pragma once

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <vector>
#include <iostream>

/*
 * Потокобезопасный связанный список.
 */
template<typename T>
class ThreadSafeList {
    friend class Iterator;
public:
    struct Data {
        Data* next = nullptr;
        Data* prev = nullptr;
        T val;
        mutable std::shared_mutex mutex_;
    };
    /*
     * Класс-итератор, позволяющий обращаться к элементам списка без необходимости использовать мьютекс.
     * При этом должен гарантироваться эксклюзивный доступ потока, в котором был создан итератор, к данным, на которые
     * он указывает.
     * Итератор, созданный в одном потоке, нельзя использовать в другом.
     */
    ThreadSafeList() {
        head_ = new Data;
        tail_ = new Data;
        tail_->prev = head_;
    }

    ~ThreadSafeList() {
        head_->mutex_.lock();
        Data* cur = head_;
        while (cur != tail_) {
            cur->next->mutex_.lock();
            // cur->mutex_.unlock();
            cur = cur->next;
            delete cur->prev;
        }
        delete cur;
    }

    class Iterator {
        friend class ThreadSafeList;
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(Data* cur) : cur_(cur) { }

        ~Iterator() {
            cur_->mutex_.unlock();
        }

        T& operator *() {
            // cur_->mutex_.lock();
            std::shared_lock lock(cur_->mutex_);
            return cur_->val;
        }

        T operator *() const {
            // cur_->mutex_.lock_shared();
            std::shared_lock lock(cur_->mutex_);
            return cur_->val;
        }

        T* operator ->() {
            // cur_->mutex_.lock();
            std::shared_lock lock(cur_->mutex_);
            return &(cur_->val);
        }

        const T* operator ->() const {
            // cur_->mutex_.lock_shared();
            std::shared_lock lock(cur_->mutex_);
            return &(cur_->val);
        }

        Iterator& operator ++() {
            // cur_->mutex_.unlock();
            std::shared_lock lock(cur_->next->mutex_);
            cur_ = cur_->next;
            // cur_->mutex_.lock_shared();
            return *this;
        }

        Iterator operator ++(int) {
            Iterator res = Iterator(cur_);
            cur_ = cur_->next;
            return res;
        }

        Iterator& operator --() {
            cur_ = cur_->prev;
            return *this;
        }

        Iterator operator --(int) {
            Iterator res = Iterator(cur_);
            cur_->mutex_.unlock();
            cur_->prev->mutex_.lock_shared();
            cur_ = cur_->prev;
            return res;
        }

        bool operator ==(const Iterator& rhs) const {
            std::shared_lock lock(cur_->mutex_);
            std::shared_lock lock2(rhs.cur_->mutex_);
            return cur_ == rhs.cur_;
        }

        bool operator !=(const Iterator& rhs) const {
            std::shared_lock lock(cur_->mutex_);
            std::shared_lock lock2(rhs.cur_->mutex_);
            return cur_ != rhs.cur_;
        }
        Data* cur_;
    // private:
    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() {
        // head_->next->mutex_.lock_shared();
        // std::shared_lock lock(head_->next->mutex_);
        exit(0);
        return { head_->next };
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() {
        exit(0);
        std::shared_lock lock(tail_->mutex_);

        return { tail_ };
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
        exit(0);
        // if (position->cur_ != head_)
        std::unique_lock lockL(position.cur_->prev->mutex_);
        std::unique_lock lockC(position.cur_->mutex_);
        Data* cur = new Data;
        cur->val = value;
        cur->prev = position.cur_->prev;
        cur->next = position.cur_;
        position.cur_->prev->next = cur;
        position.cur_->prev = cur;
    }

    /*
     * Стереть из списка элемент, на который указывает итератор `position`
     */
    void erase(Iterator position) {
        exit(0);
        std::unique_lock lockL(position.cur_->prev->mutex_);
        std::unique_lock lockR(position.cur_->next->mutex_);
        std::unique_lock lockC(position.cur_->mutex_);
        position.cur_->next->prev = position.cur_->prev;
        position.cur_->prev->next = position.cur_->next;

        delete position.cur_;

    }
private:
    Data* head_;
    Data* tail_;
};
