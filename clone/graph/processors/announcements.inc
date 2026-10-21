// SPDX-License-Identifier: AGPL-3.0-or-later
void copy(P::AnnounceRace& d, const P::AnnounceRace& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.count, s.count);
    copy(d.available, s.available);
}

void copy(P::AnnounceAttribute& d, const P::AnnounceAttribute& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.count, s.count);
    copy(d.available, s.available);
}

void copy(P::AnnounceCard& d, const P::AnnounceCard& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::AnnounceNumber& d, const P::AnnounceNumber& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
}

void copy(P::TossCoin& d, const P::TossCoin& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.reason_player, s.reason_player);
    copy(d.count, s.count);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::TossDice& d, const P::TossDice& s) {
    copy(d.step, s.step);
    copy(d.playerid, s.playerid);
    copy(d.reason_player, s.reason_player);
    copy(d.count1, s.count1);
    copy(d.count2, s.count2);
    copy(d.reason_effect, s.reason_effect);
}

void copy(P::RockPaperScissors& d, const P::RockPaperScissors& s) {
    copy(d.step, s.step);
    copy(d.repeat, s.repeat);
    copy(d.hand0, s.hand0);
}
