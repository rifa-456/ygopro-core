// SPDX-License-Identifier: AGPL-3.0-or-later
#include "copy.h"
#include "../../scriptlib.h"
#include <lobject.h>

void duel_clone::copy_vm() {
    auto& s=*source.lua;
    auto& d=*destination.lua;
    ocg::lua_clone::Hooks hooks;
    hooks.external=[&](const void* p)->void* {
        if(p==&s.heap) return &d.heap;
        auto it=objects.find(p);
        if(it==objects.end()) throw std::runtime_error("unknown external Lua pointer");
        return it->second;
    };
    hooks.userdata=[&](const Udata* before,Udata* after) {
        if(before->len==sizeof(lua_obj*)) {
            lua_obj* p; std::memcpy(&p,getudatamem(before),sizeof(p));
            auto* result=mapped(p);
            std::memcpy(getudatamem(after),&result,sizeof(result));
        } else if(before->len!=32 || before->nuvalue!=0 || before->metatable) {
            throw std::runtime_error("unsupported Lua userdata");
        }
        // The safe math library's random generator owns 32 inline bytes.
    };
    hooks.continuation=[](lua_KFunction,lua_KContext context) {
        // Core helpers use 0/1; Lua base xpcall saves the scalar stack offset 2.
        if(context<0 || context>2) throw std::runtime_error("unsupported C continuation context");
        return context;
    };
    ocg::lua_clone::Clone clone(s.heap,d.heap,std::move(hooks));
    d.lua_state=clone.run(s.lua_state);
    d.current_state=clone.mapped(s.current_state);
    for(const auto& [key,value]:s.coroutines)
        d.coroutines.emplace(key,std::make_pair(clone.mapped(value.first),value.second));
    for(const auto& [value,type]:s.params) {
        auto next=value;
        switch(type) {
        case LuaParam::CARD: case LuaParam::GROUP: case LuaParam::EFFECT:
        case LuaParam::DELETED: next.ptr=objects.at(value.ptr); break;
        case LuaParam::STRING:
            throw std::runtime_error("pending borrowed string parameter");
        default: break;
        }
        d.params.emplace_back(next,type);
    }
    d.no_action=s.no_action;
    d.call_depth=s.call_depth;
    d.weak_lua_references=s.weak_lua_references;
    d.loaded_scripts_table=s.loaded_scripts_table;
    d.deleted.ref_handle=s.deleted.ref_handle;
    d.deleted.weak_ref_handle=s.deleted.weak_ref_handle;
    d.deleted.num_ref=s.deleted.num_ref;
}
