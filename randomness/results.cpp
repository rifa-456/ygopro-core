// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../card.h"
#include "../duel.h"

#include <algorithm>
#include <new>

namespace {
// Takes the next result of a queue for a draw in [low, high]; one outside it means the queue was meant for other
// draws, so the queue is dropped.
bool take(std::deque<int32_t> &queue, int32_t low, int32_t high, int32_t &result, bool &refused) {
    const auto candidate = queue.front();
    queue.pop_front();
    if (candidate >= low && candidate <= high) {
        result = candidate;
        return true;
    }
    refused = true;
    queue.clear();
    return false;
}
}

int32_t duel_random_results::next(duel &owner, int32_t low, int32_t high) {
    return next_or(owner, low, high, -1);
}

int32_t duel_random_results::next_or(duel &owner, int32_t low, int32_t high, int32_t otherwise) {
    int32_t result = 0;
    bool was_forced = false;
    const bool chance = shuffles_ == 0;
    auto *own = kind_ == OCG_DRAW_COIN ? &coins : kind_ == OCG_DRAW_DIE ? &dice : nullptr;
    if (chance && !chances.empty())
        was_forced = take(chances, low, high, result, refused);
    else if (chance && own && !own->empty())
        was_forced = take(*own, low, high, result, refused);
    else if (!forced.empty())
        was_forced = take(forced, low, high, result, refused);
    if (!was_forced && otherwise >= low && otherwise <= high) {
        result = otherwise;
    } else if (!was_forced) {
        drawing_ = true;
        result = owner.get_next_integer(low, high);
        drawing_ = false;
    }
    if (recording)
        recorded.push_back({low, high, result, static_cast<uint32_t>(was_forced), static_cast<uint32_t>(chance), kind_});
    return result;
}

bool duel_random_results::named(const card_set &from, uint32_t place) const {
    for (std::size_t index = 0; index < cards.size() && index < places.size(); ++index) {
        if (places[index] != place)
            continue;
        for (auto *pcard : from)
            if (pcard->data.code == cards[index] || (pcard->data.alias && pcard->data.alias == cards[index]))
                return true;
    }
    return false;
}

int32_t duel_random_results::pick(duel &owner, const card_set &from, const card_set &taken, bool placed) {
    const auto high = static_cast<int32_t>(from.size()) - 1;
    // A chance result forced by number is a replay's, which picks as the run it replays did.
    if (chances.empty() && shuffles_ == 0) {
        for (auto queued = cards.begin(); queued != cards.end(); ++queued) {
            const auto index = static_cast<std::size_t>(queued - cards.begin());
            const auto where = index < places.size() ? places[index] : 0;
            if (placed && where == 0)
                continue;
            int32_t place = 0;
            for (auto *pcard : from) {
                const auto named = pcard->data.code == *queued || (pcard->data.alias && pcard->data.alias == *queued);
                const auto there = where == 0 || where == ((static_cast<uint32_t>(pcard->current.controler) << 16) |
                                                           pcard->current.location);
                if (named && there && !taken.count(pcard)) {
                    if (index < places.size())
                        places.erase(places.begin() + static_cast<std::ptrdiff_t>(index));
                    cards.erase(queued);
                    if (recording)
                        recorded.push_back({0, high, place, 1, 1, OCG_DRAW_CARD});
                    return place;
                }
                ++place;
            }
        }
    }
    const drawing scope(*this, OCG_DRAW_CARD);
    return owner.get_next_integer(0, high);
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

int OCG_DuelForceChanceResults(OCG_Duel ocg_duel, const int32_t *results, uint32_t count) {
    if (!ocg_duel || (count && !results))
        return OCG_RANDOM_INVALID_ARGUMENT;
    auto &chances = static_cast<duel *>(ocg_duel)->random_results.chances;
    try {
        chances.insert(chances.end(), results, results + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    return OCG_RANDOM_SUCCESS;
}

namespace {
template <typename T>
int replaced(std::deque<T> &queue, const T *values, uint32_t count) {
    try {
        queue.assign(values, values + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    return OCG_RANDOM_SUCCESS;
}
}

int OCG_DuelForceCoinResults(OCG_Duel ocg_duel, const int32_t *results, uint32_t count) {
    if (!ocg_duel || (count && !results))
        return OCG_RANDOM_INVALID_ARGUMENT;
    return replaced(static_cast<duel *>(ocg_duel)->random_results.coins, results, count);
}

int OCG_DuelForceDieResults(OCG_Duel ocg_duel, const int32_t *results, uint32_t count) {
    if (!ocg_duel || (count && !results))
        return OCG_RANDOM_INVALID_ARGUMENT;
    return replaced(static_cast<duel *>(ocg_duel)->random_results.dice, results, count);
}

int OCG_DuelForceRandomCards(OCG_Duel ocg_duel, const uint32_t *codes, uint32_t count) {
    if (!ocg_duel || (count && !codes))
        return OCG_RANDOM_INVALID_ARGUMENT;
    static_cast<duel *>(ocg_duel)->random_results.places.clear();
    return replaced(static_cast<duel *>(ocg_duel)->random_results.cards, codes, count);
}

int OCG_DuelForceRandomCardsFrom(OCG_Duel ocg_duel, const uint32_t *codes, const uint32_t *places, uint32_t count) {
    if (!ocg_duel || (count && (!codes || !places)))
        return OCG_RANDOM_INVALID_ARGUMENT;
    auto &results = static_cast<duel *>(ocg_duel)->random_results;
    const auto status = replaced(results.cards, codes, count);
    return status == OCG_RANDOM_SUCCESS ? replaced(results.places, places, count) : status;
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
