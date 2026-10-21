// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::Adjust& d, const P::Adjust& s) {
    copy(d.step, s.step);
}

void copy(P::Turn& d, const P::Turn& s) {
    copy(d.step, s.step);
    copy(d.turn_player, s.turn_player);
    copy(d.has_performed_second_battle_phase, s.has_performed_second_battle_phase);
}

void copy(P::RefreshLoc& d, const P::RefreshLoc& s) {
    copy(d.step, s.step);
    copy(d.dis_count, s.dis_count);
    copy(d.previously_disabled_locations, s.previously_disabled_locations);
    copy(d.current_disable_field_effect, s.current_disable_field_effect);
}

void copy(P::Startup& d, const P::Startup& s) {
    copy(d.step, s.step);
}

void copy(P::QuickEffect& d, const P::QuickEffect& s) {
    copy(d.step, s.step);
    copy(d.skip_freechain, s.skip_freechain);
    copy(d.is_opponent, s.is_opponent);
    copy(d.priority_player, s.priority_player);
}

void copy(P::IdleCommand& d, const P::IdleCommand& s) {
    copy(d.step, s.step);
    copy(d.phase_to_change_to, s.phase_to_change_to);
    copy(d.card_to_reposition, s.card_to_reposition);
}

void copy(P::PhaseEvent& d, const P::PhaseEvent& s) {
    copy(d.step, s.step);
    copy(d.phase, s.phase);
    copy(d.is_opponent, s.is_opponent);
    copy(d.priority_passed, s.priority_passed);
}

void copy(P::PointEvent& d, const P::PointEvent& s) {
    copy(d.step, s.step);
    copy(d.skip_trigger, s.skip_trigger);
    copy(d.skip_freechain, s.skip_freechain);
    copy(d.skip_new, s.skip_new);
}

void copy(P::AddChain& d, const P::AddChain& s) {
    copy(d.step, s.step);
    copy(d.is_activated_effect, s.is_activated_effect);
}

void copy(P::SolveChain& d, const P::SolveChain& s) {
    copy(d.step, s.step);
    copy(d.skip_trigger, s.skip_trigger);
    copy(d.skip_freechain, s.skip_freechain);
    copy(d.skip_new, s.skip_new);
    copy(d.backed_up_operation, s.backed_up_operation);
}

void copy(P::SolveContinuous& d, const P::SolveContinuous& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::ExecuteCost& d, const P::ExecuteCost& s) {
    copy(d.step, s.step);
    copy(d.triggering_player, s.triggering_player);
    copy(d.shuffle_check_was_disabled, s.shuffle_check_was_disabled);
    copy(d.triggering_effect, s.triggering_effect);
}

void copy(P::ExecuteOperation& d, const P::ExecuteOperation& s) {
    copy(d.step, s.step);
    copy(d.triggering_player, s.triggering_player);
    copy(d.shuffle_check_was_disabled, s.shuffle_check_was_disabled);
    copy(d.triggering_effect, s.triggering_effect);
}

void copy(P::ExecuteTarget& d, const P::ExecuteTarget& s) {
    copy(d.step, s.step);
    copy(d.triggering_player, s.triggering_player);
    copy(d.shuffle_check_was_disabled, s.shuffle_check_was_disabled);
    copy(d.triggering_effect, s.triggering_effect);
}

void copy(P::ActivateEffect& d, const P::ActivateEffect& s) {
    copy(d.step, s.step);
    copy(d.peffect, s.peffect);
}

void copy(P::RefreshRelay& d, const P::RefreshRelay& s) {
    copy(d.step, s.step);
}
