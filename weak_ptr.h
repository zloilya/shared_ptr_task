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
            : cb(r.cb), ptr(r.ptr) {
        r.cb  = nullptr;
        r.ptr = nullptr;
    }

    template<class Y>
    weak_ptr(weak_ptr<Y> &&r) noexcept
            : cb(r.cb), ptr(r.ptr) {
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

    [[nodiscard]] bool expired() const noexcept {
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

