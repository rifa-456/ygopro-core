// SPDX-License-Identifier: AGPL-3.0-or-later
#include "vm.h"
#include "../profiling/metrics.h"
#include <cstring>
#include <stdexcept>
#include "lfunc.h"
#include "lgc.h"
#include "ltable.h"

namespace ocg::lua_clone {
namespace {
GCObject** gray_link(GCObject* object) {
    switch(object->tt) {
    case LUA_VTABLE: return &gco2t(object)->gclist;
    case LUA_VLCL: return &gco2lcl(object)->gclist;
    case LUA_VCCL: return &gco2ccl(object)->gclist;
    case LUA_VPROTO: return &gco2p(object)->gclist;
    case LUA_VTHREAD: return &gco2th(object)->gclist;
    case LUA_VUSERDATA:
        if(gco2u(object)->nuvalue) return &gco2u(object)->gclist;
        [[fallthrough]];
    default: throw std::runtime_error("unexpected gray object");
    }
}
}

Clone::Clone(const Heap& source, Heap& destination, Hooks hooks)
    : source_(source), destination_(destination), hooks_(std::move(hooks)) {}

void* Clone::translate(const void* pointer) const {
    if(!pointer) return nullptr;
    auto address = reinterpret_cast<std::uintptr_t>(pointer);
    OCG_PROFILE_COUNT(OCG_PROFILE_LUA_TRANSLATIONS, 1);
    auto next = blocks_.upper_bound(address);
    if(next == blocks_.begin()) {
        if(dead_) return nullptr;
        throw std::runtime_error("unmapped Lua pointer");
    }
    const auto& [base, block] = *--next;
    if(address - base > block.size) {
        if(dead_) return nullptr;
        throw std::runtime_error("Lua pointer outside tracked allocation");
    }
    return reinterpret_cast<void*>(block.destination + address - base);
}

void* Clone::external(const void* pointer) const {
    OCG_PROFILE_COUNT(OCG_PROFILE_LUA_EXTERNAL_LOOKUPS, pointer ? 1 : 0);
    if(!pointer) return nullptr;
    auto address = reinterpret_cast<std::uintptr_t>(pointer);
    auto next = blocks_.upper_bound(address);
    if(next != blocks_.begin()) {
        const auto& [base, block] = *--next;
        if(address - base <= block.size) return translate(pointer);
    }
    return hooks_.external ? hooks_.external(pointer) : const_cast<void*>(pointer);
}

void Clone::collect(GCObject* first) {
    for(auto* object = first; object; object = object->next) {
        if(!objects_.insert(object).second) throw std::runtime_error("overlapping GC ownership lists");
    }
}

void Clone::collect_gray(GCObject* first) {
    std::unordered_set<GCObject*> seen;
    for(auto* object = first; object; object = *gray_link(object)) {
        if(!seen.insert(object).second) throw std::runtime_error("cyclic gray list");
        if(objects_.find(object) == objects_.end()) throw std::runtime_error("unowned gray object");
        gray_.insert(object);
    }
}

void Clone::fix_value(TValue& destination, const TValue& source) {
    if(iscollectable(&source)) destination.value_.gc = mapped(source.value_.gc);
    else if(ttislightuserdata(&source)) destination.value_.p = external(source.value_.p);
}

void Clone::fix_global(global_State* source) {
    auto* result = mapped(source);
    result->ud = &destination_;
    result->strt.hash = mapped(source->strt.hash);
    for(int i = 0; i < source->strt.size; ++i)
        result->strt.hash[i] = mapped(source->strt.hash[i]);
    fix_value(result->l_registry, source->l_registry);
    fix_value(result->nilvalue, source->nilvalue);
    result->allgc = mapped(source->allgc);
    result->sweepgc = mapped(source->sweepgc);
    result->finobj = mapped(source->finobj);
    result->gray = keepinvariant(source) ? mapped(source->gray) : nullptr;
    result->grayagain = keepinvariant(source) ? mapped(source->grayagain) : nullptr;
    result->weak = keepinvariant(source) ? mapped(source->weak) : nullptr;
    result->ephemeron = keepinvariant(source) ? mapped(source->ephemeron) : nullptr;
    result->allweak = keepinvariant(source) ? mapped(source->allweak) : nullptr;
    result->tobefnz = mapped(source->tobefnz);
    result->fixedgc = mapped(source->fixedgc);
    result->survival = mapped(source->survival);
    result->old1 = mapped(source->old1);
    result->reallyold = mapped(source->reallyold);
    result->firstold1 = mapped(source->firstold1);
    result->finobjsur = mapped(source->finobjsur);
    result->finobjold1 = mapped(source->finobjold1);
    result->finobjrold = mapped(source->finobjrold);
    result->twups = mapped(source->twups);
    result->mainthread = mapped(source->mainthread);
    result->memerrmsg = mapped(source->memerrmsg);
    for(int i = 0; i < TM_N; ++i) result->tmname[i] = mapped(source->tmname[i]);
    for(int i = 0; i < LUA_NUMTYPES; ++i) result->mt[i] = mapped(source->mt[i]);
    for(int i = 0; i < STRCACHE_N; ++i)
        for(int j = 0; j < STRCACHE_M; ++j)
            result->strcache[i][j] = mapped(source->strcache[i][j]);
    result->ud_warn = external(source->ud_warn);
}

void Clone::fix_table(Table* source) {
    auto* result = mapped(source);
    result->array = mapped(source->array);
    result->metatable = mapped(source->metatable);
    for(unsigned i = 0; i < luaH_realasize(source); ++i)
        fix_value(result->array[i], source->array[i]);
    if(isdummy(source)) return; // Lua's immutable shared dummy node.
    result->node = mapped(source->node);
    result->lastfree = mapped(source->lastfree);
    for(unsigned i = 0; i < sizenode(source); ++i) {
        Node& old = source->node[i];
        Node& next = result->node[i];
        if(keyiscollectable(&old)) next.u.key_val.gc = mapped(old.u.key_val.gc);
        else if(keytt(&old) == LUA_VLIGHTUSERDATA) {
            void* relocated = external(old.u.key_val.p);
            if(relocated != old.u.key_val.p)
                throw std::runtime_error("relocated lightuserdata table keys require a host key-identity policy");
        }
        fix_value(next.i_val, old.i_val);
    }
}

void Clone::fix_proto(Proto* source) {
    auto* result = mapped(source);
    result->k = mapped(source->k);
    result->code = mapped(source->code);
    result->p = mapped(source->p);
    result->upvalues = mapped(source->upvalues);
    result->lineinfo = mapped(source->lineinfo);
    result->abslineinfo = mapped(source->abslineinfo);
    result->locvars = mapped(source->locvars);
    result->source = mapped(source->source);
    for(int i = 0; i < source->sizek; ++i) fix_value(result->k[i], source->k[i]);
    for(int i = 0; i < source->sizep; ++i) result->p[i] = mapped(source->p[i]);
    for(int i = 0; i < source->sizeupvalues; ++i)
        result->upvalues[i].name = mapped(source->upvalues[i].name);
    for(int i = 0; i < source->sizelocvars; ++i)
        result->locvars[i].varname = mapped(source->locvars[i].varname);
}

void Clone::fix_thread(lua_State* source) {
    if(source->errorJmp) throw std::runtime_error("cannot clone an executing Lua thread");
    auto* result = mapped(source);
    result->top.p = mapped(source->top.p);
    result->l_G = mapped(source->l_G);
    result->ci = mapped(source->ci);
    result->stack_last.p = mapped(source->stack_last.p);
    result->stack.p = mapped(source->stack.p);
    result->openupval = mapped(source->openupval);
    result->tbclist.p = mapped(source->tbclist.p);
    result->twups = mapped(source->twups);
    void* payload;
    std::memcpy(&payload, lua_getextraspace(source), sizeof(payload));
    payload = external(payload);
    std::memcpy(lua_getextraspace(result), &payload, sizeof(payload));
    if(source->stack.p) {
        auto size = source->stack_last.p - source->stack.p + EXTRA_STACK;
        for(ptrdiff_t i = 0; i < size; ++i) {
            if(source->stack.p + i < source->top.p)
                fix_value(result->stack.p[i].val, source->stack.p[i].val);
            else setnilvalue(&result->stack.p[i].val);
        }
    }
    bool active = true;
    for(auto* frame = &source->base_ci; frame; frame = frame->next) {
        auto* copied = mapped(frame);
        if(active) {
            copied->func.p = mapped(frame->func.p);
            copied->top.p = mapped(frame->top.p);
            if(isLua(frame)) copied->u.l.savedpc = mapped(frame->u.l.savedpc);
            else if(frame->u.c.k && hooks_.continuation)
                copied->u.c.ctx = hooks_.continuation(frame->u.c.k, frame->u.c.ctx);
        } else {
            // Inactive cached frames can contain pointers to already-freed stacks.
            std::memset(copied, 0, sizeof(*copied));
        }
        copied->previous = mapped(frame->previous);
        copied->next = mapped(frame->next);
        if(frame == source->ci) active = false;
    }
}

void Clone::fix_object(GCObject* source) {
    dead_ = false;
    auto* result = mapped(source);
    dead_ = issweepphase(global_) && isdead(global_, source);
    result->next = mapped(source->next);
    switch(source->tt) {
    case LUA_VSHRSTR:
        gco2ts(result)->u.hnext = mapped(gco2ts(source)->u.hnext);
        break;
    case LUA_VLNGSTR: break;
    case LUA_VTABLE: fix_table(gco2t(source)); break;
    case LUA_VPROTO: fix_proto(gco2p(source)); break;
    case LUA_VTHREAD: fix_thread(gco2th(source)); break;
    case LUA_VLCL: {
        auto* old = gco2lcl(source); auto* next = gco2lcl(result);
        next->p = mapped(old->p);
        for(int i = 0; i < old->nupvalues; ++i) next->upvals[i] = mapped(old->upvals[i]);
        break;
    }
    case LUA_VCCL: {
        auto* old = gco2ccl(source); auto* next = gco2ccl(result);
        for(int i = 0; i < old->nupvalues; ++i) fix_value(next->upvalue[i], old->upvalue[i]);
        break;
    }
    case LUA_VUPVAL: {
        auto* old = gco2upv(source); auto* next = gco2upv(result);
        next->v.p = mapped(old->v.p);
        if(upisopen(old)) {
            next->u.open.next = mapped(old->u.open.next);
            next->u.open.previous = mapped(old->u.open.previous);
        } else fix_value(next->u.value, old->u.value);
        break;
    }
    case LUA_VUSERDATA: {
        auto* old = gco2u(source); auto* next = gco2u(result);
        next->metatable = mapped(old->metatable);
        for(int i = 0; i < old->nuvalue; ++i) fix_value(next->uv[i].uv, old->uv[i].uv);
        if(hooks_.userdata) hooks_.userdata(old, next);
        break;
    }
    default: throw std::runtime_error("unknown GC object tag");
    }
    bool has_gray_link = source->tt == LUA_VTABLE || source->tt == LUA_VLCL ||
        source->tt == LUA_VCCL || source->tt == LUA_VPROTO || source->tt == LUA_VTHREAD ||
        (source->tt == LUA_VUSERDATA && gco2u(source)->nuvalue != 0);
    if(has_gray_link) *gray_link(result) = gray_.find(source) != gray_.end() ? mapped(*gray_link(source)) : nullptr;
    dead_ = false;
}

lua_State* Clone::run(lua_State* source) {
    if(!destination_.empty()) throw std::runtime_error("clone destination must be empty");
    if(source->l_G->frealloc != Heap::allocate || source->l_G->ud != &source_)
        throw std::runtime_error("source must use the tracked allocator");
    auto* global = G(source);
    global_ = global;
    {
    OCG_PROFILE_SCOPE(OCG_PROFILE_LUA_COLLECT);
    collect(global->allgc); collect(global->finobj);
    collect(global->tobefnz); collect(global->fixedgc);
    for(auto* object : objects_)
        if(object->tt == LUA_VTHREAD && gco2th(object)->errorJmp)
            throw std::runtime_error("clone boundary contains executing Lua thread");
    if(keepinvariant(global)) {
        collect_gray(global->gray); collect_gray(global->grayagain);
        collect_gray(global->weak); collect_gray(global->ephemeron); collect_gray(global->allweak);
    }
    OCG_PROFILE_COUNT(OCG_PROFILE_LUA_OBJECTS, objects_.size());
    }
    try {
        {
        OCG_PROFILE_SCOPE(OCG_PROFILE_LUA_COPY);
        for(const auto& [address, size] : source_.blocks()) {
            void* output = Heap::allocate(&destination_, nullptr, 0, size);
            if(!output) throw std::bad_alloc();
            OCG_PROFILE_COUNT(OCG_PROFILE_LUA_BLOCKS, 1);
            OCG_PROFILE_COUNT(OCG_PROFILE_LUA_BYTES, size);
            std::memcpy(output, reinterpret_cast<const void*>(address), size);
            blocks_.emplace(address, Block{size, reinterpret_cast<std::uintptr_t>(output)});
        }
        }
        {
        OCG_PROFILE_SCOPE(OCG_PROFILE_LUA_FIX);
        fix_global(global);
        for(auto* object : objects_) fix_object(object);
        }
        return mapped(source);
    } catch(...) {
        // Incomplete clones must never run Lua finalizers or touch source owners.
        destination_.release_all();
        throw;
    }
}
}
