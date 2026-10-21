// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::SummonRule& d, const P::SummonRule& s) {
    copy(d.step, s.step);
    copy(d.sumplayer, s.sumplayer);
    copy(d.min_tribute, s.min_tribute);
    copy(d.max_allowed_tributes, s.max_allowed_tributes);
    copy(d.ignore_count, s.ignore_count);
    copy(d.zone, s.zone);
    copy(d.target, s.target);
    copy(d.summon_procedure_effect, s.summon_procedure_effect);
    copy(d.extra_summon_effect, s.extra_summon_effect);
    copy(d.tributes, s.tributes);
    copy(d.summon_cost_effects, s.summon_cost_effects);
}

void copy(P::SpSummonRule& d, const P::SpSummonRule& s) {
    copy(d.step, s.step);
    copy(d.sumplayer, s.sumplayer);
    copy(d.is_mid_chain, s.is_mid_chain);
    copy(d.summon_type, s.summon_type);
    copy(d.target, s.target);
    copy(d.summon_proc_effect, s.summon_proc_effect);
    copy(d.cards_to_summon_g, s.cards_to_summon_g);
    copy(d.spsummon_cost_effects, s.spsummon_cost_effects);
}

void copy(P::SpSummon& d, const P::SpSummon& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.zone, s.zone);
    copy(d.reason_effect, s.reason_effect);
    copy(d.targets, s.targets);
}

void copy(P::FlipSummon& d, const P::FlipSummon& s) {
    copy(d.step, s.step);
    copy(d.sumplayer, s.sumplayer);
    copy(d.target, s.target);
    copy(d.flip_summon_cost_effects, s.flip_summon_cost_effects);
}

void copy(P::MonsterSet& d, const P::MonsterSet& s) {
    copy(d.step, s.step);
    copy(d.setplayer, s.setplayer);
    copy(d.min_tribute, s.min_tribute);
    copy(d.max_allowed_tributes, s.max_allowed_tributes);
    copy(d.ignore_count, s.ignore_count);
    copy(d.zone, s.zone);
    copy(d.target, s.target);
    copy(d.summon_procedure_effect, s.summon_procedure_effect);
    copy(d.extra_summon_effect, s.extra_summon_effect);
    copy(d.tributes, s.tributes);
}

void copy(P::SpellSet& d, const P::SpellSet& s) {
    copy(d.step, s.step);
    copy(d.setplayer, s.setplayer);
    copy(d.toplayer, s.toplayer);
    copy(d.target, s.target);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::SpSummonStep& d, const P::SpSummonStep& s) {
    copy(d.step, s.step);
    copy(d.zone, s.zone);
    copy(d.targets, s.targets);
    copy(d.target, s.target);
    copy(d.spsummon_cost_effects, s.spsummon_cost_effects);
}

void copy(P::SpellSetGroup& d, const P::SpellSetGroup& s) {
    copy(d.step, s.step);
    copy(d.setplayer, s.setplayer);
    copy(d.toplayer, s.toplayer);
    copy(d.confirm, s.confirm);
    copy(d.ptarget, s.ptarget);
    copy(d.reason_effect, s.reason_effect);
    copy(d.set_cards, s.set_cards);
}

void copy(P::SpSummonRuleGroup& d, const P::SpSummonRuleGroup& s) {
    copy(d.step, s.step);
    copy(d.sumplayer, s.sumplayer);
    copy(d.summon_type, s.summon_type);
}
