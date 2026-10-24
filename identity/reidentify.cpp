// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../card.h"
#include "../duel.h"
#include "../effect.h"
#include "../field.h"
#include "../interpreter.h"

#include <new>
#include <vector>

namespace {

constexpr uint32_t hidden_locations = LOCATION_DECK | LOCATION_HAND | LOCATION_EXTRA | LOCATION_REMOVED |
    LOCATION_MZONE | LOCATION_SZONE;

bool hidden(const card& target) {
    if(!(target.current.location & hidden_locations))
        return false;
    if(target.current.location & (LOCATION_DECK | LOCATION_HAND))
        return true;
    return target.current.position & POS_FACEDOWN;
}

void bind_script(duel& pduel, card& target) {
    auto& lua = *pduel.lua;
    auto* L = lua.current_state;
    luaL_checkstack(L, 2, nullptr);
    lua_rawgeti(L, LUA_REGISTRYINDEX, target.ref_handle);
    const auto& data = target.data;
    if(data.alias && data.alias < data.code + 10 && data.code < data.alias + 10)
        lua.load_card_script(data.alias);
    else
        lua.load_card_script(data.code);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
}

void reset_script_fields(duel& pduel, card& target) {
    if(target.unique_function)
        luaL_unref(pduel.lua->current_state, LUA_REGISTRYINDEX, target.unique_function);
    target.unique_function = 0;
    target.unique_location = 0;
    target.unique_effect = nullptr;
    target.spsummon_code = 0;
}

void remove_initial_effects(card& target) {
    std::vector<effect*> initial;
    for(const auto& entry : target.indexer)
        if(entry.first->is_flag(EFFECT_FLAG_INITIAL) && entry.first->owner == &target)
            initial.push_back(entry.first);
    for(auto* peffect : initial)
        target.remove_effect(peffect);
}

void reidentify(duel& pduel, card& target, uint32_t code) {
    remove_initial_effects(target);
    reset_script_fields(pduel, target);
    target.data = pduel.read_card(code);
    target.data.code = code;
    bind_script(pduel, target);
    const bool forced = !(target.data.type & TYPE_NORMAL) || (target.data.type & TYPE_PENDULUM);
    target.set_status(STATUS_INITIALIZING, TRUE);
    pduel.lua->add_param<LuaParam::CARD>(&target);
    pduel.lua->call_card_function(&target, "initial_effect", 1, 0, forced);
    target.set_status(STATUS_INITIALIZING, FALSE);
}

}

int OCG_ReidentifyCard(OCG_Duel ocg_duel, uint8_t controller, uint32_t location, uint32_t sequence, uint32_t code) {
    if(!ocg_duel || controller > 1 || !code)
        return OCG_IDENTITY_INVALID_ARGUMENT;
    auto& pduel = *static_cast<duel*>(ocg_duel);
    if(pduel.has_ended || pduel.last_status != OCG_DUEL_STATUS_AWAITING)
        return OCG_IDENTITY_NOT_AWAITING;
    card* target = pduel.game_field->get_field_card(controller, location, sequence);
    if(!target)
        return OCG_IDENTITY_NOT_FOUND;
    if(!hidden(*target))
        return OCG_IDENTITY_NOT_HIDDEN;
    if(pduel.read_card(code).code != code)
        return OCG_IDENTITY_UNKNOWN_CODE;
    try {
        reidentify(pduel, *target, code);
    } catch(const std::bad_alloc&) {
        return OCG_IDENTITY_ALLOCATION_FAILED;
    }
    pduel.last_status = OCG_DUEL_STATUS_AWAITING;
    return OCG_IDENTITY_SUCCESS;
}
