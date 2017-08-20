// Author - dhanjit
// Source - https://raw.githubusercontent.com/dhanjit/qlog/master/include/LockFreeQueue.hpp

#ifndef _LOCK_FREE_QUEUE_HPP_
#define _LOCK_FREE_QUEUE_HPP_
#include <unistd.h>
#include <atomic>
#include <memory>

template <std::size_t size, std::size_t extra_size = 0>
class LockFreeQueue {
  private:
    std::atomic<int> head __attribute__((aligned(64)));
    // char head_padding[128];
    std::atomic<int> tail __attribute__((aligned(64)));
    // char tail_padding[128];
    char buffer[size + extra_size] __attribute__((aligned(64)));
    // char *buffer = new char[size];    //__attribute__((aligned(64)));
    //                                   // char *buffer = new char[size];
    //                                   // char buffer_padding[128];

    // protected:
  public:
    template <typename T>
    __attribute__((always_inline)) void doPush(T &&arg) {
        auto storeAt = this->buffer + this->tail.load(std::memory_order_acquire);
        // new (storeAt) typename std::decay<T>::type{arg};
        memcpy(storeAt, &arg, sizeof(T));
        // *(T *)storeAt = arg;
    }

    template <typename T, typename... Args>
    __attribute__((always_inline)) void doEmplace(Args &&... args) {
        new (this->buffer + this->tail.load(std::memory_order_acquire)) T{std::forward<Args>(args)...};
    }

    template <typename T, typename... Args>
    __attribute__((always_inline)) inline void doOffsetEmplace(std::size_t offset, Args &&... args) {
        new (this->buffer + ((this->tail.load(std::memory_order_relaxed) + offset) & (size - 1))) T{std::forward<Args>(args)...};
    }

    __attribute__((always_inline)) void updateTail(std::size_t elemsize) { this->tail = ((this->tail + elemsize) & (size - 1)); }

    __attribute__((always_inline)) void updateHead(std::size_t elemsize) { this->head = ((this->head + elemsize) & (size - 1)); }

  public:
    LockFreeQueue() : head{0}, tail{0} { static_assert((size & (size - 1)) == 0, "size should be power of 2"); }
    ~LockFreeQueue() {}
    LockFreeQueue(LockFreeQueue &&) = delete;
    LockFreeQueue operator=(LockFreeQueue &&) = delete;

    static constexpr std::size_t maxSize() noexcept { return size - 1; };

    template <typename T, typename... Args>
    __attribute__((always_inline)) void emplace(Args &&... args) {
        this->doEmplace<T>(std::forward<Args>(args)...);
        this->updateTail(sizeof(T));
    }

    template <typename T>
    __attribute__((always_inline)) void push(T &&arg) {
        this->doPush(std::forward<T>(arg));
        this->updateTail(sizeof(T));
    }

    // This is just a guess.
    std::size_t fillSize() const { return ((size + this->tail - this->head) & (size - 1)); }

    bool empty() const { return this->head.load(std::memory_order_acquire) == this->tail.load(std::memory_order_acquire); }

    const void *front(int offset = 0) const { return this->buffer + ((this->head.load(std::memory_order_acquire) + offset) & (size - 1)); }

    template<typename type>
    const type *front(int offset = 0) const { return static_cast<const type *>(this->front(offset)); }

    __attribute__((always_inline)) void pop(std::size_t popsize) { this->updateHead(popsize); }

    // Exposed mostly for debugging. Shouldn't be required elsewhere.
    int getHead(std::memory_order mo = std::memory_order_relaxed) const { return this->head.load(mo); }
    int getTail(std::memory_order mo = std::memory_order_relaxed) const { return this->tail.load(mo); }

    __attribute__((always_inline)) inline bool canEnqueue(std::size_t requiredSize) const {
        // Don't do subtraction! std::size_t
        return __builtin_expect(this->fillSize() + requiredSize < maxSize() && requiredSize < extra_size, 1);
    }

};    // __attribute__((aligned (64)));

#endif
