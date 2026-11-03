// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../duel.h"
#include "../card.h"

#include <algorithm>
#include <new>

namespace {
// The order that puts top on top, the first code highest, with the other cards beneath it in their current order.
bool target_order(const std::vector<card *> &deck, const std::vector<uint32_t> &top, std::vector<card *> &order) {
    order = deck;
    std::vector<card *> placed;
    for (const auto code : top) {
        const auto found =
            std::find_if(order.rbegin(), order.rend(), [code](const card *c) { return c->data.code == code; });
        if (found == order.rend())
            return false;
        placed.push_back(*found);
        order.erase(std::next(found).base());
    }
    order.insert(order.end(), placed.rbegin(), placed.rend());
    return true;
}
}

void duel_forced_shuffles::prepare(duel &owner, uint8_t player, const std::vector<card *> &deck) {
    if (player > 1 || queued[player].empty())
        return;
    const auto top = std::move(queued[player].front());
    queued[player].pop_front();
    if (top.empty())
        return;
    std::vector<card *> order;
    if (!target_order(deck, top, order)) {
        // The deck does not hold the cards the host expects, so none of the later entries can be trusted.
        refused = true;
        queued[0].clear();
        queued[1].clear();
        return;
    }
    // field::shuffle swaps position i with a drawn position in [i, size - 1] for each i but the last, so the
    // result each draw needs is where the card wanted at i is at that point.
    auto working = deck;
    std::vector<int32_t> results;
    for (std::size_t i = 0; i + 1 < working.size(); ++i) {
        const auto at = std::find(working.begin() + i, working.end(), order[i]);
        results.push_back(static_cast<int32_t>(at - working.begin()));
        std::iter_swap(working.begin() + i, at);
    }
    auto &forced = owner.random_results.forced;
    forced.insert(forced.begin(), results.begin(), results.end());
}

int OCG_DuelForceDeckShuffle(OCG_Duel ocg_duel, uint8_t player, const uint32_t *codes, uint32_t count) {
    if (!ocg_duel || player > 1 || (count && !codes))
        return OCG_RANDOM_INVALID_ARGUMENT;
    try {
        static_cast<duel *>(ocg_duel)->forced_shuffles.queued[player].emplace_back(codes, codes + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    return OCG_RANDOM_SUCCESS;
}

int OCG_DuelGetDeckShuffleStatus(OCG_Duel ocg_duel, uint32_t *left) {
    if (!ocg_duel || !left)
        return OCG_RANDOM_INVALID_ARGUMENT;
    const auto &shuffles = static_cast<duel *>(ocg_duel)->forced_shuffles;
    left[0] = static_cast<uint32_t>(shuffles.queued[0].size());
    left[1] = static_cast<uint32_t>(shuffles.queued[1].size());
    return shuffles.refused ? OCG_RANDOM_REFUSED : OCG_RANDOM_SUCCESS;
}
