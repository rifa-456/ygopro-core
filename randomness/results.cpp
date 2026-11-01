// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../duel.h"

#include <algorithm>
#include <new>

int32_t duel_random_results::next(duel &owner, int32_t low, int32_t high) {
    int32_t result = 0;
    bool was_forced = false;
    if (!forced.empty()) {
        const auto candidate = forced.front();
        forced.pop_front();
        if (candidate >= low && candidate <= high) {
            result = candidate;
            was_forced = true;
        } else {
            // The draw is not the one the results were meant for, so none of the rest can be trusted either.
            refused = true;
            forced.clear();
        }
    }
    if (!was_forced) {
        drawing_ = true;
        result = owner.get_next_integer(low, high);
        drawing_ = false;
    }
    if (recording)
        recorded.push_back({low, high, result, static_cast<uint32_t>(was_forced)});
    return result;
}

int OCG_DuelForceRandomResults(OCG_Duel ocg_duel, const int32_t *results, uint32_t count) {
    if (!ocg_duel || (count && !results))
        return OCG_RANDOM_INVALID_ARGUMENT;
    auto &forced = static_cast<duel *>(ocg_duel)->random_results.forced;
    try {
        forced.insert(forced.end(), results, results + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    return OCG_RANDOM_SUCCESS;
}

int OCG_DuelRecordRandomResults(OCG_Duel ocg_duel) {
    if (!ocg_duel)
        return OCG_RANDOM_INVALID_ARGUMENT;
    static_cast<duel *>(ocg_duel)->random_results.recording = true;
    return OCG_RANDOM_SUCCESS;
}

int OCG_DuelGetRandomResults(OCG_Duel ocg_duel, OCG_RandomResult *output, uint32_t capacity, uint32_t *count) {
    if (!ocg_duel || !count || (capacity && !output))
        return OCG_RANDOM_INVALID_ARGUMENT;
    const auto &results = static_cast<duel *>(ocg_duel)->random_results;
    const auto &recorded = results.recorded;
    *count = static_cast<uint32_t>(recorded.size());
    std::copy_n(recorded.begin(), std::min<std::size_t>(capacity, recorded.size()), output);
    return results.refused ? OCG_RANDOM_REFUSED : OCG_RANDOM_SUCCESS;
}
