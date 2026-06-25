#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace pbt {

// Thread-safe blocking queue with bounded capacity.
// Network thread: try_push (non-blocking, drops if full or lock contended).
// Processing thread: pop (blocks until item available or stopped).
template <typename T>
class BoundedBlockingQueue {
public:
    explicit BoundedBlockingQueue(std::size_t capacity) : capacity_(capacity) {}

    bool try_push(T&& item) {
        std::unique_lock<std::mutex> lk(mu_, std::try_to_lock);
        if (!lk || buf_.size() >= capacity_) return false;
        buf_.push_back(std::move(item));
        cv_.notify_one();
        return true;
    }

    bool try_push(const T& item) {
        T copy = item;
        return try_push(std::move(copy));
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this] { return !buf_.empty() || stopped_; });
        if (buf_.empty()) return std::nullopt;
        T item = std::move(buf_.front());
        buf_.pop_front();
        return item;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mu_);
        buf_.clear();
        stopped_ = false;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        return buf_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        return buf_.empty();
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t             capacity_;
    std::deque<T>           buf_;
    mutable std::mutex      mu_;
    std::condition_variable cv_;
    bool                    stopped_{false};
};

}  // namespace pbt
