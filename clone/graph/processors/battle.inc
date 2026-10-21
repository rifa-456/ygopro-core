// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::BattleCommand& d, const P::BattleCommand& s) {
    copy(d.step, s.step);
    copy(d.phase_to_change_to, s.phase_to_change_to);
    copy(d.forced_attack, s.forced_attack);
    copy(d.forced_attack_done, s.forced_attack_done);
    copy(d.is_replaying_attack, s.is_replaying_attack);
    copy(d.attack_announce_failed, s.attack_announce_failed);
    copy(d.repeat_battle_phase, s.repeat_battle_phase);
    copy(d.second_battle_phase_is_optional, s.second_battle_phase_is_optional);
    copy(d.previous_point_event_had_any_trigger_to_resolve, s.previous_point_event_had_any_trigger_to_resolve);
    copy(d.reason_player, s.reason_player);
    copy(d.damage_change_effect, s.damage_change_effect);
    copy(d.cards_destroyed_by_battle, s.cards_destroyed_by_battle);
    copy(d.reason_card, s.reason_card);
    copy(d.must_attack_map, s.must_attack_map);
}

void copy(P::DamageStep& d, const P::DamageStep& s) {
    copy(d.step, s.step);
    copy(d.backup_phase, s.backup_phase);
    copy(d.new_attack, s.new_attack);
    copy(d.attacker, s.attacker);
    copy(d.attack_target, s.attack_target);
    copy(d.cards_destroyed_by_battle, s.cards_destroyed_by_battle);
}

void copy(P::ForcedBattle& d, const P::ForcedBattle& s) {
    copy(d.step, s.step);
    copy(d.backup_phase, s.backup_phase);
}

void copy(P::AttackDisable& d, const P::AttackDisable& s) {
    copy(d.step, s.step);
}
