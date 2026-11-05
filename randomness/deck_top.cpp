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

void duel_forced_deck_top::prepare(field &owner, uint8_t player, uint32_t count, bool taking, bool reading) {
    if (player > 1)
        return;
    auto &deck = owner.player[player].list_main;
    auto &queued = codes[player];
    auto &on = placed[player];
    auto &seen = read[player];
    bool moved = false;
    // The codes are given in order from the top: the code at each place in the queue goes to the card at that depth,
    // counting only the cards a code went to, so a card put on top after the deck settled takes the code it has.
    std::size_t entry = 0;
    for (std::size_t depth = 0; depth < count && depth < deck.size() && entry < queued.size(); ++depth) {
        const auto top = deck.size() - 1 - depth;
        auto *here = deck[top];
        if (on[entry] == here->fieldid) {
            if (reading)
                seen[entry] = 1;
            ++entry;
            continue;
        }
        const auto code = queued[entry];
        auto found = top + 1;
        if (code == 0 || has_code(here, code)) {
            found = top;
        } else {
            // The card the code was placed on, else the free copy nearest the top, else one a later code holds.
            for (auto index = top; index-- > 0 && found > top;)
                if (deck[index]->fieldid == on[entry])
                    found = index;
            for (auto index = top; index-- > 0 && found > top;)
                if (has_code(deck[index], code) && !holds_code(on, deck[index]))
                    found = index;
            for (auto index = top; index-- > 0 && found > top;)
                if (has_code(deck[index], code))
                    found = index;
        }
        if (found > top) {
            if (!taking)
                break;
            // The card already here is read in its stead, and the code is dropped.
            ++missing[player];
            queued.erase(queued.begin() + static_cast<std::ptrdiff_t>(entry));
            seen.erase(seen.begin() + static_cast<std::ptrdiff_t>(entry));
            on.erase(on.begin() + static_cast<std::ptrdiff_t>(entry));
            continue;
        }
        if (found != top) {
            std::swap(deck[found], deck[top]);
            moved = true;
        }
        const auto fieldid = deck[top]->fieldid;
        for (auto later = entry + 1; later < on.size(); ++later)
            if (on[later] == fieldid) {
                on[later] = not_placed;
                seen[later] = 0;
            }
        on[entry] = fieldid;
        seen[entry] = reading ? 1 : 0;
        ++entry;
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
    const auto index = entry - on.begin();
    auto &seen = read[player];
    if (!seen[static_cast<std::size_t>(index)]) {
        *entry = not_placed;
        return;
    }
    codes[player].erase(codes[player].begin() + index);
    seen.erase(seen.begin() + index);
    on.erase(entry);
}

void duel_forced_deck_top::moved(uint8_t player, uint32_t fieldid, bool to_top) noexcept {
    if (player > 1 || to_top)
        return;
    auto &on = placed[player];
    const auto entry = std::find(on.begin(), on.end(), fieldid);
    if (entry == on.end())
        return;
    read[player][static_cast<std::size_t>(entry - on.begin())] = 0;
    *entry = not_placed;
}

void duel_forced_deck_top::release(uint8_t player) noexcept {
    if (player < 2) {
        std::fill(placed[player].begin(), placed[player].end(), not_placed);
        std::fill(read[player].begin(), read[player].end(), 0);
    }
}

void duel_forced_deck_top::settle(field &owner) {
    for (uint8_t player = 0; player < 2; ++player)
        prepare(owner, player, static_cast<uint32_t>(codes[player].size()), false, false);
}

int OCG_DuelForceDeckTop(OCG_Duel ocg_duel, uint8_t player, const uint32_t *codes, uint32_t count) {
    if (!ocg_duel || player > 1 || (count && !codes))
        return OCG_RANDOM_INVALID_ARGUMENT;
    auto &top = static_cast<duel *>(ocg_duel)->forced_deck_top;
    try {
        top.codes[player].reserve(top.codes[player].size() + count);
        top.placed[player].reserve(top.placed[player].size() + count);
        top.read[player].reserve(top.read[player].size() + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    top.codes[player].insert(top.codes[player].end(), codes, codes + count);
    top.placed[player].resize(top.codes[player].size(), duel_forced_deck_top::not_placed);
    top.read[player].resize(top.codes[player].size(), 0);
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
