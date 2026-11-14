#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace FBP {

// Owns objects behind integer handles handed out to PHP objects. Freed slots are
// reused, so a long-running process does not grow the list without bound. A handle
// carries its slot's generation: a stale handle (object freed, slot reused) is
// invalid instead of reaching the new occupant. 0 is never a valid handle.
template <typename T>
class Handle_List
{
    static constexpr unsigned IDX_BITS = sizeof(size_t) * 4;
    static constexpr size_t IDX_MASK = (size_t(1) << IDX_BITS) - 1;

    struct Slot {
        std::unique_ptr<T> ptr;
        size_t gen = 0;
    };

    std::vector<Slot> slots;
    std::vector<size_t> free_slots;

    static size_t gen_bits(size_t gen) { return gen & IDX_MASK; }

    Slot *find(size_t h)
    {
        size_t idx = h & IDX_MASK;
        if (!idx || idx > slots.size()) return nullptr;
        Slot &s = slots[idx - 1];
        if (!s.ptr || gen_bits(s.gen) != (h >> IDX_BITS)) return nullptr;
        return &s;
    }

    // Slot is free before the object is destroyed: its destructor may use the list
    void release(size_t idx)
    {
        Slot &s = slots[idx];
        std::unique_ptr<T> p = std::move(s.ptr);
        s.gen++;
        free_slots.push_back(idx);
        p.reset();
    }

public:
    size_t add(std::unique_ptr<T> p)
    {
        size_t idx;
        if (free_slots.empty()) {
            if (slots.size() >= IDX_MASK) throw std::length_error("Too many handles");
            idx = slots.size();
            slots.emplace_back();
        } else {
            idx = free_slots.back();
            free_slots.pop_back();
        }
        slots[idx].ptr = std::move(p);
        return (gen_bits(slots[idx].gen) << IDX_BITS) | (idx + 1);
    }

    template <typename... Args>
    size_t emplace(Args&&... args)
    {
        return add(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
    }

    bool is_valid(size_t h) { return find(h) != nullptr; }

    // Caller checks is_valid() first
    std::unique_ptr<T> &get(size_t h) { return find(h)->ptr; }

    void free(size_t h)
    {
        if (find(h)) release((h & IDX_MASK) - 1);
    }

    void clear()
    {
        for (size_t i = 0; i < slots.size(); i++) {
            if (slots[i].ptr) release(i);
        }
    }

    template <typename F>
    void for_each(F f)
    {
        for (auto &s: slots) {
            if (s.ptr) f(*s.ptr);
        }
    }
};

} // namespace
