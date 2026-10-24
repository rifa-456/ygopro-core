// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

class duel;
class effect;

namespace ocg::graph {

void release_effect(duel& owner, effect* peffect);

} // namespace ocg::graph
