// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "graph/copy.h"

#include <random>

void duel_clone::reseed_core(duel& branch, uint64_t seed) {
    std::mt19937_64 generator(seed);
    RNG::Xoshiro256StarStar::StateType state;
    for(auto& word : state)
        word = generator();
    if(state == RNG::Xoshiro256StarStar::StateType{})
        state[0] = 1;
    branch.random = RNG::Xoshiro256StarStar(state);
}

int OCG_CloneDuelWithSeed(OCG_Duel* output, OCG_Duel source,
                        const OCG_CloneCallbacks* callbacks, uint64_t seed) {
    const int status = OCG_CloneDuel(output, source, callbacks);
    if(status != OCG_CLONE_SUCCESS)
        return status;
    try {
        OCG_PROFILE_SCOPE(OCG_PROFILE_CORE_RESEED);
        auto& branch = *static_cast<duel*>(*output);
        duel_clone::reseed(branch,seed);
        return OCG_CLONE_SUCCESS;
    } catch(const std::bad_alloc&) {
        OCG_DestroyDuel(*output);
        *output = nullptr;
        return OCG_CLONE_ALLOCATION_FAILED;
    } catch(...) {
        OCG_DestroyDuel(*output);
        *output = nullptr;
        return OCG_CLONE_UNSUPPORTED;
    }
}

void duel_clone::reseed(duel& branch,uint64_t seed) {
        auto* L = branch.lua->lua_state;
        if(!lua_checkstack(L, 3))
            throw std::bad_alloc();
        const int top = lua_gettop(L);
        lua_getfield(L, LUA_REGISTRYINDEX, "ocgcore.clone.randomseed");
        lua_pushinteger(L, static_cast<lua_Integer>(seed & 0xffffffffULL));
        lua_pushinteger(L, static_cast<lua_Integer>(seed >> 32));
        const int result = lua_pcall(L, 2, 0, 0);
        lua_settop(L, top);
        if(result != LUA_OK) {
            if(result==LUA_ERRMEM)throw std::bad_alloc();
            throw std::runtime_error("could not reseed Lua random streams");
        }
        duel_clone::reseed_core(branch, seed);
}
