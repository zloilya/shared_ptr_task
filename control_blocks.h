struct control_block {

private:
    size_t ref_cnt  = 0;
    size_t weak_cnt = 0;
public:

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
struct control_block_ptr final : control_block, D {
    U* ptr;

    explicit control_block_ptr(U* ptr, D deleter = std::default_delete<U>())
            : ptr(ptr), D(std::move(deleter))
    {}

    void delete_object() override {
        static_cast<D &>(*this)(ptr);
    }
};

template <typename U>
struct control_block_object final : control_block {
    std::aligned_storage_t<sizeof(U), alignof(U)> data;

    template <typename... Args>
    explicit control_block_object(Args&& ...args) {
        new (&data) U(std::forward<Args>(args)...);
    }

    U* get() noexcept {
        return reinterpret_cast<U *>(&data);
    }

    void delete_object() override {
        reinterpret_cast<U &>(data).~U();
    }
};