#pragma once
#include "hazardPointerGuard.h"

// Lock-Free Queue Implementation
template <typename T>
class LockFreeQueue
{
private:
    struct Node
    {
        std::atomic<T*>    data{nullptr};
        std::atomic<Node*> next{nullptr};

        Node() = default;
        explicit Node(T&& item) :
            data(new T(std::move(item))) {}
    };

    std::atomic<Node*>  head;
    std::atomic<Node*>  tail;
    std::atomic<size_t> size_counter{0};

public:
    LockFreeQueue()
    {
        Node* dummy = new Node;
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue()
    {
        while (pop()) {}
        delete head.load();
    }

    void push(T item)
    {
        Node* new_node = new Node(std::move(item));

        while (true)
        {
            HazardPointerGuard tail_guard;
            Node*              last = tail_guard.protect(tail);
            Node*              next = last->next.load();

            if (last == tail.load())
            {
                if (next == nullptr)
                {
                    if (last->next.compare_exchange_weak(next, new_node))
                    {
                        tail.compare_exchange_weak(last, new_node);
                        size_counter.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
                else
                {
                    tail.compare_exchange_weak(last, next);
                }
            }
        }
    }

    std::optional<T> pop()
    {
        while (true)
        {
            HazardPointerGuard head_guard;
            Node*              first = head_guard.protect(head);

            HazardPointerGuard tail_guard;
            Node*              last = tail_guard.protect(tail);

            Node* next = first->next.load();

            if (first == head.load())
            {
                if (first == last)
                {
                    if (next == nullptr)
                    {
                        return std::nullopt; // Queue is empty
                    }
                    tail.compare_exchange_weak(last, next);
                }
                else
                {
                    if (next == nullptr)
                    {
                        continue;
                    }

                    T* data = next->data.load();
                    if (data == nullptr)
                    {
                        continue;
                    }

                    if (head.compare_exchange_weak(first, next))
                    {
                        T result = *data;
                        delete data;
                        size_counter.fetch_sub(1, std::memory_order_relaxed);

                        // Retire the old head node
                        HazardPointerManager::instance().retire_node(first);

                        return result;
                    }
                }
            }
        }
    }

    size_t length() const
    {
        return size_counter.load(std::memory_order_relaxed);
    }
};
