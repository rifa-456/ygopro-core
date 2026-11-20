// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cstdint>

#include "ocgapi_constants.h"

/** The Damage Step of seven timings, as it was played between the one of six and today's of five (YGO-Gym's fork).

    Its timings are the start of the Damage Step, the flip of a face-down attack target, before damage calculation,
    damage calculation, after damage calculation, the resolution of effects and the end of the Damage Step. Against
    today's it differs in two ways. After damage calculation is a timing of its own, where the effects that battle
    damage triggers activate, ahead of the Flip Effects and the effects of "after damage calculation", which activate
    together in the next. And from damage calculation on, each timing holds one chain a player starts: once it has
    resolved, only an effect it triggered starts another. Before damage calculation the players may start any number,
    as today. The Damage Step of six timings, DUEL_6_STEP_BATLLE_STEP with DUEL_SINGLE_CHAIN_IN_DAMAGE_SUBSTEP, has the
    same timing after damage calculation, but joins the start with before damage calculation and holds one chain in
    every timing. */
namespace damage_step_timings {

/** Whether after damage calculation is a timing of its own, ahead of the Flip Effects. */
constexpr bool has_one_after_calculation(uint64_t flags) {
	return (flags & (DUEL_6_STEP_BATLLE_STEP | DUEL_7_TIMING_DAMAGE_STEP)) != 0;
}

/** Whether each timing from damage calculation on holds one chain a player starts. */
constexpr bool hold_one_chain_from_calculation(uint64_t flags) {
	return (flags & (DUEL_SINGLE_CHAIN_IN_DAMAGE_SUBSTEP | DUEL_7_TIMING_DAMAGE_STEP)) != 0;
}

}
