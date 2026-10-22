// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace ocg::persistence {
class Stream {
  public:
    explicit Stream(std::vector<unsigned char> &data, bool reading)
        : reading(reading), data_(data) {}
    const bool reading;
    template <class T> void scalar(T &value) {
        static_assert(std::is_trivially_copyable_v<T>);
        bytes(&value, sizeof(value));
    }
    void bytes(void *value, std::size_t size) {
        if (!size)
            return;
        if (reading) {
            if (size > data_.size() - position_)
                throw std::invalid_argument("truncated duel snapshot");
            std::memcpy(value, data_.data() + position_, size);
            position_ += size;
        } else {
            if (size > limit - data_.size())
                throw std::length_error("duel snapshot exceeds limit");
            auto *first = static_cast<unsigned char *>(value);
            data_.insert(data_.end(), first, first + size);
        }
    }
    std::uint64_t count(std::size_t size = 0) {
        std::uint64_t result = size;
        scalar(result);
        if (result > limit || (reading && result > data_.size()))
            throw std::invalid_argument("invalid snapshot collection size");
        return result;
    }
    void finish() const {
        if (reading && position_ != data_.size())
            throw std::invalid_argument("trailing snapshot bytes");
    }
    static constexpr std::size_t limit = 512u * 1024u * 1024u;

  private:
    std::vector<unsigned char> &data_;
    std::size_t position_{};
};
} // namespace ocg::persistence
