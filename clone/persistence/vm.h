// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "archive.h"
#include <map>
namespace ocg::persistence {
class VmArchive {
  public:
    VmArchive(duel_archive &native, Stream &stream, interpreter &vm);
    void run();
    template <class T> void pointer(T *&value) {
        auto reference =
            stream_.reading ? Reference{} : encode(reinterpret_cast<std::uintptr_t>(value));
        stream_.scalar(reference);
        if (stream_.reading)
            value = reinterpret_cast<T *>(decode(reference));
    }

  private:
    struct Reference {
        std::uint64_t kind{}, index{}, offset{};
    };
    struct Block {
        std::uintptr_t address{};
        std::vector<unsigned char> bytes;
    };
    struct Patch {
        std::uint64_t block{}, offset{};
        Reference target;
    };
    duel_archive &native_;
    Stream &stream_;
    interpreter &vm_;
    std::vector<Block> blocks_;
    std::map<std::uintptr_t, std::size_t> index_;
    std::vector<Patch> patches_;
    Reference encode(std::uintptr_t pointer) const;
    std::uintptr_t decode(const Reference &reference) const;
    void slot(void *location, std::uintptr_t value);
    template <class T> void slot(T &value) {
        static_assert(sizeof(T) == sizeof(std::uintptr_t));
        std::uintptr_t address;
        std::memcpy(&address, &value, sizeof(address));
        slot(&value, address);
    }
    void rehash();
    void value(TValue &value);
    void global(global_State &state);
    void thread(lua_State &state);
    void object(GCObject &object);
    void table(Table &table);
    void proto(Proto &proto);
};
std::uintptr_t image_base(const void *pointer);
} // namespace ocg::persistence
