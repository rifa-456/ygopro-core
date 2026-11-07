// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../ocgapi.h"
#include "../duel.h"
#include "../field.h"
#include "../card.h"
#include "../interpreter.h"

#include <algorithm>
#include <iterator>
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
    // A script's check of whether an effect could be activated or a condition holds is hypothetical: it sees the cards
    // where they are, and draws, records and fixes none, so the read the effect makes when it runs takes the cards the
    // host names.
    if (owner.pduel->lua->checking > 0)
        reading = false;
    auto &deck = owner.player[player].list_main;
    auto &queued = codes[player];
    auto &on = placed[player];
    auto &seen = read[player];
    bool moved = false;
    // The codes are given in order from the top: the code at each place in the queue goes to the card at that depth,
    // counting only the cards a code went to, so a card put on top after the deck settled takes the code it has.
    std::size_t entry = 0;
    std::size_t depth = 0;
    for (; depth < count && depth < deck.size() && entry < queued.size(); ++depth) {
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
            erase(player, entry);
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
    // A read past the queued codes draws each further card from those at and below its place; one a code the deck does
    // not hold stopped draws nothing past that code.
    const bool past = entry >= queued.size();
    for (; past && reading && drawing && depth < count && depth < deck.size(); ++depth) {
        const auto top = deck.size() - 1 - depth;
        card_set below;
        for (std::size_t index = 0; index <= top; ++index)
            below.insert(deck[index]);
        // A card the host names from this deck is picked; any other read finds the card already there, as the deck's
        // unseen order puts it, recorded so that a replay finds it again.
        auto &results = owner.pduel->random_results;
        const auto place = (static_cast<uint32_t>(player) << 16) | LOCATION_DECK;
        int32_t chosen = 0;
        if (results.chances.empty() && results.named(below, place)) {
            chosen = results.pick(*owner.pduel, below, {}, true);
        } else {
            const auto here = static_cast<int32_t>(std::distance(below.begin(), below.find(deck[top])));
            const duel_random_results::drawing kept(results, OCG_DRAW_KEPT);
            chosen = results.next_or(*owner.pduel, 0, static_cast<int32_t>(below.size()) - 1, here);
        }
        auto *picked = *std::next(below.begin(), chosen);
        const auto found = std::find(deck.begin(), deck.end(), picked) - deck.begin();
        if (static_cast<std::size_t>(found) != top) {
            std::swap(deck[static_cast<std::size_t>(found)], deck[top]);
            moved = true;
        }
        queued.push_back(picked->data.code);
        on.push_back(picked->fieldid);
        seen.push_back(1);
        drawn[player].push_back(1);
        ++entry;
    }
    if (moved)
        owner.reset_sequence(player, LOCATION_DECK);
}

void duel_forced_deck_top::erase(uint8_t player, std::size_t entry) noexcept {
    const auto at = static_cast<std::ptrdiff_t>(entry);
    codes[player].erase(codes[player].begin() + at);
    placed[player].erase(placed[player].begin() + at);
    read[player].erase(read[player].begin() + at);
    drawn[player].erase(drawn[player].begin() + at);
}

void duel_forced_deck_top::leave(uint8_t player, uint32_t fieldid) noexcept {
    if (player > 1)
        return;
    auto &on = placed[player];
    const auto entry = std::find(on.begin(), on.end(), fieldid);
    if (entry == on.end())
        return;
    const auto index = static_cast<std::size_t>(entry - on.begin());
    if (!read[player][index]) {
        *entry = not_placed;
        return;
    }
    erase(player, index);
}

void duel_forced_deck_top::moved(uint8_t player, uint32_t fieldid, bool to_top) noexcept {
    if (player > 1 || to_top)
        return;
    auto &on = placed[player];
    const auto entry = std::find(on.begin(), on.end(), fieldid);
    if (entry == on.end())
        return;
    const auto index = static_cast<std::size_t>(entry - on.begin());
    if (drawn[player][index]) {
        erase(player, index);
        return;
    }
    read[player][index] = 0;
    *entry = not_placed;
}

void duel_forced_deck_top::release(uint8_t player) noexcept {
    if (player > 1)
        return;
    for (auto entry = drawn[player].size(); entry-- > 0;)
        if (drawn[player][entry])
            erase(player, entry);
    std::fill(placed[player].begin(), placed[player].end(), not_placed);
    std::fill(read[player].begin(), read[player].end(), 0);
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
        top.drawn[player].reserve(top.drawn[player].size() + count);
    } catch (const std::bad_alloc &) {
        return OCG_RANDOM_ALLOCATION_FAILED;
    }
    top.codes[player].insert(top.codes[player].end(), codes, codes + count);
    top.placed[player].resize(top.codes[player].size(), duel_forced_deck_top::not_placed);
    top.read[player].resize(top.codes[player].size(), 0);
    top.drawn[player].resize(top.codes[player].size(), 0);
    return OCG_RANDOM_SUCCESS;
}

int OCG_DuelDrawDeckTops(OCG_Duel ocg_duel, int enabled) {
    if (!ocg_duel)
        return OCG_RANDOM_INVALID_ARGUMENT;
    static_cast<duel *>(ocg_duel)->forced_deck_top.drawing = enabled != 0;
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
