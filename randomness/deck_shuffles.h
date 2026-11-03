// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cstdint>
#include <deque>
#include <vector>

class card;
class duel;

/** The cards each player's next deck shuffles put on top, as the host forces them (YGO-Gym's fork).

    A forced shuffle is still a shuffle: before its draws, the queued entry becomes the forced results that give
    the order wanted, so the shuffle is recorded and replays like any other. */
class duel_forced_shuffles {
  public:
    /** Before player's deck is shuffled, force the draws that put the next queued entry's cards on top. */
    void prepare(duel &owner, uint8_t player, const std::vector<card *> &deck);

    /** Per player, the codes each coming deck shuffle puts on top, first shuffle first, top card first. */
    std::deque<std::vector<uint32_t>> queued[2];
    /** Whether an entry asked for a card its deck did not hold; the queues are cleared when it happens. */
    bool refused = false;
};
