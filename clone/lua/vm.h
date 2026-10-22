// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "heap.h"
#include <functional>
#include <map>
#include <unordered_set>
#include "lua.h"
#include "lstate.h"
#if !defined(LUA_OCG_CLONE_ABI) || LUA_OCG_CLONE_ABI != 1
#error "ocgcore cloning requires the matching Lua fork (clone ABI 1)"
#endif

namespace ocg::lua_clone {
struct Hooks {
    std::function<void*(const void*)> external;
    std::function<void(const Udata*, Udata*)> userdata;
    std::function<lua_KContext(lua_KFunction, lua_KContext)> continuation;
};

class Clone {
public:
    Clone(const Heap& source, Heap& destination, Hooks hooks);
    lua_State* run(lua_State* source);
    template<class T> T* mapped(T* pointer) const {
        return static_cast<T*>(translate(pointer));
    }
    std::size_t object_count() const { return objects_.size(); }

private:
    struct Block { std::size_t size; std::uintptr_t destination; };
    const Heap& source_;
    Heap& destination_;
    Hooks hooks_;
    std::map<std::uintptr_t, Block> blocks_;
    std::unordered_set<GCObject*> objects_;
    std::unordered_set<GCObject*> gray_;
    void* translate(const void* pointer) const;
    void* external(const void* pointer) const;
    void collect(GCObject* first);
    void collect_gray(GCObject* first);
    void fix_global(global_State* source);
    void fix_object(GCObject* source);
    void fix_table(Table* source);
    void fix_proto(Proto* source);
    void fix_thread(lua_State* source);
    void fix_value(TValue& destination, const TValue& source);
};
}
