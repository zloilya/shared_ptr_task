#pragma once

struct control_block {
    size_t ref_cnt  = 0;
    size_t weak_cnt = 0;

    void release_ref() {
        --ref_cnt;
        --weak_cnt;
        if (ref_cnt == 0) {
            delete_object();
        }
    }
    void release_weak() {
        --weak_cnt;
    }

    void add_ref() {
        ref_cnt++;
        weak_cnt++;
    }
    void add_weak() {
        weak_cnt++;
    }

    [[nodiscard]] size_t use_count() const {
        return ref_cnt;
    }

    [[nodiscard]] size_t use_weak() const {
        return weak_cnt;
    }

    virtual ~control_block() = default;
    virtual void delete_object() = 0;
};

// type-erasure
// std::any, std::function

// erase Y, D
// -> empty base optimization

template <typename U, typename D = std::default_delete<U>>
struct cb_ptr final : control_block {
    U* ptr;
    D deleter;

    explicit cb_ptr(U* ptr, D deleter = std::default_delete<U>())
            : ptr(ptr)
            , deleter(std::move(deleter))
    {}

    void delete_object() override {
        deleter(ptr);
    }
};

template <typename U>
struct cb_obj : control_block {
    std::aligned_storage_t<sizeof(U), alignof(U)> data;

    template <typename... Args>
    explicit cb_obj(Args&& ...args) {
        new (&data) U(std::forward<Args>(args)...);
    }

    U* get() noexcept {
        return reinterpret_cast<U *>(&data);
    }

    void delete_object() {
        reinterpret_cast<U &>(data).~U();
    }
};

template<typename T>
struct shared_ptr;

template <typename T>
struct weak_ptr {

private:
    control_block *cb;

    T* ptr;

    friend class shared_ptr<T>;

    control_block* add_weak(control_block *b) {
        if (b != nullptr) {
            b->add_weak();
        }
        return b;
    }

public:
    constexpr weak_ptr() noexcept
            : cb(nullptr), ptr(nullptr) {}

    weak_ptr(const weak_ptr &r) noexcept
            : cb(add_weak(r.cb)), ptr(r.ptr) {}

    template<class Y>
    weak_ptr(const weak_ptr<Y> &r) noexcept
            : cb(add_weak(r.cb)), ptr(r.ptr) {}

    template<class Y>
    weak_ptr(const shared_ptr<Y> &r) noexcept
            : cb(add_weak(r.cb)), ptr(r.ptr) {}

    weak_ptr(weak_ptr &&r) noexcept
            : cb(add_weak(r.cb)), ptr(r.ptr) {
        r.cb  = nullptr;
        r.ptr = nullptr;
    }

    template<class Y>
    weak_ptr(weak_ptr<Y> &&r) noexcept
            : cb(add_weak(r.cb)), ptr(r.ptr) {
        r.cb  = nullptr;
        r.ptr = nullptr;
    }

    ~weak_ptr() {
        if (cb != nullptr) {
            cb->release_weak();
            if (cb->use_weak() == 0) {
                delete cb;
            }
        }
    }

    weak_ptr &operator=(weak_ptr const& wp) noexcept {
        weak_ptr(wp).swap(*this);
        return *this;
    }

    template<class U>
    weak_ptr &operator=(weak_ptr<U> const& wp) noexcept {
        weak_ptr(wp).swap(*this);
        return *this;
    }

    template<class U>
    weak_ptr &operator=(shared_ptr<U> const& wp) noexcept {
        weak_ptr(wp).swap(*this);
        return *this;
    }

    weak_ptr &operator=(weak_ptr&& wp) noexcept {
        weak_ptr(std::move(wp)).swap(*this);
        return *this;
    }

    template<class U>
    weak_ptr &operator=(weak_ptr<U>&& wp) noexcept {
        weak_ptr(std::move(wp)).swap(*this);
        return *this;
    }

    shared_ptr<T> lock() const noexcept {
        return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
    }

    bool expired() const noexcept {
        return cb == nullptr || cb->use_count() == 0;
    }

    void swap(weak_ptr& wp) noexcept {
        std::swap(ptr, wp.ptr);
        std::swap(cb , wp.cb );
    }

    void reset() noexcept {
        weak_ptr().swap(*this);
    }
};

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
            : cb(nullptr), ptr(nullptr) {}


    // shared_ptr(T*); -> T = Base => ~Base()
    // непустой объект
    template<typename U>
    shared_ptr(U* new_ptr) {
        try {
            cb  = new cb_ptr<U>(new_ptr, std::default_delete<U>());
            ptr = new_ptr;
            cb->add_ref();
        } catch (...) {
            delete new_ptr;
        }
    }

    // Base, Derived : Base.
    // shared_ptr<Base>(new Derived()) -> Y = Derived

    template<typename U, typename D>
    shared_ptr(U* new_ptr, D deleter) {
        try {
            cb  = new cb_ptr<U, D>(new_ptr, deleter);
            ptr = new_ptr;
            cb->add_ref();
        } catch (...) {
            deleter(new_ptr);
        }
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

    T* get() noexcept {
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
        if (cb == nullptr) return 0;
        return cb->use_count();
    }

    void reset() noexcept {
        if(cb != nullptr) {
            cb->release_ref();
        }
        shared_ptr().swap(*this);
    }

    template<class U>
    void reset(U *_ptr) {
        if(cb != nullptr)
            cb->release_ref();
        shared_ptr(_ptr).swap(*this);
    }

    template<class U, class D>
    void reset(U *_ptr, D _d) {
        if(cb != nullptr)
            cb->release_ref();
        shared_ptr(_ptr, _d).swap(*this);
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

    friend bool operator==(const shared_ptr &lhs, const shared_ptr &rhs) {
        return  lhs.ptr == rhs.ptr;
    }

    friend bool operator!=(const shared_ptr &lhs, const shared_ptr &rhs) {
        return  lhs.ptr != rhs.ptr;
    }

    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&& ... args);

    ~shared_ptr() {
        if (cb != nullptr && cb->use_count() != 0) {
            cb->release_ref();
        }
    }
};

template<typename Y, typename... Args>
shared_ptr<Y> make_shared(Args &&... args) {
    auto* b = new cb_obj<Y>(std::forward<Args>(args)...);
    b->add_ref();
    shared_ptr<Y> ans;
    ans.cb  = b;
    ans.ptr = b->get();
    return ans;
}
