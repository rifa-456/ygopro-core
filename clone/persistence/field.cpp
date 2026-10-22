// SPDX-License-Identifier: AGPL-3.0-or-later
#include "archive.h"
void duel_archive::field_state() {
    auto &d = *duel_.game_field;
    io(d.player);
    io(d.temp_card);
    io(d.infos);
    io(d.core);
    io(d.nil_event);
    io(d.return_cards.list);
    io(d.effects.aura_effect);
    io(d.effects.ignition_effect);
    io(d.effects.activate_effect);
    io(d.effects.trigger_o_effect);
    io(d.effects.trigger_f_effect);
    io(d.effects.quick_o_effect);
    io(d.effects.quick_f_effect);
    io(d.effects.continuous_effect);
    io(d.effects.oath);
    io(d.effects.pheff);
    io(d.effects.cheff);
    io(d.effects.rechargeable);
    io(d.effects.spsummon_count_eff);
    io(d.effects.disable_check_set);
    io(d.returns.data);
    io(d.return_cards.canceled);
    io(d.return_card_codes.canceled);
    io(d.return_card_codes.list);
    if (stream_.reading)
        for (auto *collection :
             {&d.effects.aura_effect, &d.effects.ignition_effect, &d.effects.activate_effect,
              &d.effects.trigger_o_effect, &d.effects.trigger_f_effect, &d.effects.quick_o_effect,
              &d.effects.quick_f_effect, &d.effects.continuous_effect})
            for (auto it = collection->begin(); it != collection->end(); ++it)
                d.effects.indexer.emplace(it->second, it);
    auto n = stream_.count(d.effects.grant_effect.unsorted.size());
    if (stream_.reading) {
        for (std::uint64_t i = 0; i < n; ++i) {
            effect *key{};
            field_effect::gain_effects gains;
            io(key);
            io(gains);
            d.effects.grant_effect.emplace(key, std::move(gains));
        }
    } else
        for (auto &[key, value] : d.effects.grant_effect.unsorted) {
            auto mutable_key = key;
            io(mutable_key);
            io(value);
        }
}
