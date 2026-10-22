// SPDX-License-Identifier: AGPL-3.0-or-later
#include "lfunc.h"
#include "ltable.h"
#include "vm.h"
namespace ocg::persistence {
void VmArchive::rehash() {
    auto *state = G(vm_.lua_state);
    std::vector<Table *> tables;
    for (auto *first : {state->allgc, state->finobj, state->tobefnz, state->fixedgc})
        for (auto *item = first; item; item = item->next)
            if (item->tt == LUA_VTABLE)
                tables.push_back(gco2t(item));
    for (auto *table : tables) {
        if (isdummy(table))
            continue;
        bool needed = false;
        for (unsigned i = 0; i < sizenode(table); ++i) {
            auto *node = &table->node[i];
            if (!isempty(gval(node)) &&
                ((keyiscollectable(node) && keytt(node) != LUA_VSHRSTR &&
                  keytt(node) != LUA_VLNGSTR) ||
                 keytt(node) == LUA_VLIGHTUSERDATA || keytt(node) == LUA_VLCF))
                needed = true;
        }
        if (needed) {
            auto *L = vm_.lua_state;
            if (!lua_checkstack(L, 2))
                throw std::bad_alloc();
            const int top = lua_gettop(L);
            lua_pushcfunction(
                L, +[](lua_State *L) -> int {
                    auto *table = static_cast<Table *>(lua_touserdata(L, 1));
                    luaH_resize(L, table, luaH_realasize(table), sizenode(table));
                    return 0;
                });
            lua_pushlightuserdata(L, table);
            const int status = lua_pcall(L, 1, 0, 0);
            lua_settop(L, top);
            if (status == LUA_ERRMEM)
                throw std::bad_alloc();
            if (status != LUA_OK)
                throw std::invalid_argument("snapshot table rehash failed");
        }
    }
}
void VmArchive::value(TValue &v) {
    if (iscollectable(&v))
        slot(v.value_.gc);
    else if (ttislightuserdata(&v))
        slot(v.value_.p);
    else if (ttislcf(&v))
        slot(v.value_.f);
}
void VmArchive::global(global_State &g) {
    slot(g.frealloc);
    slot(g.ud);
    slot(g.panic);
    slot(g.warnf);
    slot(g.ud_warn);
    slot(g.strt.hash);
    for (int i = 0; i < g.strt.size; ++i)
        slot(g.strt.hash[i]);
    value(g.l_registry);
    value(g.nilvalue);
    slot(g.allgc);
    slot(g.sweepgc);
    slot(g.finobj);
    slot(g.gray);
    slot(g.grayagain);
    slot(g.weak);
    slot(g.ephemeron);
    slot(g.allweak);
    slot(g.tobefnz);
    slot(g.fixedgc);
    slot(g.survival);
    slot(g.old1);
    slot(g.reallyold);
    slot(g.firstold1);
    slot(g.finobjsur);
    slot(g.finobjold1);
    slot(g.finobjrold);
    slot(g.twups);
    slot(g.mainthread);
    slot(g.memerrmsg);
    for (auto &p : g.tmname)
        slot(p);
    for (auto &p : g.mt)
        slot(p);
    for (auto &row : g.strcache)
        for (auto &p : row)
            slot(p);
}
void VmArchive::thread(lua_State &L) {
    if (L.errorJmp || L.hook)
        throw std::invalid_argument("snapshot contains an executing or hooked Lua thread");
    slot(L.next);
    slot(L.top.p);
    slot(L.l_G);
    slot(L.ci);
    slot(L.stack_last.p);
    slot(L.stack.p);
    slot(L.openupval);
    slot(L.tbclist.p);
    slot(L.twups);
    slot(L.gclist);
    std::uintptr_t payload;
    std::memcpy(&payload, lua_getextraspace(&L), sizeof(payload));
    slot(lua_getextraspace(&L), payload);
    for (auto *s = L.stack.p; s && s < L.top.p; ++s)
        value(s->val);
    bool active = true;
    for (auto *frame = &L.base_ci; frame; frame = frame->next) {
        if (active) {
            slot(frame->func.p);
            slot(frame->top.p);
            if (isLua(frame))
                slot(frame->u.l.savedpc);
            else {
                slot(frame->u.c.k);
                if (frame->u.c.k && (frame->u.c.ctx < 0 || frame->u.c.ctx > 2))
                    throw std::invalid_argument("unsupported snapshot C continuation context");
            }
        }
        slot(frame->previous);
        slot(frame->next);
        if (frame == L.ci)
            active = false;
    }
}
void VmArchive::table(Table &t) {
    slot(t.metatable);
    slot(t.array);
    slot(t.node);
    slot(t.lastfree);
    slot(t.gclist);
    for (unsigned i = 0; i < luaH_realasize(&t); ++i)
        value(t.array[i]);
    if (isdummy(&t))
        return;
    for (unsigned i = 0; i < sizenode(&t); ++i) {
        auto &node = t.node[i];
        if (keyiscollectable(&node))
            slot(node.u.key_val.gc);
        else if (keytt(&node) == LUA_VLIGHTUSERDATA)
            slot(node.u.key_val.p);
        else if (keytt(&node) == LUA_VLCF)
            slot(node.u.key_val.f);
        value(node.i_val);
    }
}
void VmArchive::proto(Proto &p) {
    slot(p.k);
    slot(p.code);
    slot(p.p);
    slot(p.upvalues);
    slot(p.lineinfo);
    slot(p.abslineinfo);
    slot(p.locvars);
    slot(p.source);
    slot(p.gclist);
    for (int i = 0; i < p.sizek; ++i)
        value(p.k[i]);
    for (int i = 0; i < p.sizep; ++i)
        slot(p.p[i]);
    for (int i = 0; i < p.sizeupvalues; ++i)
        slot(p.upvalues[i].name);
    for (int i = 0; i < p.sizelocvars; ++i)
        slot(p.locvars[i].varname);
}
void VmArchive::object(GCObject &item) {
    slot(item.next);
    switch (item.tt) {
    case LUA_VSHRSTR:
        slot(gco2ts(&item)->u.hnext);
        break;
    case LUA_VLNGSTR:
        break;
    case LUA_VTABLE:
        table(*gco2t(&item));
        break;
    case LUA_VPROTO:
        proto(*gco2p(&item));
        break;
    case LUA_VTHREAD:
        thread(*gco2th(&item));
        break;
    case LUA_VLCL: {
        auto &c = *gco2lcl(&item);
        slot(c.p);
        slot(c.gclist);
        for (int i = 0; i < c.nupvalues; ++i)
            slot(c.upvals[i]);
        break;
    }
    case LUA_VCCL: {
        auto &c = *gco2ccl(&item);
        slot(c.f);
        slot(c.gclist);
        for (int i = 0; i < c.nupvalues; ++i)
            value(c.upvalue[i]);
        break;
    }
    case LUA_VUPVAL: {
        auto &u = *gco2upv(&item);
        slot(u.v.p);
        if (upisopen(&u)) {
            slot(u.u.open.next);
            slot(u.u.open.previous);
        } else
            value(u.u.value);
        break;
    }
    case LUA_VUSERDATA: {
        auto &u = *gco2u(&item);
        slot(u.metatable);
        if (u.nuvalue)
            slot(u.gclist);
        for (int i = 0; i < u.nuvalue; ++i)
            value(u.uv[i].uv);
        if (u.len == sizeof(lua_obj *)) {
            std::uintptr_t payload;
            std::memcpy(&payload, getudatamem(&u), sizeof(payload));
            slot(getudatamem(&u), payload);
        } else if (u.len != 32 || u.nuvalue || u.metatable)
            throw std::invalid_argument("unsupported snapshot userdata");
        break;
    }
    default:
        throw std::invalid_argument("unknown snapshot Lua object");
    }
}
} // namespace ocg::persistence
