// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../../ocgapi.h"
#include "checkpoint.h"
#include "lfunc.h"
#include "ltable.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
duel_checkpoint::duel_checkpoint(duel &owner)
    : owner_(&owner), identity_(owner.checkpoint_identity), native_(new duel(duel::clone_tag{})),
      main_(owner.lua->lua_state), current_(owner.lua->current_state),
      coroutines_(owner.lua->coroutines) {
    auto &saved = *native_;
    saved.clone_constructing = true;
    saved.read_card_callback = owner.read_card_callback;
    saved.read_card_payload = owner.read_card_payload;
    saved.read_script_callback = owner.read_script_callback;
    saved.read_script_payload = owner.read_script_payload;
    saved.handle_message_callback = owner.handle_message_callback;
    saved.handle_message_payload = owner.handle_message_payload;
    saved.read_card_done_callback = owner.read_card_done_callback;
    saved.read_card_done_payload = owner.read_card_done_payload;
    duel_clone capture(owner, saved);
    capture.run();
    saved.lua->lua_state = nullptr;
    saved.lua->current_state = nullptr;
    saved.lua->heap.release_all();
    copier_ = std::make_unique<duel_clone>(saved, owner);
    std::unordered_map<const void *, const void *> roots;
    for (auto [original, copy] : capture.objects) {
        copier_->objects.emplace(copy, const_cast<void *>(original));
        roots.emplace(original, copy);
    }
    copier_->objects.emplace(&saved.lua->heap, &owner.lua->heap);
    roots.emplace(&owner.lua->heap, &saved.lua->heap);
    std::unordered_map<const void *, std::pair<const void *, int>> userdata;
    for (auto *p : saved.cards)
        userdata.emplace(static_cast<lua_obj *>(static_cast<card *>(copier_->mapped(p))),
                         std::make_pair(p, 1));
    for (auto *p : saved.effects)
        userdata.emplace(static_cast<lua_obj *>(static_cast<effect *>(copier_->mapped(p))),
                         std::make_pair(p, 2));
    for (auto *p : saved.groups)
        userdata.emplace(static_cast<lua_obj *>(static_cast<group *>(copier_->mapped(p))),
                         std::make_pair(p, 3));
    userdata.emplace(&owner.lua->deleted, std::make_pair(&saved.lua->deleted, 4));
    const auto live = owner.lua->heap.blocks();
    auto external = [&](void *location, void *pointer) {
        if (!pointer)
            return;
        if (const auto found = roots.find(pointer); found != roots.end()) {
            patches_.push_back({location, &copier_->objects.at(found->second), 0});
            return;
        }
        const auto address = reinterpret_cast<std::uintptr_t>(pointer);
        auto found = std::upper_bound(live.begin(), live.end(), address,
                                      [](auto p, const auto &b) { return p < b.first; });
        if (found != live.begin()) {
            --found;
            if (address - found->first <= found->second)
                return;
        }
        throw std::runtime_error("unknown checkpoint external pointer");
    };
    auto value = [&](TValue &v) {
        if (ttislightuserdata(&v))
            external(&v.value_.p, v.value_.p);
    };
    auto thread = [&](lua_State *L) {
        void *pointer;
        std::memcpy(&pointer, lua_getextraspace(L), sizeof(pointer));
        external(lua_getextraspace(L), pointer);
        if (L->stack.p)
            for (auto *s = L->stack.p; s < L->top.p; ++s)
                value(s->val);
    };
    auto *g = G(main_);
    value(g->l_registry);
    value(g->nilvalue);
    external(&g->ud_warn, g->ud_warn);
    thread(main_);
    for (auto *head : {g->allgc, g->finobj, g->tobefnz, g->fixedgc})
        for (auto *object = head; object; object = object->next) {
            switch (object->tt) {
            case LUA_VTABLE: {
                auto *t = gco2t(object);
                for (unsigned i = 0; i < luaH_realasize(t); ++i)
                    value(t->array[i]);
                if (!isdummy(t))
                    for (unsigned i = 0; i < sizenode(t); ++i)
                        value(t->node[i].i_val);
                break;
            }
            case LUA_VUSERDATA: {
                auto *u = gco2u(object);
                for (int i = 0; i < u->nuvalue; ++i)
                    value(u->uv[i].uv);
                if (u->len == sizeof(lua_obj *)) {
                    void *pointer;
                    std::memcpy(&pointer, getudatamem(u), sizeof(pointer));
                    const auto [key, kind] = userdata.at(pointer);
                    patches_.push_back({getudatamem(u), &copier_->objects.at(key), kind});
                }
                break;
            }
            case LUA_VCCL: {
                auto *c = gco2ccl(object);
                for (int i = 0; i < c->nupvalues; ++i)
                    value(c->upvalue[i]);
                break;
            }
            case LUA_VUPVAL: {
                auto *u = gco2upv(object);
                if (!upisopen(u))
                    value(u->u.value);
                break;
            }
            case LUA_VPROTO: {
                auto *p = gco2p(object);
                for (int i = 0; i < p->sizek; ++i)
                    value(p->k[i]);
                break;
            }
            case LUA_VTHREAD:
                thread(gco2th(object));
                break;
            default:
                break;
            }
        }
    heap_ = owner.lua->heap.snapshot();
}
