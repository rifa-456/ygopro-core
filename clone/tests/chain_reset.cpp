// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include "card.h"
#include "duel.h"
#include "effect.h"
#include "field.h"
#include "ocgapi.h"

namespace {
void require(bool condition, const char* message) {
    if(!condition)
        throw std::runtime_error(message);
}

void check_chain_reset_order() {
    OCG_DuelOptions options{};
    options.seed[0] = 1;
    options.cardReader = [](void*, uint32_t, OCG_CardData* data) { *data = {}; };
    options.scriptReader = [](void*, OCG_Duel, const char*) { return 0; };
    OCG_Duel raw = nullptr;
    require(OCG_CreateDuel(&raw, &options) == OCG_DUEL_CREATION_SUCCESS,
            "cannot create chain reset fixture");
    std::unique_ptr<duel, decltype(&OCG_DestroyDuel)> owner(
        static_cast<duel*>(raw), OCG_DestroyDuel);
    auto& field = *owner->game_field;
    auto* handler = owner->new_card(0);
    handler->current.controler = 0;
    handler->current.location = LOCATION_HAND;
    for(uint32_t i = 0; i < 8; ++i) {
        auto* entry = owner->new_effect();
        entry->owner = handler;
        entry->type = EFFECT_TYPE_SINGLE;
        entry->code = EFFECT_UPDATE_ATTACK;
        entry->flag[0] = EFFECT_FLAG_CLIENT_HINT;
        entry->description = 100 + i;
        entry->reset_flag = RESET_CHAIN;
        handler->add_effect(entry);
    }

    // Force hash iteration to disagree with creation order on every platform.
    // Initial IDs are unique and independent of pointer addresses in real duels.
    std::vector<effect*> expected(field.effects.cheff.begin(), field.effects.cheff.end());
    for(size_t i = 0; i < expected.size(); ++i)
        expected[i]->initial_id = static_cast<uint32_t>(expected.size() - i);
    std::reverse(expected.begin(), expected.end());
    std::vector<uint64_t> descriptions;
    for(auto* entry : expected)
        descriptions.push_back(entry->description);
    uint32_t size = 0;
    OCG_DuelGetMessage(raw, &size);
    owner->buff.clear();
    field.reset_chain();

    require(field.effects.cheff.empty(), "chain reset left registered effects");
    require(handler->single_effect.empty(), "chain reset left card effects");
    const auto* messages = static_cast<const uint8_t*>(OCG_DuelGetMessage(raw, &size));
    require(size == descriptions.size() * 24, "wrong hint removal count");
    size_t index = 0;
    for(size_t offset = 0; offset < size; offset += 24) {
        uint32_t length = 0;
        std::memcpy(&length, messages + offset, sizeof(length));
        const auto* message = messages + offset + 4;
        require(length == 20 && message[0] == MSG_CARD_HINT
                && message[11] == CHINT_DESC_REMOVE, "unexpected reset message");
        uint64_t description = 0;
        std::memcpy(&description, message + 12, sizeof(description));
        require(description == descriptions[index++], "chain resets depend on pointer hash order");
    }
    owner->buff.clear();
    field.reset_chain();
    OCG_DuelGetMessage(raw, &size);
    require(size == 0, "chain reset repeated removals");
}
}

int main() {
    try {
        check_chain_reset_order();
        std::cout << "PASS deterministic chain reset hint order\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
