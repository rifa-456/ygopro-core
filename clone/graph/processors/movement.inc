// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::Destroy& d, const P::Destroy& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.reason, s.reason);
    copy(d.targets, s.targets);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::Release& d, const P::Release& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.reason, s.reason);
    copy(d.targets, s.targets);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::SendTo& d, const P::SendTo& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.reason, s.reason);
    copy(d.targets, s.targets);
    copy(d.reason_effect, s.reason_effect);
    copy(d.extra_args, s.extra_args);
}

void copy(P::DestroyReplace& d, const P::DestroyReplace& s) {
    copy(d.step, s.step);
    copy(d.battle, s.battle);
    copy(d.targets, s.targets);
    copy(d.target, s.target);
}

void copy(P::ReleaseReplace& d, const P::ReleaseReplace& s) {
    copy(d.step, s.step);
    copy(d.targets, s.targets);
    copy(d.target, s.target);
}

void copy(P::SendToReplace& d, const P::SendToReplace& s) {
    copy(d.step, s.step);
    copy(d.targets, s.targets);
    copy(d.target, s.target);
}

void copy(P::MoveToField& d, const P::MoveToField& s) {
    copy(d.step, s.step);
    copy(d.enable, s.enable);
    copy(d.ret, s.ret);
    copy(d.pzone, s.pzone);
    copy(d.zone, s.zone);
    copy(d.rule, s.rule);
    copy(d.location_reason, s.location_reason);
    copy(d.confirm, s.confirm);
    copy(d.target, s.target);
}

void copy(P::ChangePos& d, const P::ChangePos& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.enable, s.enable);
    copy(d.oppo_selection, s.oppo_selection);
    copy(d.reason_effect, s.reason_effect);
    copy(d.targets, s.targets);
    copy(d.to_grave_set, s.to_grave_set);
}

void copy(P::OperationReplace& d, const P::OperationReplace& s) {
    copy(d.step, s.step);
    copy(d.is_destroy, s.is_destroy);
    copy(d.replace_effect, s.replace_effect);
    copy(d.targets, s.targets);
    copy(d.target, s.target);
}

void copy(P::Equip& d, const P::Equip& s) {
    copy(d.step, s.step);
    copy(d.equip_player, s.equip_player);
    copy(d.is_step, s.is_step);
    copy(d.faceup, s.faceup);
    copy(d.equip_card, s.equip_card);
    copy(d.target, s.target);
}

void copy(P::GetControl& d, const P::GetControl& s) {
    copy(d.step, s.step);
    copy(d.chose_player, s.chose_player);
    copy(d.playerid, s.playerid);
    copy(d.reset_count, s.reset_count);
    copy(d.reset_phase, s.reset_phase);
    copy(d.zone, s.zone);
    copy(d.reason_effect, s.reason_effect);
    copy(d.targets, s.targets);
    copy(d.destroy_set, s.destroy_set);
}

void copy(P::SwapControl& d, const P::SwapControl& s) {
    copy(d.step, s.step);
    copy(d.reset_count, s.reset_count);
    copy(d.reason_player, s.reason_player);
    copy(d.self_selected_sequence, s.self_selected_sequence);
    copy(d.reset_phase, s.reset_phase);
    copy(d.reason_effect, s.reason_effect);
    copy(d.targets1, s.targets1);
    copy(d.targets2, s.targets2);
}

void copy(P::ControlAdjust& d, const P::ControlAdjust& s) {
    copy(d.step, s.step);
    copy(d.adjusting_player, s.adjusting_player);
    copy(d.destroy_set, s.destroy_set);
    copy(d.adjust_set, s.adjust_set);
}

void copy(P::SelfDestroyUnique& d, const P::SelfDestroyUnique& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.unique_card, s.unique_card);
}

void copy(P::SelfDestroy& d, const P::SelfDestroy& s) {
    copy(d.step, s.step);
}

void copy(P::SelfToGrave& d, const P::SelfToGrave& s) {
    copy(d.step, s.step);
}

void copy(P::TrapMonsterAdjust& d, const P::TrapMonsterAdjust& s) {
    copy(d.step, s.step);
    copy(d.oppo_selection, s.oppo_selection);
    copy(d.to_grave_set, s.to_grave_set);
}

void copy(P::SendTo::exargs& d, const P::SendTo::exargs& s) {
    copy(d.leave_field, s.leave_field);
    copy(d.leave_grave, s.leave_grave);
    copy(d.detach, s.detach);
    copy(d.check_decktop_visibility, s.check_decktop_visibility);
    copy(d.cv, s.cv);
    copy(d.predirect, s.predirect);
    d.cvit = d.cv.begin() + std::distance(s.cv.cbegin(), card_vector::const_iterator(s.cvit));
}
