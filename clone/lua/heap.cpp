// SPDX-License-Identifier: AGPL-3.0-or-later
#include "heap.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ocg::lua_clone {
Heap::~Heap() { release_all(); }

void* Heap::allocate(void* opaque, void* previous, std::size_t,
                     std::size_t size) noexcept {
    auto& heap = *static_cast<Heap*>(opaque);
    auto found = heap.blocks.find(reinterpret_cast<std::uintptr_t>(previous));
    if(previous && found == heap.blocks.end()) std::abort();
    if(size == 0) {
        if(previous) {
            std::free(previous);
            heap.blocks.erase(found);
            ++heap.releases;
        }
        return nullptr;
    }
    if(heap.allocations >= heap.fail_after) return nullptr;
    void* next = std::malloc(size);
    if(!next) return nullptr;
    try {
        heap.blocks.emplace(reinterpret_cast<std::uintptr_t>(next), size);
    } catch(...) {
        std::free(next);
        return nullptr;
    }
    ++heap.allocations;
    if(previous) {
        std::memcpy(next, previous, std::min(size, found->second));
        std::free(previous);
        heap.blocks.erase(found);
        ++heap.releases;
    }
    return next;
}

void Heap::release_all() noexcept {
    for(const auto& [address, size] : blocks) {
        (void)size;
        std::free(reinterpret_cast<void*>(address));
    }
    blocks.clear();
}

std::uint64_t Heap::fingerprint() const {
    std::uint64_t result = 1469598103934665603ULL;
    for(const auto& [address, size] : blocks) {
        auto bytes = reinterpret_cast<const unsigned char*>(address);
        result ^= address;
        result *= 1099511628211ULL;
        for(std::size_t i = 0; i < size; ++i) {
            result ^= bytes[i];
            result *= 1099511628211ULL;
        }
    }
    return result;
}
}
