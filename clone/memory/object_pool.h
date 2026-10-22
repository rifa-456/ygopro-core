// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <limits>
#include <vector>
namespace ocg::memory {
enum class ObjectKind : std::size_t { Card, Effect, Group, Field };
class ObjectPool {
  public:
    std::size_t requests{};
    std::size_t fail_after = std::numeric_limits<std::size_t>::max();
    ObjectPool() = default;
    ~ObjectPool();
    ObjectPool(const ObjectPool &) = delete;
    ObjectPool &operator=(const ObjectPool &) = delete;
    void *acquire(std::size_t size, ObjectKind kind);
    static void *allocate(std::size_t size);
    static void release(void *pointer) noexcept;

  private:
    struct alignas(std::max_align_t) Block {
        ObjectPool *owner;
        Block *next;
        ObjectKind kind;
    };
    std::array<Block *, 4> free_{};
    std::vector<Block *> blocks_;
};
} // namespace ocg::memory
