#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <array>
#include <vector>
#include <cassert>
#include <iostream>
#include <numeric>
#include <chrono>
#include <future>

// Hazard Pointer Manager
class HazardPointerManager {
private:
    static constexpr size_t MAX_HAZARD_POINTERS = 100;
    static constexpr size_t MAX_RETIRED_NODES = 100;

    struct HazardPointer {
        std::atomic<void*> pointer{ nullptr };
        std::atomic<std::thread::id> owner{ std::thread::id{} };
    };

    struct RetiredNode {
        void* ptr;
        void (*deleter)(void*);
    };

    std::array<HazardPointer, MAX_HAZARD_POINTERS> hazard_pointers;
    thread_local static std::vector<RetiredNode> retired_nodes;

public:
    static HazardPointerManager& instance() { //singleton 
        static HazardPointerManager instance;
        return instance;
    }

    void* acquire_hazard_pointer() {
        auto thread_id = std::this_thread::get_id();
        for (auto& hp : hazard_pointers) {
            std::thread::id expected{};
            if (hp.owner.compare_exchange_weak(expected, thread_id)) {
                return &hp.pointer;
            }
        }
        throw std::runtime_error("No available hazard pointers");
    }

    void release_hazard_pointer(void* hp_ptr) {
        auto* hp = static_cast<std::atomic<void*>*>(hp_ptr);
        hp->store(nullptr);

        // Find and release ownership
        for (auto& hazard_ptr : hazard_pointers) {
            if (&hazard_ptr.pointer == hp) {
                hazard_ptr.owner.store(std::thread::id{});
                break;
            }
        }
    }

    template<typename T>
    void retire_node(T* node) {
        retired_nodes.push_back({ node, [](void* ptr) { delete static_cast<T*>(ptr); } });

        if (retired_nodes.size() >= MAX_RETIRED_NODES) {
            cleanup_retired_nodes();
        }
    }

private:
    void cleanup_retired_nodes() {
        std::vector<void*> hazard_ptrs;

        // Collect all active hazard pointers
        for (const auto& hp : hazard_pointers) {
            void* ptr = hp.pointer.load();
            if (ptr != nullptr) {
                hazard_ptrs.push_back(ptr);
            }
        }

        // Remove nodes that are still protected by hazard pointers
        auto it = std::remove_if(retired_nodes.begin(), retired_nodes.end(),
            [&hazard_ptrs](const RetiredNode& node) {
                bool is_hazardous = std::find(hazard_ptrs.begin(), hazard_ptrs.end(), node.ptr) != hazard_ptrs.end();
                if (!is_hazardous) {
                    node.deleter(node.ptr);
                    return true;
                }
                return false;
            });

        retired_nodes.erase(it, retired_nodes.end());
    }
};

inline thread_local std::vector<HazardPointerManager::RetiredNode> HazardPointerManager::retired_nodes;

// RAII Hazard Pointer Guard
class HazardPointerGuard {
private:
    std::atomic<void*>* hp_ptr;

public:
    HazardPointerGuard() : hp_ptr(static_cast<std::atomic<void*>*>(HazardPointerManager::instance().acquire_hazard_pointer())) {}

    ~HazardPointerGuard() {
        if (hp_ptr) {
            HazardPointerManager::instance().release_hazard_pointer(hp_ptr);
        }
    }

    HazardPointerGuard(const HazardPointerGuard&) = delete;
    HazardPointerGuard& operator=(const HazardPointerGuard&) = delete;

    HazardPointerGuard(HazardPointerGuard&& other) noexcept : hp_ptr(other.hp_ptr) {
        other.hp_ptr = nullptr;
    }

    HazardPointerGuard& operator=(HazardPointerGuard&& other) noexcept {
        if (this != &other) {
            if (hp_ptr) {
                HazardPointerManager::instance().release_hazard_pointer(hp_ptr);
            }
            hp_ptr = other.hp_ptr;
            other.hp_ptr = nullptr;
        }
        return *this;
    }

    template<typename T>
    T* protect(const std::atomic<T*>& atomic_ptr) {
        T* ptr;
        do {
            ptr = atomic_ptr.load();
            hp_ptr->store(ptr);
        } while (ptr != atomic_ptr.load());
        return ptr;
    }
};

// Lock-Free Queue Implementation
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<T*> data{ nullptr };
        std::atomic<Node*> next{ nullptr };

        Node() = default;
        explicit Node(T&& item) : data(new T(std::move(item))) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    std::atomic<size_t> size_counter{ 0 };

public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue() {
        while (pop()) {}
        delete head.load();
    }

    void push(T item) {
        Node* new_node = new Node(std::move(item));

        while (true) {
            HazardPointerGuard tail_guard;
            Node* last = tail_guard.protect(tail);
            Node* next = last->next.load();

            if (last == tail.load()) {
                if (next == nullptr) {
                    if (last->next.compare_exchange_weak(next, new_node)) {
                        tail.compare_exchange_weak(last, new_node);
                        size_counter.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
                else {
                    tail.compare_exchange_weak(last, next);
                }
            }
        }
    }

    std::optional<T> pop() {
        while (true) {
            HazardPointerGuard head_guard;
            Node* first = head_guard.protect(head);

            HazardPointerGuard tail_guard;
            Node* last = tail_guard.protect(tail);

            Node* next = first->next.load();

            if (first == head.load()) {
                if (first == last) {
                    if (next == nullptr) {
                        return std::nullopt; // Queue is empty
                    }
                    tail.compare_exchange_weak(last, next);
                }
                else {
                    if (next == nullptr) {
                        continue;
                    }

                    T* data = next->data.load();
                    if (data == nullptr) {
                        continue;
                    }

                    if (head.compare_exchange_weak(first, next)) {
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

    size_t length() const {
        return size_counter.load(std::memory_order_relaxed);
    }
};

