// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../../ocgapi.h"
#include "checkpoint.h"
#include "lfunc.h"
#include "ltable.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
namespace {
void *lua_pointer(void *value, int kind) {
    switch (kind) {
    case 1:
        return static_cast<lua_obj *>(static_cast<card *>(value));
    case 2:
        return static_cast<lua_obj *>(static_cast<effect *>(value));
    case 3:
        return static_cast<lua_obj *>(static_cast<group *>(value));
    default:
        return value;
    }
}
} // namespace
bool duel_checkpoint::belongs_to(const duel &owner) const noexcept {
    return &owner == owner_ && !identity_.expired() &&
           identity_.lock() == owner.checkpoint_identity;
}
void duel_checkpoint::copy_interpreter() {
    auto &source = *native_->lua;
    auto &target = *owner_->lua;
    for (auto [v, type] : source.params) {
        if (type == LuaParam::CARD || type == LuaParam::EFFECT || type == LuaParam::GROUP ||
            type == LuaParam::DELETED)
            v.ptr = copier_->objects.at(v.ptr);
        target.params.emplace_back(v, type);
    }
    target.no_action = source.no_action;
    target.call_depth = source.call_depth;
    target.weak_lua_references = source.weak_lua_references;
    target.loaded_scripts_table = source.loaded_scripts_table;
    target.clone_safe = source.clone_safe;
    static_cast<lua_obj &>(target.deleted) = static_cast<const lua_obj &>(source.deleted);
    target.deleted.pduel = owner_;
    target.coroutines = coroutines_;
    target.lua_state = main_;
    target.current_state = current_;
}
void duel_checkpoint::restore(std::optional<uint64_t> seed) {
    auto &target = *owner_;
    target.clone_constructing = true;
    try {
        delete target.game_field;
        target.game_field = nullptr;
        for (auto *p : target.cards)
            delete p;
        target.cards.clear();
        for (auto *p : target.effects)
            delete p;
        target.effects.clear();
        for (auto *p : target.groups)
            delete p;
        target.groups.clear();
        for (auto *p : target.cloned_chain_counters)
            delete p;
        target.cloned_chain_counters.clear();
        target.assumes.clear();
        target.uncopy.clear();
        target.lua->params.clear();
        target.lua->coroutines.clear();
        copier_->counters.clear();
        OCG_DuelOptions options{};
        target.game_field = new (target.storage) field(&target, options);
        copier_->objects.at(native_->game_field) = target.game_field;
        copier_->allocate_objects();
        copier_->copy_objects();
        copier_->copy_field();
        copier_->copy_metadata();
        copy_interpreter();
        target.lua->heap.restore(heap_);
        for (const auto &patch : patches_) {
            auto *pointer = lua_pointer(*patch.value, patch.kind);
            std::memcpy(patch.location, &pointer, sizeof(pointer));
        }
        target.last_status = native_->last_status;
        target.clone_constructing = false;
        if (seed)
            duel_clone::reseed(target, *seed);
    } catch (...) {
        invalidate(target);
        throw;
    }
}
void duel_checkpoint::invalidate(duel &target) noexcept {
    target.clone_constructing = true;
    target.has_ended = true;
    target.last_status = -1;
    target.checkpoint_identity.reset();
    if (target.lua) {
        target.lua->lua_state = nullptr;
        target.lua->current_state = nullptr;
        target.lua->heap.release_all();
    }
}
