// SPDX-License-Identifier: AGPL-3.0-or-later
#include "buffer.h"
#include <new>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif
namespace ocg::memory {
Buffer::Buffer(std::size_t capacity) : capacity_(capacity) {
#if defined(_WIN32)
    data_ = VirtualAlloc(nullptr, capacity, MEM_RESERVE, PAGE_NOACCESS);
    if (!data_)
        throw std::bad_alloc();
#else
    data_ = mmap(nullptr, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        throw std::bad_alloc();
    }
#endif
}
Buffer::~Buffer() {
#if defined(_WIN32)
    if (data_)
        VirtualFree(data_, 0, MEM_RELEASE);
#else
    if (data_)
        munmap(data_, capacity_);
#endif
}
void Buffer::commit(std::size_t size) {
    if (size > capacity_)
        throw std::bad_alloc();
    if (size <= committed_)
        return;
#if defined(_WIN32)
    if (!VirtualAlloc(data_, size, MEM_COMMIT, PAGE_READWRITE))
        throw std::bad_alloc();
#endif
    committed_ = size;
}
} // namespace ocg::memory
