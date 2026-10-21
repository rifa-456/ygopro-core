// SPDX-License-Identifier: AGPL-3.0-or-later
#include "copy.h"

void duel_clone::copy_field() {
    auto& d=*destination.game_field;
    const auto& s=*source.game_field;
    copy(d.player,s.player);
    copy(d.temp_card,s.temp_card);
    copy(d.infos,s.infos);
    copy(d.core,s.core);
    copy(d.nil_event,s.nil_event);
    d.returns=s.returns;
    d.return_cards.canceled=s.return_cards.canceled;
    copy(d.return_cards.list,s.return_cards.list);
    d.return_card_codes=s.return_card_codes;
    copy(d.effects.aura_effect,s.effects.aura_effect);
    copy(d.effects.ignition_effect,s.effects.ignition_effect);
    copy(d.effects.activate_effect,s.effects.activate_effect);
    copy(d.effects.trigger_o_effect,s.effects.trigger_o_effect);
    copy(d.effects.trigger_f_effect,s.effects.trigger_f_effect);
    copy(d.effects.quick_o_effect,s.effects.quick_o_effect);
    copy(d.effects.quick_f_effect,s.effects.quick_f_effect);
    copy(d.effects.continuous_effect,s.effects.continuous_effect);
    copy(d.effects.oath,s.effects.oath);
    copy(d.effects.pheff,s.effects.pheff);
    copy(d.effects.cheff,s.effects.cheff);
    copy(d.effects.rechargeable,s.effects.rechargeable);
    copy(d.effects.spsummon_count_eff,s.effects.spsummon_count_eff);
    copy(d.effects.disable_check_set,s.effects.disable_check_set);
    for(auto* collection : {&d.effects.aura_effect,&d.effects.ignition_effect,&d.effects.activate_effect,&d.effects.trigger_o_effect,&d.effects.trigger_f_effect,&d.effects.quick_o_effect,&d.effects.quick_f_effect,&d.effects.continuous_effect})
        for(auto it=collection->begin();it!=collection->end();++it) d.effects.indexer.emplace(it->second,it);
    for(const auto& [key,value]:s.effects.grant_effect.unsorted) {
        field_effect::gain_effects gains; copy(gains,value);
        d.effects.grant_effect.emplace(mapped(key),std::move(gains));
    }
}
