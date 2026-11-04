// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
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

    /** Whether the next draw comes here: results are forced or recorded, and no draw is under way. */
    bool intercepts() const noexcept {
        return !drawing_ && (recording || !forced.empty() || !chances.empty());
    }
    /** The result of a draw in [low, high]: the next forced one, or else one from the duel's random state. */
    int32_t next(duel &owner, int32_t low, int32_t high);

    /** The results the next draws take, first to last. */
    std::deque<int32_t> forced;
    /** The results the next chance draws take, those outside any shuffle (a coin, a die, a random pick), ahead of
        any forced result, first to last. */
    std::deque<int32_t> chances;
    /** Every draw since recording started. */
    std::vector<OCG_RandomResult> recorded;
    bool recording = false;
    /** Whether a forced result fell outside its draw's range. */
    bool refused = false;

  private:
    bool drawing_ = false;
    int shuffles_ = 0;
};
