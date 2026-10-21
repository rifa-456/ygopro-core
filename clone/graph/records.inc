// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(card_state& d, const card_state& s) {
    copy(d.code, s.code);
    copy(d.code2, s.code2);
    copy(d.setcodes, s.setcodes);
    copy(d.type, s.type);
    copy(d.level, s.level);
    copy(d.rank, s.rank);
    copy(d.link, s.link);
    copy(d.link_marker, s.link_marker);
    copy(d.lscale, s.lscale);
    copy(d.rscale, s.rscale);
    copy(d.attribute, s.attribute);
    copy(d.race, s.race);
    copy(d.attack, s.attack);
    copy(d.defense, s.defense);
    copy(d.base_attack, s.base_attack);
    copy(d.base_defense, s.base_defense);
    copy(d.controler, s.controler);
    copy(d.location, s.location);
    copy(d.sequence, s.sequence);
    copy(d.position, s.position);
    copy(d.reason, s.reason);
    copy(d.pzone, s.pzone);
    copy(d.reason_card, s.reason_card);
    copy(d.reason_player, s.reason_player);
    copy(d.reason_effect, s.reason_effect);
}

void copy(card::sendto_param_t& d, const card::sendto_param_t& s) {
    copy(d.playerid, s.playerid);
    copy(d.position, s.position);
    copy(d.location, s.location);
    copy(d.sequence, s.sequence);
}

void copy(card::summon_info& d, const card::summon_info& s) {
    copy(d.type, s.type);
    copy(d.player, s.player);
    copy(d.location, s.location);
    copy(d.sequence, s.sequence);
    copy(d.pzone, s.pzone);
}

void copy(tevent& d, const tevent& s) {
    copy(d.trigger_card, s.trigger_card);
    copy(d.event_cards, s.event_cards);
    copy(d.reason_effect, s.reason_effect);
    copy(d.event_code, s.event_code);
    copy(d.event_value, s.event_value);
    copy(d.reason, s.reason);
    copy(d.event_player, s.event_player);
    copy(d.reason_player, s.reason_player);
    copy(d.global_id, s.global_id);
}

void copy(optarget& d, const optarget& s) {
    copy(d.op_cards, s.op_cards);
    copy(d.op_count, s.op_count);
    copy(d.op_player, s.op_player);
    copy(d.op_param, s.op_param);
}

void copy(chain& d, const chain& s) {
    copy(d.triggering_state, s.triggering_state);
    copy(d.evt, s.evt);
    copy(d.chain_count, s.chain_count);
    copy(d.triggering_player, s.triggering_player);
    copy(d.triggering_controler, s.triggering_controler);
    copy(d.triggering_position, s.triggering_position);
    copy(d.target_player, s.target_player);
    copy(d.disable_player, s.disable_player);
    copy(d.triggering_summon_location, s.triggering_summon_location);
    copy(d.triggering_summon_proc_complete, s.triggering_summon_proc_complete);
    copy(d.was_just_sent, s.was_just_sent);
    copy(d.chain_id, s.chain_id);
    copy(d.triggering_location, s.triggering_location);
    copy(d.triggering_sequence, s.triggering_sequence);
    copy(d.triggering_status, s.triggering_status);
    copy(d.triggering_summon_type, s.triggering_summon_type);
    copy(d.replace_op, s.replace_op);
    copy(d.target_param, s.target_param);
    copy(d.flag, s.flag);
    copy(d.event_id, s.event_id);
    copy(d.triggering_effect, s.triggering_effect);
    copy(d.target_cards, s.target_cards);
    copy(d.disable_reason, s.disable_reason);
    copy(d.applied_chain_counters, s.applied_chain_counters);
    copy(d.opinfos, s.opinfos);
    copy(d.possibleopinfos, s.possibleopinfos);
}

void copy(player_info& d, const player_info& s) {
    copy(d.lp, s.lp);
    copy(d.start_lp, s.start_lp);
    copy(d.start_count, s.start_count);
    copy(d.draw_count, s.draw_count);
    copy(d.used_location, s.used_location);
    copy(d.disabled_location, s.disabled_location);
    copy(d.extra_p_count, s.extra_p_count);
    copy(d.exchanges, s.exchanges);
    copy(d.tag_index, s.tag_index);
    copy(d.recharge, s.recharge);
    copy(d.list_mzone, s.list_mzone);
    copy(d.list_szone, s.list_szone);
    copy(d.list_main, s.list_main);
    copy(d.list_grave, s.list_grave);
    copy(d.list_hand, s.list_hand);
    copy(d.list_remove, s.list_remove);
    copy(d.list_extra, s.list_extra);
    copy(d.extra_lists_main, s.extra_lists_main);
    copy(d.extra_lists_hand, s.extra_lists_hand);
    copy(d.extra_lists_extra, s.extra_lists_extra);
    copy(d.extra_extra_p_count, s.extra_extra_p_count);
}

void copy(field_info& d, const field_info& s) {
    copy(d.event_id, s.event_id);
    copy(d.field_id, s.field_id);
    copy(d.copy_id, s.copy_id);
    copy(d.turn_id, s.turn_id);
    copy(d.turn_id_by_player, s.turn_id_by_player);
    copy(d.card_id, s.card_id);
    copy(d.phase, s.phase);
    copy(d.turn_player, s.turn_player);
    copy(d.priorities, s.priorities);
    copy(d.can_shuffle, s.can_shuffle);
}

void copy(processor::chain_limit_t& d, const processor::chain_limit_t& s) {
    copy(d.function, s.function);
    copy(d.player, s.player);
}

void copy(processor::action_value_t& d, const processor::action_value_t& s) {
    copy(d.check_function, s.check_function);
    copy(d.player_amount, s.player_amount);
}
