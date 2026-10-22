// SPDX-License-Identifier: AGPL-3.0-or-later
#include "vm.h"
#include <algorithm>
namespace ocg::persistence {
VmArchive::VmArchive(duel_archive &native, Stream &stream, interpreter &vm)
    : native_(native), stream_(stream), vm_(vm) {}
VmArchive::Reference VmArchive::encode(std::uintptr_t address) const {
    if (!address)
        return {};
    auto found = index_.upper_bound(address);
    if (found != index_.begin()) {
        auto [base, index] = *--found;
        if (address - base <= blocks_[index].bytes.size())
            return {1, index, address - base};
    }
    try {
        return {2, native_.native(reinterpret_cast<void *>(address)), 0};
    } catch (const std::invalid_argument &) {
    }
    auto base = image_base(reinterpret_cast<const void *>(&ocg::lua_clone::Heap::allocate));
    if (image_base(reinterpret_cast<const void *>(address)) != base)
        throw std::invalid_argument("snapshot contains a pointer outside the engine module");
    return {3, 0, address - base};
}
std::uintptr_t VmArchive::decode(const Reference &ref) const {
    switch (ref.kind) {
    case 0:
        if (ref.index || ref.offset)
            break;
        return 0;
    case 1:
        if (ref.index >= blocks_.size() || ref.offset > blocks_[ref.index].bytes.size())
            break;
        return blocks_[ref.index].address + ref.offset;
    case 2:
        if (ref.offset)
            break;
        return reinterpret_cast<std::uintptr_t>(native_.native(ref.index));
    case 3: {
        auto base = image_base(reinterpret_cast<const void *>(&ocg::lua_clone::Heap::allocate));
        auto address = base + ref.offset;
        if (address < base || image_base(reinterpret_cast<const void *>(address)) != base)
            break;
        return address;
    }
    default:
        break;
    }
    throw std::invalid_argument("invalid snapshot relocation");
}
void VmArchive::slot(void *location, std::uintptr_t address) {
    auto at = reinterpret_cast<std::uintptr_t>(location);
    auto found = index_.upper_bound(at);
    if (found == index_.begin())
        throw std::invalid_argument("unowned snapshot pointer slot");
    auto [base, index] = *--found;
    auto offset = at - base;
    if (offset > blocks_[index].bytes.size() ||
        sizeof(address) > blocks_[index].bytes.size() - offset)
        throw std::invalid_argument("snapshot pointer slot outside allocation");
    patches_.push_back({index, offset, encode(address)});
    std::memset(blocks_[index].bytes.data() + offset, 0, sizeof(address));
}
void VmArchive::run() {
    if (!stream_.reading) {
        for (auto [address, size] : vm_.heap.blocks()) {
            index_.emplace(address, blocks_.size());
            auto *start = reinterpret_cast<const unsigned char *>(address);
            blocks_.push_back({address, {start, start + size}});
        }
        auto *state = G(vm_.lua_state);
        global(*state);
        thread(*vm_.lua_state);
        for (auto *first : {state->allgc, state->finobj, state->tobefnz, state->fixedgc})
            for (auto *item = first; item; item = item->next)
                object(*item);
    }
    auto n = stream_.count(blocks_.size());
    if (stream_.reading)
        blocks_.resize(n);
    for (auto &block : blocks_) {
        auto size = stream_.count(block.bytes.size());
        if (!size)
            throw std::invalid_argument("empty snapshot Lua allocation");
        if (stream_.reading) {
            block.bytes.resize(size);
            auto *output = ocg::lua_clone::Heap::allocate(&vm_.heap, nullptr, 0, size);
            if (!output)
                throw std::bad_alloc();
            block.address = reinterpret_cast<std::uintptr_t>(output);
        }
        stream_.bytes(block.bytes.data(), size);
        if (stream_.reading)
            std::memcpy(reinterpret_cast<void *>(block.address), block.bytes.data(), size);
    }
    n = stream_.count(patches_.size());
    if (stream_.reading)
        patches_.resize(n);
    for (auto &patch : patches_) {
        stream_.scalar(patch);
        if (stream_.reading) {
            if (patch.block >= blocks_.size())
                throw std::invalid_argument("invalid snapshot patch block");
            auto &block = blocks_[patch.block];
            if (patch.offset > block.bytes.size() ||
                sizeof(std::uintptr_t) > block.bytes.size() - patch.offset)
                throw std::invalid_argument("invalid snapshot patch offset");
            auto address = decode(patch.target);
            std::memcpy(reinterpret_cast<void *>(block.address + patch.offset), &address,
                        sizeof(address));
        }
    }
    pointer(vm_.lua_state);
    pointer(vm_.current_state);
    if (stream_.reading)
        rehash();
    n = stream_.count(vm_.coroutines.size());
    if (stream_.reading) {
        for (std::uint64_t i = 0; i < n; ++i) {
            int32_t key{}, status{};
            lua_State *thread{};
            stream_.scalar(key);
            pointer(thread);
            stream_.scalar(status);
            vm_.coroutines.emplace(key, std::make_pair(thread, status));
        }
    } else
        for (auto &[key, item] : vm_.coroutines) {
            auto id = key;
            stream_.scalar(id);
            pointer(item.first);
            stream_.scalar(item.second);
        }
}
} // namespace ocg::persistence
