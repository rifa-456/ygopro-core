// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::SelectBattleCmd& d, const P::SelectBattleCmd& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::SelectIdleCmd& d, const P::SelectIdleCmd& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::SelectEffectYesNo& d, const P::SelectEffectYesNo& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.pcard, s.pcard);
    copy(d.description, s.description);
}

void copy(P::SelectYesNo& d, const P::SelectYesNo& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.description, s.description);
}

void copy(P::SelectOption& d, const P::SelectOption& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::SelectCard& d, const P::SelectCard& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.min, s.min);
    copy(d.max, s.max);
}

void copy(P::SelectCardCodes& d, const P::SelectCardCodes& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.min, s.min);
    copy(d.max, s.max);
}

void copy(P::SelectUnselectCard& d, const P::SelectUnselectCard& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.min, s.min);
    copy(d.max, s.max);
    copy(d.finishable, s.finishable);
}

void copy(P::SelectChain& d, const P::SelectChain& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.spe_count, s.spe_count);
    copy(d.forced, s.forced);
}

void copy(P::SelectPlace& d, const P::SelectPlace& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.count, s.count);
    copy(d.flag, s.flag);
    copy(d.disable_field, s.disable_field);
}

void copy(P::SelectPosition& d, const P::SelectPosition& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.positions, s.positions);
    copy(d.code, s.code);
}

void copy(P::SelectTributeP& d, const P::SelectTributeP& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.min, s.min);
    copy(d.max, s.max);
}

void copy(P::SortChain& d, const P::SortChain& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::SelectCounter& d, const P::SelectCounter& s) {
    copy(d.step, s.step);
    copy(d.countertype, s.countertype);
    copy(d.count, s.count);
    copy(d.playerid, s.playerid);
    copy(d.self, s.self);
    copy(d.oppo, s.oppo);
}

void copy(P::SelectSum& d, const P::SelectSum& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.acc, s.acc);
    copy(d.min, s.min);
    copy(d.max, s.max);
}

void copy(P::SortCard& d, const P::SortCard& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.is_chain, s.is_chain);
}

void copy(P::SelectRelease& d, const P::SelectRelease& s) {
    copy(d.step, s.step);
    copy(d.min, s.min);
    copy(d.max, s.max);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.check_field, s.check_field);
    copy(d.toplayer, s.toplayer);
    copy(d.zone, s.zone);
    copy(d.to_check, s.to_check);
    copy(d.extra_release_nonsum_effect, s.extra_release_nonsum_effect);
    copy(d.must_choose_one, s.must_choose_one);
}

void copy(P::SelectTribute& d, const P::SelectTribute& s) {
    copy(d.step, s.step);
    copy(d.min, s.min);
    copy(d.max, s.max);
    copy(d.playerid, s.playerid);
    copy(d.cancelable, s.cancelable);
    copy(d.toplayer, s.toplayer);
    copy(d.zone, s.zone);
    copy(d.target, s.target);
    copy(d.extra_release_effect, s.extra_release_effect);
    copy(d.must_choose_one, s.must_choose_one);
}

void copy(P::SelectFusion& d, const P::SelectFusion& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.chkf, s.chkf);
    copy(d.fusion_materials, s.fusion_materials);
    copy(d.forced_materials, s.forced_materials);
    copy(d.pcard, s.pcard);
}
