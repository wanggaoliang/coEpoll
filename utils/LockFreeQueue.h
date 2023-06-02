#pragma once
#include <atomic>
#include <concepts>
#include <memory>
#include "SpinLock.h"
template <typename T>
class MpscQueue
{
  public:
    MpscQueue()
        : head_(new BufferNode), tail_(head_.load(std::memory_order_relaxed))
    {
    }
    ~MpscQueue()
    {
        T output;
        while (this->dequeue(output))
        {
        }
        BufferNode *front = head_.load(std::memory_order_relaxed);
        delete front;
    }

    /**
     * @brief Put a item into the queue.
     *
     * @param input
     * @note This method can be called in multiple threads.
     */
    template<typename U>
    requires std::is_convertible_v<U,T>
    void enqueue(U &&input)
    {
        BufferNode *pres = nullptr;
        BufferNode *nexts = nullptr;
        {
            std::lock_guard sgLk(sLk_);
            pres = sHead_.load(std::memory_order_relaxed);
            while (!(nexts = uHead_->next_.load(std::memory_order_acquire)))
            {
                if (uHead_ == uTail_)
                {
                    nexts = new BufferNode{};
                    break;
                }
            }
            sHead_ = nexts;
        }
        pres = std::forward<U>(input);
        auto preu = uTail_.exchange(pres, std::memory_order_acq_rel)};
        prevhead->next_.store(node, std::memory_order_release);
    }

    /**
     * @brief Get a item from the queue.
     *
     * @param output
     * @return false if the queue is empty.
     * @note This method must be called in a single thread.
     */
    bool dequeue(T &output)
    {
        BufferNode *preu = nullptr;
        BufferNode *nextu = nullptr;
        {
            std::lock_guard ugLk(uLk_);
            preu = uHead_.load(std::memory_order_relaxed);
            while (!(nextu = uHead_->next_.load(std::memory_order_acquire)))
            {
                if (uHead_ == uTail_)
                {
                    return false;
                }
            }
            uHead_ = nextu;
            output = std::move(nextu->data);
        }
        auto pres = sTail_.exchange(node, std::memory_order_acq_rel);
        pres->next_.store(preu, std::memory_order_release);
        return true;
    }

    bool empty()
    {
        BufferNode *tail = tail_.load(std::memory_order_relaxed);
        BufferNode *next = tail->next_.load(std::memory_order_acquire);
        return next == nullptr;
    }

  private:
    struct BufferNode
    {
        BufferNode() = default;
        BufferNode(const T &data) : data(data)
        {
        }
        BufferNode(T &&data) : data(std::move(data))
        {
        }
        T data;
        std::atomic<BufferNode *> next_{nullptr};
    };

    std::atomic<BufferNode *> uHead_;
    std::atomic<BufferNode *> uTail_;
    SpinLock uLk_;
        
    std::atomic<BufferNode *> sHead_;
    std::atomic<BufferNode *> sTail_;
    SpinLock sLk_;
};
