#ifndef BUFFER_H
#define BUFFER_H

#include <array>
#include <deque>
#include <thread>

/**
 * A thread-safe queue class.
 */
class Queue {
public:
    Queue() = default;

    /**
     * Push a message to the queue.
     */
    void push(const std::array<int, 4>& message) {
        while (true) {
            std::unique_lock<std::mutex> locker(mu);
            cond.wait(locker, [this](){ return buffer_.size() < size_; });
            buffer_.push_back(message);
            locker.unlock();
            cond.notify_all();
            return;
        }
    }

    /**
     * Pop a message off the queue.
     */
    bool pop(std::array<int, 4>& value) {
        while (true) {
            std::unique_lock<std::mutex> locker(mu);
            if (buffer_.size() == 0) {
                return false;
            }
            value = buffer_.front();
            buffer_.pop_front();
            locker.unlock();
            cond.notify_all();
            return true;
        }
    }

private:
    std::mutex mu;
    std::condition_variable cond;

    std::deque<std::array<int, 4>> buffer_;
    const unsigned int size_ = 200;
    
};

#endif /* BUFFER_H */
