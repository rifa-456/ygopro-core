// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../graph/copy.h"
#include <memory>
#include <optional>
class duel_checkpoint {
  public:
    explicit duel_checkpoint(duel &owner);
    bool belongs_to(const duel &owner) const noexcept;
    void restore(std::optional<uint64_t> seed);
    std::size_t bytes() const noexcept {
        return heap_.bytes.size();
    }
    static void invalidate(duel &owner) noexcept;

  private:
    struct Patch {
        void *location;
        void **value;
        int kind;
    };
    duel *owner_;
    std::weak_ptr<int> identity_;
    std::unique_ptr<duel> native_;
    std::unique_ptr<duel_clone> copier_;
    ocg::lua_clone::HeapSnapshot heap_;
    lua_State *main_{};
    lua_State *current_{};
    interpreter::coroutine_map coroutines_;
    std::vector<Patch> patches_;
    void copy_interpreter();
};
