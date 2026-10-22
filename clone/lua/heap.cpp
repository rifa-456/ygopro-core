#include "heap.h"
#include "region.h"
#include <cstring>
#include <new>
#include <stdexcept>
namespace ocg::lua_clone {
Heap::Heap(std::size_t capacity) : capacity_(capacity) {
}
Heap::~Heap() = default;
void *Heap::allocate(void *opaque, void *previous, std::size_t, std::size_t size) noexcept {
    auto &heap = *static_cast<Heap *>(opaque);
    try {
        if (size == 0) {
            if (previous) {
                heap.region_->release(previous);
                ++heap.releases;
            }
            return nullptr;
        }
        if (heap.allocations >= heap.fail_after)
            return nullptr;
        if (!heap.region_)
            heap.region_ = std::make_unique<Region>(heap.capacity_);
        auto *result = heap.region_->resize(previous, size);
        ++heap.allocations;
        if (previous)
            ++heap.releases;
        return result;
    } catch (...) {
        return nullptr;
    }
}
void Heap::release_all() noexcept {
    region_.reset();
}
bool Heap::empty() const noexcept {
    return !region_ || region_->empty();
}
std::vector<std::pair<std::uintptr_t, std::size_t>> Heap::blocks() const {
    return region_ ? region_->blocks() : std::vector<std::pair<std::uintptr_t, std::size_t>>{};
}
HeapSnapshot Heap::snapshot() const {
    if (!region_)
        throw std::logic_error("cannot capture an empty Lua heap");
    return {region_->snapshot(), allocations, releases};
}
void Heap::restore(const HeapSnapshot &snapshot) {
    if (!region_)
        throw std::logic_error("Lua checkpoint owner was released");
    region_->restore(snapshot.bytes);
    allocations = snapshot.allocations;
    releases = snapshot.releases;
}
std::uint64_t Heap::fingerprint() const {
    std::uint64_t result = 1469598103934665603ULL;
    for (auto [address, size] : blocks()) {
        result ^= address;
        result *= 1099511628211ULL;
        for (std::size_t i = 0; i < size; ++i) {
            result ^= reinterpret_cast<const unsigned char *>(address)[i];
            result *= 1099511628211ULL;
        }
    }
    return result;
}
} // namespace ocg::lua_clone
