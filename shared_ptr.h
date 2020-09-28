#pragma once
#include "control_blocks.h"
#include "weak_ptr.h"

template <typename T>
struct shared_ptr {

private:

    friend class weak_ptr<T>;

    template<typename Y>
    friend struct shared_ptr;

    control_block *cb;
    T* ptr;

public:
    // пустой shared_pointer :: cb == nullptr
    // нулевой shared_pointer :: ptr == nullptr

    // пустой и нулевой объект
    shared_ptr()
            : cb(nullptr), ptr(nullptr) {}

    // тоже пустой нулевой
    shared_ptr(std::nullptr_t)
            : shared_ptr() {}

    template<typename D>
    shared_ptr(std::nullptr_t, D)
            : shared_ptr() {}

    template<typename U, typename D = std::default_delete<U>>
    shared_ptr(U* new_ptr, D deleter = std::default_delete<U>()) try
            : cb(new control_block_ptr<U, D>(new_ptr, std::move(deleter))), ptr(new_ptr) {
        cb->add_ref();
    } catch (...) {
        deleter(new_ptr);
        throw ;
    }

    // aliasing constructor
    template<typename U>
    shared_ptr(const shared_ptr<U> &sp, T* ptr) noexcept
            : ptr(ptr), cb(sp.cb) {
        if (cb)
            cb->add_ref();
    }

    shared_ptr(const shared_ptr &sp) noexcept
            : ptr(sp.ptr), cb(sp.cb) {
        if (cb)
            cb->add_ref();
    }

    template<typename U>
    shared_ptr(const shared_ptr<U> &sp) noexcept
            : ptr(sp.ptr), cb(sp.cb) {
        if (cb)
            cb->add_ref();
    }

    shared_ptr(shared_ptr &&sp) noexcept
            : ptr(sp.ptr), cb(sp.cb) {
        sp.ptr = nullptr;
        sp.cb  = nullptr;
    }

    template<typename U>
    shared_ptr(shared_ptr<U> &&sp) noexcept
            : ptr(sp.ptr), cb(sp.cb) {
        sp.ptr = nullptr;
        sp.cb  = nullptr;
    }

    template<typename U>
    explicit shared_ptr(const weak_ptr<U> &wp) {
        ptr = wp.ptr;
        cb  = wp.cb;
        if (cb)
            cb->add_ref();
    }

    T* get() const noexcept {
        return ptr;
    }

    T& operator*() const noexcept {
        return *ptr;
    }

    T* operator->() const noexcept {
        return ptr;
    }

    T& operator[](std::ptrdiff_t idx) const {
        return ptr[idx];
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    [[nodiscard]] size_t use_count() const noexcept {
        return cb == nullptr ? 0 : cb->use_count();
    }

    void reset() noexcept {
        shared_ptr().swap(*this);
    }

    template<class U>
    void reset(U* pU) {
        shared_ptr(pU).swap(*this);
    }

    template<class U, class D>
    void reset(U* pU, D d) {
        shared_ptr(pU, std::move(d)).swap(*this);
    }

    shared_ptr &operator=(const shared_ptr &sp) noexcept {
        shared_ptr(sp).swap(*this);
        return *this;
    }

    shared_ptr &operator=(shared_ptr &&sp) noexcept {
        shared_ptr(std::move(sp)).swap(*this);
        return *this;
    }

    template<class U>
    shared_ptr &operator=(shared_ptr<U> &&sp) {
        shared_ptr(std::move(sp)).swap(*this);
        return *this;
    }

    void swap(shared_ptr &sp) noexcept {
        std::swap(ptr, sp.ptr);
        std::swap(cb , sp.cb );
    }

    template<typename V>
    bool operator==(shared_ptr<V> const& lhs) {
        return ptr == lhs.get();
    }

    template<typename V>
    bool operator!=(shared_ptr<V> const& lhs) {
        return ptr != lhs.get();
    }

    bool operator==(std::nullptr_t) {
        return ptr == nullptr;
    }

    bool operator!=(std::nullptr_t) {
        return ptr != nullptr;
    }

    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&& ... args);

    ~shared_ptr() {
        if (cb != nullptr && cb->use_count() != 0) {
            cb->release_ref();
        }
        if (cb != nullptr && cb->use_weak() == 0) {
            delete cb;
        }
    }
};

template<typename V>
bool operator==(std::nullptr_t, shared_ptr<V> const& lhs) {
    return lhs.get() == nullptr;
}

template<typename V>
bool operator!=(std::nullptr_t, shared_ptr<V> const& lhs) {
    return lhs.get() != nullptr;
}

template<typename Y, typename... Args>
shared_ptr<Y> make_shared(Args &&... args) {
    auto* b = new control_block_object<Y>(std::forward<Args>(args)...);
    b->add_ref();
    shared_ptr<Y> ans;
    ans.cb  = b;
    ans.ptr = b->get();
    return ans;
}
