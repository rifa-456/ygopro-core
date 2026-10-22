// SPDX-License-Identifier: AGPL-3.0-or-later
#include "region.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
namespace ocg::lua_clone {
Region::Region(std::size_t capacity) : buffer_(capacity) {
    const auto initial = std::min(capacity, quantum * 2);
    if (initial < quantum)
        throw std::invalid_argument("Lua reserve is smaller than 64 KiB");
    buffer_.commit(initial);
    segment_ = boost::interprocess::managed_external_buffer(boost::interprocess::create_only,
                                                            buffer_.data(), initial);
    auto *value = ::new (segment_.allocate(sizeof(State))) State{};
    state_offset_ = offset(value);
}
std::size_t Region::offset(const void *pointer) const noexcept {
    return static_cast<const unsigned char *>(pointer) -
           static_cast<const unsigned char *>(buffer_.data());
}
Region::Block *Region::block(std::size_t value) const noexcept {
    return reinterpret_cast<Block *>(static_cast<unsigned char *>(buffer_.data()) + value);
}
Region::State &Region::state() const noexcept {
    return *reinterpret_cast<State *>(static_cast<unsigned char *>(buffer_.data()) + state_offset_);
}
bool Region::empty() const noexcept {
    return state().count == 0;
}
void *Region::allocate(std::size_t size) {
    if (size > buffer_.capacity() - sizeof(Block) - alignment)
        throw std::bad_alloc();
    const auto capacity = (size + alignment - 1) / alignment * alignment;
    const auto index = capacity / alignment - 1;
    auto &control = state();
    Block *value = nullptr;
    if (index < classes && control.free[index]) {
        value = block(control.free[index]);
        control.free[index] = value->next;
    } else {
        for (;;) {
            try {
                value = static_cast<Block *>(segment_.allocate(sizeof(Block) + capacity));
                break;
            } catch (const boost::interprocess::bad_alloc &) {
                const auto extent = segment_.get_size();
                const auto growth = std::max(
                    quantum, ((capacity + sizeof(Block) + quantum - 1) / quantum) * quantum);
                if (growth > buffer_.capacity() - extent)
                    throw std::bad_alloc();
                buffer_.commit(extent + growth);
                segment_.grow(growth);
            }
        }
        ::new (static_cast<void *>(value)) Block{0, capacity, 0, 0};
    }
    value->size = size;
    value->previous = 0;
    value->next = control.live;
    if (control.live)
        block(control.live)->previous = offset(value);
    control.live = offset(value);
    ++control.count;
    return value + 1;
}
void Region::release(void *pointer) noexcept {
    if (!pointer)
        return;
    auto *value = static_cast<Block *>(pointer) - 1;
    auto &control = state();
    if (value->previous)
        block(value->previous)->next = value->next;
    else
        control.live = value->next;
    if (value->next)
        block(value->next)->previous = value->previous;
    --control.count;
    const auto index = value->capacity / alignment - 1;
    if (index < classes) {
        value->next = control.free[index];
        control.free[index] = offset(value);
    } else
        segment_.deallocate(value);
}
void *Region::resize(void *pointer, std::size_t size) {
    if (!pointer)
        return allocate(size);
    auto *value = static_cast<Block *>(pointer) - 1;
    if (size <= value->capacity) {
        value->size = size;
        return pointer;
    }
    auto *next = allocate(size);
    std::memcpy(next, pointer, std::min(size, value->size));
    release(pointer);
    return next;
}
std::vector<std::pair<std::uintptr_t, std::size_t>> Region::blocks() const {
    std::vector<std::pair<std::uintptr_t, std::size_t>> result;
    result.reserve(state().count);
    for (auto at = state().live; at; at = block(at)->next) {
        const auto *value = block(at);
        result.emplace_back(reinterpret_cast<std::uintptr_t>(value + 1), value->size);
    }
    if (result.size() != state().count)
        throw std::logic_error("inconsistent Lua allocation census");
    std::sort(result.begin(), result.end());
    return result;
}
std::vector<unsigned char> Region::snapshot() const {
    const auto *start = static_cast<const unsigned char *>(buffer_.data());
    return {start, start + segment_.get_size()};
}
void Region::restore(const std::vector<unsigned char> &bytes) {
    if (bytes.empty() || bytes.size() > buffer_.capacity())
        throw std::invalid_argument("invalid Lua checkpoint region");
    buffer_.commit(bytes.size());
    std::memcpy(buffer_.data(), bytes.data(), bytes.size());
}
} // namespace ocg::lua_clone
