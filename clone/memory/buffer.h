// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstddef>
namespace ocg::memory {
class Buffer {
  public:
    explicit Buffer(std::size_t capacity);
    ~Buffer();
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    void commit(std::size_t size);
    void *data() const noexcept {
        return data_;
    }
    std::size_t capacity() const noexcept {
        return capacity_;
    }

  private:
    void *data_{};
    std::size_t capacity_{};
    std::size_t committed_{};
};
} // namespace ocg::memory
