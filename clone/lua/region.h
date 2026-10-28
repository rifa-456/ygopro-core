// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../memory/buffer.h"
#include <array>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace ocg::lua_clone {
// managed_external_buffer with a null mutex: one thread uses a duel's heap at a time, and cloning copies the
// segment byte for byte, which a real mutex does not allow.
using Segment = boost::interprocess::basic_managed_external_buffer<
    char, boost::interprocess::rbtree_best_fit<boost::interprocess::null_mutex_family>,
    boost::interprocess::iset_index>;
class Region {
  public:
    explicit Region(std::size_t capacity);
    Region(const Region &source);
    void *resize(void *pointer, std::size_t size);
    void release(void *pointer) noexcept;
    std::vector<std::pair<std::uintptr_t, std::size_t>> blocks() const;
    std::vector<unsigned char> snapshot() const;
    void restore(const std::vector<unsigned char> &bytes);
    bool empty() const noexcept;
    std::uintptr_t base() const noexcept;
    std::size_t extent() const noexcept;

  private:
    static constexpr std::size_t quantum = 64u * 1024u;
    static constexpr std::size_t alignment = alignof(std::max_align_t);
    static constexpr std::size_t classes = 64;
    struct alignas(std::max_align_t) Block {
        std::size_t size, capacity, previous, next;
    };
    struct State {
        std::array<std::size_t, classes> free{};
        std::size_t live{}, count{};
    };
    ocg::memory::Buffer buffer_;
    Segment segment_;
    std::size_t state_offset_{};
    State &state() const noexcept;
    std::size_t offset(const void *pointer) const noexcept;
    Block *block(std::size_t offset) const noexcept;
    void *allocate(std::size_t size);
};
} // namespace ocg::lua_clone
