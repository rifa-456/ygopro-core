// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cstdint>
#include <vector>

class field;

/** The cards each deck gives from its top, as the host forces them (YGO-Gym's fork).

    No player sees a deck's order until a card leaves its top or is shown, so every order that agrees with what was
    seen is the same game to both players, whatever the shuffles drew. The host queues, per player, the codes of the
    cards the deck gives from its top, in order. Before each read of a deck's top, every position read that holds no
    placed card takes the next code not placed yet, and the card with it moves there. A code is used up only when its
    card leaves the deck; when the deck is shuffled, or the card is moved away from the top inside the deck, the code
    is free again, so a look at the top that takes nothing leaves the order as it was. */
class duel_forced_deck_top {
  public:
    /** Before the duel reads the top count cards of player's deck, place the next free codes there: for each position
        holding no placed card, the card nearest the top that has the next free code, or that code as its alias, and
        holds no code is moved there. Code 0 places the card already there. A code the deck does not hold stops a look
        and stays queued; when taking is set, the read takes the cards, so the code is counted as missing and dropped,
        and the card already there is read in its stead. */
    void prepare(field &owner, uint8_t player, uint32_t count, bool taking);
    /** The card with fieldid left player's deck, so the code placed on it is used up. */
    void leave(uint8_t player, uint32_t fieldid) noexcept;
    /** The card with fieldid moved inside player's deck; anywhere but its top, the code placed on it is free again. */
    void moved(uint8_t player, uint32_t fieldid, bool to_top) noexcept;
    /** player's deck was shuffled, so every code placed on it is free again. */
    void release(uint8_t player) noexcept;

    /** Placed on no card. Field ids start at 1. */
    static constexpr uint32_t not_placed = 0;
    /** Per player, the codes of the cards the deck has still to give from its top, first to last; 0 for any card. */
    std::vector<uint32_t> codes[2];
    /** Per player and code, the field id of the card the code is placed on, or not_placed. */
    std::vector<uint32_t> placed[2];
    /** Per player, how many codes the deck did not hold when a read took its top. */
    uint32_t missing[2] = {0, 0};
};
