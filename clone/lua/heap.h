#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
namespace ocg::lua_clone {
class Region;
struct HeapSnapshot {
    std::vector<unsigned char> bytes;
    std::size_t allocations{};
    std::size_t releases{};
};
class Heap {
  public:
    explicit Heap(std::size_t capacity = 256u * 1024u * 1024u);
    ~Heap();
    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;
    std::size_t allocations{};
    std::size_t releases{};
    std::size_t fail_after = std::numeric_limits<std::size_t>::max();
    static void *allocate(void *opaque, void *previous, std::size_t old_size,
                          std::size_t size) noexcept;
    void release_all() noexcept;
    bool empty() const noexcept;
    std::vector<std::pair<std::uintptr_t, std::size_t>> blocks() const;
    HeapSnapshot snapshot() const;
    void restore(const HeapSnapshot &snapshot);
    std::uint64_t fingerprint() const;

  private:
    std::size_t capacity_;
    std::unique_ptr<Region> region_;
};
} // namespace ocg::lua_clone
