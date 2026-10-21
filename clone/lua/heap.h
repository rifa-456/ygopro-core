// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <limits>

namespace ocg::lua_clone {
struct Heap {
    std::map<std::uintptr_t, std::size_t> blocks;
    std::size_t allocations = 0;
    std::size_t releases = 0;
    std::size_t fail_after = std::numeric_limits<std::size_t>::max();
    Heap() = default;
    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;
    ~Heap();
    static void* allocate(void* opaque, void* previous, std::size_t old_size,
                          std::size_t size) noexcept;
    void release_all() noexcept;
    std::uint64_t fingerprint() const;
};
}
