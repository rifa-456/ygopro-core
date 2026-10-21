// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::Draw& d, const P::Draw& s) {
    copy(d.step, s.step);
    copy(d.count, s.count);
    copy(d.reason_player, s.reason_player);
    copy(d.playerid, s.playerid);
    copy(d.reason, s.reason);
    copy(d.reason_effect, s.reason_effect);
    copy(d.drawn_set, s.drawn_set);
}

void copy(P::Damage& d, const P::Damage& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.playerid, s.playerid);
    copy(d.is_step, s.is_step);
    copy(d.is_reflected, s.is_reflected);
    copy(d.amount, s.amount);
    copy(d.reason, s.reason);
    copy(d.reason_card, s.reason_card);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::Recover& d, const P::Recover& s) {
    copy(d.step, s.step);
    copy(d.reason_player, s.reason_player);
    copy(d.playerid, s.playerid);
    copy(d.is_step, s.is_step);
    copy(d.amount, s.amount);
    copy(d.reason, s.reason);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::PayLPCost& d, const P::PayLPCost& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.cost, s.cost);
}

void copy(P::RemoveCounter& d, const P::RemoveCounter& s) {
    copy(d.step, s.step);
    copy(d.rplayer, s.rplayer);
    copy(d.self, s.self);
    copy(d.oppo, s.oppo);
    copy(d.countertype, s.countertype);
    copy(d.count, s.count);
    copy(d.reason, s.reason);
    copy(d.pcard, s.pcard);
}

void copy(P::DiscardHand& d, const P::DiscardHand& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.min, s.min);
    copy(d.max, s.max);
    copy(d.reason, s.reason);
}

void copy(P::DiscardDeck& d, const P::DiscardDeck& s) {
    copy(d.step, s.step);
    copy(d.count, s.count);
    copy(d.playerid, s.playerid);
    copy(d.reason, s.reason);
}

void copy(P::SortDeck& d, const P::SortDeck& s) {
    copy(d.step, s.step);
    copy(d.count, s.count);
    copy(d.sort_player, s.sort_player);
    copy(d.target_player, s.target_player);
    copy(d.bottom, s.bottom);
}

void copy(P::RemoveOverlay& d, const P::RemoveOverlay& s) {
    copy(d.step, s.step);
    copy(d.min, s.min);
    copy(d.max, s.max);
    copy(d.replaced_amount, s.replaced_amount);
    copy(d.has_used_overlay_remove_replace_effect, s.has_used_overlay_remove_replace_effect);
    copy(d.rplayer, s.rplayer);
    copy(d.self, s.self);
    copy(d.oppo, s.oppo);
    copy(d.reason, s.reason);
    copy(d.pgroup, s.pgroup);
}

void copy(P::XyzOverlay& d, const P::XyzOverlay& s) {
    copy(d.step, s.step);
    copy(d.send_materials_to_grave, s.send_materials_to_grave);
    copy(d.target, s.target);
    copy(d.materials, s.materials);
}
