// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../containers_fwd.h"
#include "../ocgapi_types.h"

#include <cstdint>
#include <deque>
#include <vector>

class duel;

/** The results a duel's random events take, as the host forces or records them (YGO-Gym's fork).

    Every random event, a shuffle, a coin, a die or a random pick, takes its results from
    duel::get_next_integer, which hands each draw to this object while results are forced or recorded. */
class duel_random_results {
  public:
    /** Marks the draws of one shuffle while it lives: they are not chance results and take none. */
    class shuffling {
      public:
        explicit shuffling(duel_random_results &results) noexcept : results_(&results) {
            ++results_->shuffles_;
        }
        ~shuffling() {
            --results_->shuffles_;
        }
        shuffling(const shuffling &) = delete;
        shuffling &operator=(const shuffling &) = delete;

      private:
        duel_random_results *results_;
    };

    /** Marks the draws made while it lives as being for one kind of event, an OCG_DrawKind, which they are recorded
        as and take the forced results of. */
    class drawing {
      public:
        drawing(duel_random_results &results, uint32_t kind) noexcept : results_(&results), outer_(results.kind_) {
            results_->kind_ = kind;
        }
        ~drawing() {
            results_->kind_ = outer_;
        }
        drawing(const drawing &) = delete;
        drawing &operator=(const drawing &) = delete;

      private:
        duel_random_results *results_;
        uint32_t outer_;
    };

    /** Whether the next draw comes here: results are forced or recorded, and no draw is under way. */
    bool intercepts() const noexcept {
        return !drawing_ && (recording || !forced.empty() || !chances.empty() || !coins.empty() || !dice.empty());
    }
    /** The result of a draw in [low, high]: the next forced one, a chance result first, then one forced for its kind,
        or else one from the duel's random state. */
    int32_t next(duel &owner, int32_t low, int32_t high);
    /** The result of a draw in [low, high] that, unless a result is forced, takes otherwise rather than the duel's
        random state. */
    int32_t next_or(duel &owner, int32_t low, int32_t high, int32_t otherwise);
    /** The place among from of the card a random pick takes, the cards in taken excepted: the first queued card from
        holds where its place says, recorded as a forced draw, or else a draw like any other. A chance result forced
        ahead goes first. With placed set, a queued card whose place says anywhere is passed over. */
    int32_t pick(duel &owner, const card_set &from, const card_set &taken, bool placed = false);
    /** Whether a queued card has exactly this place, its controller times 0x10000 plus its location, and from holds a
        card of its code there. */
    bool named(const card_set &from, uint32_t place) const;
    /** Whether a read of a deck's top at place keeps the card already there because the host queued code 0 for it.
        Only when some card queued for place is 0 are the cards queued for it read in order, as each position of the
        deck from its top: the first one still held by from is the read's, a code from no longer holds is dropped, and
        a 0 is taken up and keeps the card. Without a 0 nothing changes. */
    bool keeps(const card_set &from, uint32_t place);

    /** The results the next draws take, first to last. */
    std::deque<int32_t> forced;
    /** The results the next chance draws take, those outside any shuffle (a coin, a die, a random pick), ahead of
        any forced result, first to last. */
    std::deque<int32_t> chances;
    /** The results the next coin tosses take, 1 for heads, behind any chance result. */
    std::deque<int32_t> coins;
    /** The results the next die rolls take, behind any chance result. */
    std::deque<int32_t> dice;
    /** The codes of the cards the next random picks take, behind any chance result. */
    std::deque<uint32_t> cards;
    /** Where each of those cards is to be taken from, as a card's controller times 0x10000 plus its location; none, or
        0, for anywhere. */
    std::deque<uint32_t> places;
    /** Every draw since recording started. */
    std::vector<OCG_RandomResult> recorded;
    bool recording = false;
    /** Whether a forced result fell outside its draw's range. */
    bool refused = false;

  private:
    bool drawing_ = false;
    int shuffles_ = 0;
    uint32_t kind_ = OCG_DRAW_OTHER;
};
