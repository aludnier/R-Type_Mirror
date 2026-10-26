/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ThreadSafeQueue.h
*/

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

namespace RType::Network {
/**
 * @brief FIFO queue shared by exactly two threads: the network thread
 *        pushes, the game thread pops.
 *
 * The queue is bounded: when it is full, push() drops the new value
 * instead of growing, so that a flood of datagrams cannot exhaust the
 * memory (docs/protocol.md, section 9).
 */
template<class T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(std::size_t maxSize) :
        _maxSize(maxSize)
    {
    }

    /** @return false if the queue was full and @p value was dropped. */
    bool push(T value)
    {
        std::lock_guard lock(_mutex);

        if (_queue.size() >= _maxSize) {
            return false;
        }
        _queue.push(std::move(value));
        return true;
    }

    /** @return The oldest value, or std::nullopt if the queue is empty. */
    std::optional<T> tryPop()
    {
        std::lock_guard lock(_mutex);

        if (_queue.empty()) {
            return std::nullopt;
        }
        T value = std::move(_queue.front());

        _queue.pop();
        return value;
    }

private:
    std::mutex _mutex;
    std::queue<T> _queue;
    std::size_t _maxSize;
};
} // namespace RType::Network

#endif // THREADSAFEQUEUE_H
