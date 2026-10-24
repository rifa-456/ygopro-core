// SPDX-License-Identifier: AGPL-3.0-or-later
#include "references.h"

#include <algorithm>
#include "../../card.h"
#include "../../duel.h"
#include "../../field.h"

namespace ocg::graph {

void release_effect(duel& owner, effect* peffect) {
    for(auto* pcard : owner.cards)
        for(auto* state : {&pcard->current, &pcard->previous, &pcard->temp})
            if(state->reason_effect == peffect)
                state->reason_effect = nullptr;
    auto& core = owner.game_field->core;
    std::replace(core.select_effects.begin(), core.select_effects.end(), peffect, static_cast<effect*>(nullptr));
    core.dec_count_reserve.erase(std::remove(core.dec_count_reserve.begin(), core.dec_count_reserve.end(), peffect),
        core.dec_count_reserve.end());
}

} // namespace ocg::graph
