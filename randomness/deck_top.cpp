// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../duel.h"
#include "../field.h"
#include "../card.h"

#include <algorithm>
#include <new>

namespace {
bool has_code(const card *pcard, uint32_t code) {
    return pcard->data.code == code || pcard->data.alias == code;
}

bool holds_code(const std::vector<uint32_t> &placed, const card *pcard) {
    return std::find(placed.begin(), placed.end(), pcard->fieldid) != placed.end();
}
}

void duel_forced_deck_top::prepare(field &owner, uint8_t player, uint32_t count, bool taking) {
    if (player > 1)
        return;
    auto &deck = owner.player[player].list_main;
    auto &queued = codes[player];
    auto &on = placed[player];
    bool moved = false;
    for (std::size_t depth = 0; depth < count && depth < deck.size(); ++depth) {
        const auto top = deck.size() - 1 - depth;
        if (holds_code(on, deck[top]))
            continue;
        const auto next = std::find(on.begin(), on.end(), not_placed);
        if (next == on.end())
            break;
        const auto entry = static_cast<std::size_t>(next - on.begin());
        const auto code = queued[entry];
        auto found = top;
        if (code != 0) {
            // The copy nearest the top is taken, so a card already in place stays and the fewest cards move.
            found = top + 1;
            for (auto index = top + 1; index-- > 0;)
                if (has_code(deck[index], code) && !holds_code(on, deck[index])) {
                    found = index;
                    break;
                }
        }
        if (found > top) {
            if (!taking)
                break;
            ++missing[player];
            queued.erase(queued.begin() + static_cast<std::ptrdiff_t>(entry));
            on.erase(next);
            continue;
        }
        if (found != top) {
            std::swap(deck[found], deck[top]);
            moved = true;
        }
        on[entry] = deck[top]->fieldid;
    }
    if (moved)
        owner.reset_sequence(player, LOCATION_DECK);
}

void duel_forced_deck_top::leave(uint8_t player, uint32_t fieldid) noexcept {
    if (player > 1)
        return;
    auto &on = placed[player];
    const auto entry = std::find(on.begin(), on.end(), fieldid);
    if (entry == on.end())
        return;
    codes[player].erase(codes[player].begin() + (entry - on.begin()));
    on.erase(entry);
}

void duel_forced_deck_top::moved(uint8_t player, uint32_t fieldid, bool to_top) noexcept {
    if (player > 1 || to_top)
        return;
    auto &on = placed[player];
    std::replace(on.begin(), on.end(), fieldid, not_placed);
}

void duel_forced_deck_top::release(uint8_t player) noexcept {
    if (player < 2)
        std::fill(placed[player].begin(), placed[player].end(), not_placed);
}

void duel_forced_deck_top::settle(field &owner) {
    for (uint8_t player = 0; player < 2; ++player)
        prepare(owner, player, static_cast<uint32_t>(codes[player].size()), false);
}

int OCG_DuelForceDeckTop(OCG_Duel ocg_duel, uint8_t player, const uint32_t *codes, uint32_t count) {
    if (!ocg_duel || player > 1 || (count && !codes))
        return OCG_RANDOM_INVALID_ARGUMENT;
    auto &top = static_cast<duel *>(ocg_duel)->forced_deck_top;
    try {
        top.codes[player].reserve(top.codes[player].size() + count);
        top.placed[player].reserve(top.placed[player].size() + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    top.codes[player].insert(top.codes[player].end(), codes, codes + count);
    top.placed[player].resize(top.codes[player].size(), duel_forced_deck_top::not_placed);
    return OCG_RANDOM_SUCCESS;
}

int OCG_DuelGetDeckTopStatus(OCG_Duel ocg_duel, uint32_t *left, uint32_t *missing) {
    if (!ocg_duel || !left || !missing)
        return OCG_RANDOM_INVALID_ARGUMENT;
    const auto &top = static_cast<duel *>(ocg_duel)->forced_deck_top;
    for (uint8_t player = 0; player < 2; ++player) {
        left[player] = static_cast<uint32_t>(top.codes[player].size());
        missing[player] = top.missing[player];
    }
    return OCG_RANDOM_SUCCESS;
}
