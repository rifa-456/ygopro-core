// SPDX-License-Identifier: AGPL-3.0-or-later
#include "object_pool.h"
#include "../../card.h"
#include "../../effect.h"
#include "../../field.h"
#include "../../group.h"
#include <limits>
#include <new>
namespace ocg::memory {
ObjectPool::~ObjectPool() {
    for (auto *block : blocks_)
        ::operator delete(block);
}
void *ObjectPool::allocate(std::size_t size) {
    if (size > std::numeric_limits<std::size_t>::max() - sizeof(Block))
        throw std::bad_alloc();
    auto *block =
        ::new (::operator new(sizeof(Block) + size)) Block{nullptr, nullptr, ObjectKind::Card};
    return block + 1;
}
void *ObjectPool::acquire(std::size_t size, ObjectKind kind) {
    if (requests >= fail_after)
        throw std::bad_alloc();
    ++requests;
    const auto index = static_cast<std::size_t>(kind);
    if (auto *block = free_[index]) {
        free_[index] = block->next;
        return block + 1;
    }
    auto *result = allocate(size);
    auto *block = static_cast<Block *>(result) - 1;
    try {
        blocks_.push_back(block);
    } catch (...) {
        ::operator delete(block);
        throw;
    }
    block->owner = this;
    block->kind = kind;
    return result;
}
void ObjectPool::release(void *pointer) noexcept {
    if (!pointer)
        return;
    auto *block = static_cast<Block *>(pointer) - 1;
    if (auto *owner = block->owner) {
        auto &head = owner->free_[static_cast<std::size_t>(block->kind)];
        block->next = head;
        head = block;
    } else
        ::operator delete(block);
}
} // namespace ocg::memory
void *card::operator new(std::size_t size) {
    return ocg::memory::ObjectPool::allocate(size);
}
void *card::operator new(std::size_t size, ocg::memory::ObjectPool &pool) {
    return pool.acquire(size, ocg::memory::ObjectKind::Card);
}
void card::operator delete(void *pointer) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void card::operator delete(void *pointer, ocg::memory::ObjectPool &) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void *effect::operator new(std::size_t size) {
    return ocg::memory::ObjectPool::allocate(size);
}
void *effect::operator new(std::size_t size, ocg::memory::ObjectPool &pool) {
    return pool.acquire(size, ocg::memory::ObjectKind::Effect);
}
void effect::operator delete(void *pointer) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void effect::operator delete(void *pointer, ocg::memory::ObjectPool &) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void *group::operator new(std::size_t size) {
    return ocg::memory::ObjectPool::allocate(size);
}
void *group::operator new(std::size_t size, ocg::memory::ObjectPool &pool) {
    return pool.acquire(size, ocg::memory::ObjectKind::Group);
}
void group::operator delete(void *pointer) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void group::operator delete(void *pointer, ocg::memory::ObjectPool &) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void *field::operator new(std::size_t size) {
    return ocg::memory::ObjectPool::allocate(size);
}
void *field::operator new(std::size_t size, ocg::memory::ObjectPool &pool) {
    return pool.acquire(size, ocg::memory::ObjectKind::Field);
}
void field::operator delete(void *pointer) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
void field::operator delete(void *pointer, ocg::memory::ObjectPool &) noexcept {
    ocg::memory::ObjectPool::release(pointer);
}
