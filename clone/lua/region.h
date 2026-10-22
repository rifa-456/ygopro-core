// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../memory/buffer.h"
#include <array>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace ocg::lua_clone {
class Region {
  public:
    explicit Region(std::size_t capacity);
    void *resize(void *pointer, std::size_t size);
    void release(void *pointer) noexcept;
    std::vector<std::pair<std::uintptr_t, std::size_t>> blocks() const;
    std::vector<unsigned char> snapshot() const;
    void restore(const std::vector<unsigned char> &bytes);
    bool empty() const noexcept;

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
    boost::interprocess::managed_external_buffer segment_;
    std::size_t state_offset_{};
    State &state() const noexcept;
    std::size_t offset(const void *pointer) const noexcept;
    Block *block(std::size_t offset) const noexcept;
    void *allocate(std::size_t size);
};
} // namespace ocg::lua_clone
