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
    card leaves the deck after a read of the top reached it; a card taken from elsewhere, as a search takes one, frees
    its code for another copy. When the deck is shuffled, or the card is moved away from the top inside the deck, the code
    is free again, so a look at the top that takes nothing leaves the order as it was. The deck also settles into that
    order before each step of the duel and after each shuffle, so a script that finds a card by its place in the deck
    before it reads the top, as excavating until a kind of card does, sees the order the reads will give. */
class duel_forced_deck_top {
  public:
    /** Before the duel reads the top count cards of player's deck, give the queued codes to those cards in order: the
        first code still to give goes to the top card, the next to the card below, and so on. A card that has its code,
        or that code as its alias, takes it where it is, so a card put on top after the deck settled keeps its place;
        else the card the code was placed on moves there, or the copy nearest the top that holds no code, or one a later
        code holds, which that code then gives up. Code 0 takes the card already there. A code the deck does not hold
        stops a look and stays queued; when taking is set, the read takes the cards, so the code is counted as missing
        and dropped, and the card already there is read in its stead. With reading unset, as when the deck settles, the
        cards are placed but no read reaches them. */
    void prepare(field &owner, uint8_t player, uint32_t count, bool taking, bool reading = true);
    /** The card with fieldid left player's deck. Its code is used up if a read of the top reached it; a card taken from
        anywhere else in the deck, as a search takes one, only frees its code for another copy, since the deck still
        gives that code from its top. */
    void leave(uint8_t player, uint32_t fieldid) noexcept;
    /** The card with fieldid moved inside player's deck; anywhere but its top, the code placed on it is free again. */
    void moved(uint8_t player, uint32_t fieldid, bool to_top) noexcept;
    /** player's deck was shuffled, so every code placed on it is free again. */
    void release(uint8_t player) noexcept;
    /** Place every free code each player's deck holds, as a read of all of them that takes nothing would. */
    void settle(field &owner);

    /** Placed on no card. Field ids start at 1. */
    static constexpr uint32_t not_placed = 0;
    /** Per player, the codes of the cards the deck has still to give from its top, first to last; 0 for any card. */
    std::vector<uint32_t> codes[2];
    /** Per player and code, the field id of the card the code is placed on, or not_placed. */
    std::vector<uint32_t> placed[2];
    /** Per player and code, whether a read of the deck's top reached the card it is placed on. */
    std::vector<uint8_t> read[2];
    /** Per player, how many codes the deck did not hold when a read took its top. */
    uint32_t missing[2] = {0, 0};
};
