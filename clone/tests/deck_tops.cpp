// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/graph/copy.h"
#include "clone/persistence/api.h"
#include "support/duel_fixture.h"

#include <algorithm>
#include <array>
#include <string>

using namespace ocg::test;

namespace {
constexpr uint32_t not_in_deck = 100000001;

// Bottom card first, as list_main stores them.
std::vector<uint32_t> deck_codes(duel &d, uint8_t player) {
    std::vector<uint32_t> codes;
    for (const auto *pcard : d.game_field->player[player].list_main)
        codes.push_back(pcard->data.code);
    return codes;
}

uint32_t top(duel &d, uint8_t player, std::size_t depth = 0) {
    const auto &deck = d.game_field->player[player].list_main;
    return deck[deck.size() - 1 - depth]->data.code;
}

std::vector<uint32_t> hand_codes(duel &d, uint8_t player) {
    std::vector<uint32_t> codes;
    for (const auto *pcard : d.game_field->player[player].list_hand)
        codes.push_back(pcard->data.code);
    std::sort(codes.begin(), codes.end());
    return codes;
}

void force(duel &d, uint8_t player, const std::vector<uint32_t> &codes) {
    require(OCG_DuelForceDeckTop(&d, player, codes.data(), static_cast<uint32_t>(codes.size())) == OCG_RANDOM_SUCCESS,
            "force failed");
}

// Left codes of each player, then missing codes of each player.
bool status(duel &d, std::array<uint32_t, 4> expected) {
    uint32_t left[2] = {};
    uint32_t missing[2] = {};
    return OCG_DuelGetDeckTopStatus(&d, left, missing) == OCG_RANDOM_SUCCESS &&
           std::array<uint32_t, 4>{left[0], left[1], missing[0], missing[1]} == expected;
}

// Groups the top count cards of player's deck, as excavating cards do.
void read_top(duel &d, uint8_t player, int count) {
    const auto chunk = "Duel.GetDecktopGroup(" + std::to_string(player) + "," + std::to_string(count) + ")";
    require(OCG_LoadScript(&d, chunk.data(), static_cast<uint32_t>(chunk.size()), "read_top") != 0, "read failed");
    // The query leaves the duel at its decision; loading the chunk only cleared the status that says so.
    d.last_status = OCG_DUEL_STATUS_AWAITING;
}

void statuses(Host &host) {
    auto source = setup(host, not_in_deck);
    const uint32_t code = 1;
    uint32_t left[2] = {};
    uint32_t missing[2] = {};
    require(OCG_DuelForceDeckTop(nullptr, 0, &code, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    require(OCG_DuelForceDeckTop(source.get(), 2, &code, 1) == OCG_RANDOM_INVALID_ARGUMENT, "player 2 accepted");
    require(OCG_DuelForceDeckTop(source.get(), 0, nullptr, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no codes accepted");
    require(OCG_DuelGetDeckTopStatus(nullptr, left, missing) == OCG_RANDOM_INVALID_ARGUMENT, "no duel counted");
    require(OCG_DuelGetDeckTopStatus(source.get(), nullptr, missing) == OCG_RANDOM_INVALID_ARGUMENT, "no left output");
    require(OCG_DuelGetDeckTopStatus(source.get(), left, nullptr) == OCG_RANDOM_INVALID_ARGUMENT, "no missing output");
    require(status(*source, {0, 0, 0, 0}), "a duel starts with codes queued");
}

// The opening hands are drawn through the queue, which is set before the duel starts.
void opening_hand(Host &host) {
    std::vector<uint32_t> wanted;
    auto source = setup(host, not_in_deck, 2, [&](duel &d) {
        const auto deck = deck_codes(d, 0);
        wanted = {deck[0], deck[2]};
        force(d, 0, wanted);
    });
    std::sort(wanted.begin(), wanted.end());
    auto hand = hand_codes(*source, 0);
    hand.erase(std::remove(hand.begin(), hand.end(), not_in_deck), hand.end());
    require(hand == wanted, "the opening hand is not the queued cards");
    require(status(*source, {0, 0, 0, 0}), "codes left after the opening hand");
}

// Moves the top card of player's deck to the hand, as a draw does after its read.
void take_top(duel &d, uint8_t player) {
    d.game_field->move_card(player, d.game_field->player[player].list_main.back(), LOCATION_HAND, 0);
}

// A look moves the queued cards on top, the first on top, but uses up no code: after a shuffle the next read finds
// the same cards again.
void looked_then_shuffled(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[0], deck[2]});
    read_top(*source, 0, 2);
    require(top(*source, 0) == deck[0] && top(*source, 0, 1) == deck[2], "queued cards not on top");
    require(status(*source, {2, 0, 0, 0}), "a look used up codes");
    force(*source, 0, {deck[4]});
    read_top(*source, 0, 2);
    require(top(*source, 0) == deck[0] && top(*source, 0, 1) == deck[2], "placed cards moved");
    source->game_field->shuffle(0, LOCATION_DECK);
    read_top(*source, 0, 1);
    require(top(*source, 0) == deck[0] && status(*source, {3, 0, 0, 0}), "a shuffle changed the queued order");
}

// The card a read takes uses up its code, and the next read finds the code after it.
void taken(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[0], deck[2]});
    source->forced_deck_top.prepare(*source->game_field, 0, 1, true);
    take_top(*source, 0);
    require(status(*source, {1, 0, 0, 0}), "the taken card's code not used up");
    read_top(*source, 0, 1);
    require(top(*source, 0) == deck[2], "the next code not placed");
}

// A card moved to the bottom frees its code, which the next read places again in its place in the queue: the queue's
// order is the order the deck gives, so the card comes back on top.
void moved_to_bottom(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[0], deck[2]});
    read_top(*source, 0, 2);
    auto *first = source->game_field->player[0].list_main.back();
    source->game_field->move_card(0, first, LOCATION_DECK, 1);
    require(source->game_field->player[0].list_main.front() == first, "card not moved to the bottom");
    read_top(*source, 0, 2);
    require(top(*source, 0) == deck[0] && top(*source, 0, 1) == deck[2], "the freed code not placed again");
    require(status(*source, {2, 0, 0, 0}), "a moved card used up its code");
}

// A look stops at a code the deck does not hold and keeps it; a read that takes counts it as missing, drops it and
// reads the card there; 0 takes the card there; the codes after them are still placed.
void missing_and_any(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    const auto first = top(*source, 0);
    const auto second = top(*source, 0, 1);
    force(*source, 0, {not_in_deck, 0, deck[1]});
    read_top(*source, 0, 3);
    require(top(*source, 0) == first && status(*source, {3, 0, 0, 0}), "a look placed or dropped a missing code");
    source->forced_deck_top.prepare(*source->game_field, 0, 3, true);
    require(top(*source, 0) == first && top(*source, 0, 1) == second, "a missing or any code moved a card");
    require(top(*source, 0, 2) == deck[1], "the code after them not placed");
    require(status(*source, {2, 0, 1, 0}), "missing code not counted and dropped");
}

// A card is found by its alias, as an alternate artwork stands for the card it shows.
void alias(Host &host) {
    auto aliased = host;
    const auto deck = deck_codes(*setup(host, not_in_deck), 0);
    const uint32_t original = 999999999;
    aliased.definitions[deck[3]].alias = original;
    auto source = setup(aliased, not_in_deck);
    force(*source, 0, {original});
    read_top(*source, 0, 1);
    require(top(*source, 0) == deck[3], "the card with the alias not placed");
}

// Each player's reads take that player's codes only.
void per_player(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto wanted = deck_codes(*source, 1)[0];
    force(*source, 1, {wanted});
    read_top(*source, 0, 2);
    require(status(*source, {0, 1, 0, 0}) && top(*source, 1) != wanted, "the other player's read placed a code");
    read_top(*source, 1, 1);
    require(top(*source, 1) == wanted && status(*source, {0, 1, 0, 0}), "code not placed");
}

// Settling places every queued code the deck holds before any read, and a shuffle settles the deck again at once, so a
// script that finds a card by its place in the deck sees the order the reads will give.
void settled(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[0], deck[2], deck[4]});
    source->forced_deck_top.settle(*source->game_field);
    require(top(*source, 0) == deck[0] && top(*source, 0, 1) == deck[2] && top(*source, 0, 2) == deck[4],
            "settling did not place the queued cards");
    require(status(*source, {3, 0, 0, 0}), "settling used up codes");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(top(*source, 0) == deck[0] && top(*source, 0, 1) == deck[2] && top(*source, 0, 2) == deck[4],
            "a shuffle left the queued cards off the top");
}

// A card taken from below the top, as a search takes one, does not use up its code: the code is freed for another
// copy, and a read that takes the top with no copy left counts it missing. A read's own card still uses its code up.
void searched(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[2], deck[0]});
    source->forced_deck_top.settle(*source->game_field);
    auto &main = source->game_field->player[0].list_main;
    auto *placed = main[main.size() - 2];
    require(placed->data.code == deck[0], "settling did not place the second code");
    source->game_field->move_card(0, placed, LOCATION_HAND, 0);
    require(status(*source, {2, 0, 0, 0}), "a search used up a code no read reached");
    source->forced_deck_top.prepare(*source->game_field, 0, 1, true);
    take_top(*source, 0);
    require(status(*source, {1, 0, 0, 0}), "a read's card did not use up its code");
    source->forced_deck_top.prepare(*source->game_field, 0, 1, true);
    require(status(*source, {0, 0, 1, 0}), "the searched code, with no copy left, not counted missing");
}

// A card put on top after the deck settled takes the next code when it has it, so the deck gives it first and the
// code meant for it is not left on a copy further down.
void put_on_top(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[2], deck[4]});
    source->forced_deck_top.settle(*source->game_field);
    auto &main = source->game_field->player[0].list_main;
    auto *placed = main.back();
    require(placed->data.code == deck[2], "settling did not place the first code");
    source->game_field->move_card(0, placed, LOCATION_HAND, 0);
    source->game_field->move_card(0, placed, LOCATION_DECK, 0);
    require(main.back() == placed, "card not put back on top");
    source->forced_deck_top.prepare(*source->game_field, 0, 1, true);
    take_top(*source, 0);
    require(status(*source, {1, 0, 0, 0}), "the card put back on top did not use up its code");
    read_top(*source, 0, 1);
    require(top(*source, 0) == deck[4], "the next code not placed after it");
}

// Clones, snapshots and checkpoints carry the codes left and the cards placed.
void carried(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    force(*source, 0, {deck[0], deck[1]});
    read_top(*source, 0, 1);
    auto copy = clone(*source, host);
    auto loaded = ocg::persistence::load(ocg::persistence::save(*source), callbacks(host));
    OCG_Checkpoint checkpoint = nullptr;
    require(OCG_CreateCheckpoint(&checkpoint, source.get()) == OCG_CHECKPOINT_SUCCESS, "capture failed");
    read_top(*source, 0, 2);
    require(OCG_RestoreCheckpoint(source.get(), checkpoint) == OCG_CHECKPOINT_SUCCESS, "restore failed");
    OCG_DestroyCheckpoint(checkpoint);
    for (auto *d : {source.get(), copy.get(), loaded.get()}) {
        require(status(*d, {2, 0, 0, 0}), "codes left not carried");
        read_top(*d, 0, 2);
        require(top(*d, 0) == deck[0] && top(*d, 0, 1) == deck[1], "placed card or code left not carried");
    }
}

// The card draws a duel recorded, as OCG_DRAW_CARD picks.
std::size_t card_draws(duel &d) {
    return static_cast<std::size_t>(std::count_if(d.random_results.recorded.begin(), d.random_results.recorded.end(),
                                                  [](const OCG_RandomResult &draw) { return draw.kind == OCG_DRAW_CARD; }));
}

// With drawing set, a read past the queued codes draws each further card as a random pick: a card a host forces by its
// code comes up, the draw is recorded, and the card keeps its place until a shuffle forgets it.
void drawn(Host &host) {
    auto source = setup(host, not_in_deck);
    const auto deck = deck_codes(*source, 0);
    require(OCG_DuelDrawDeckTops(nullptr, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    require(OCG_DuelDrawDeckTops(source.get(), 1) == OCG_RANDOM_SUCCESS, "drawing not set");
    OCG_DuelRecordRandomResults(source.get());
    force(*source, 0, {deck[3]});
    const std::array<uint32_t, 2> named{not_in_deck, deck[0]};
    require(OCG_DuelForceRandomCards(source.get(), named.data(), 2) == OCG_RANDOM_SUCCESS, "cards not forced");
    read_top(*source, 0, 2);
    require(top(*source, 0) == deck[3] && top(*source, 0, 1) == deck[0], "the named card not drawn below the queued");
    require(card_draws(*source) == 1 && status(*source, {2, 0, 0, 0}), "the draw not recorded as one pick");
    read_top(*source, 0, 2);
    require(top(*source, 0, 1) == deck[0] && card_draws(*source) == 1, "a drawn card drawn again");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(status(*source, {1, 0, 0, 0}), "a shuffle kept the drawn card");
}

// A run's draws replayed as chance results draw the same cards; without drawing set, a read draws nothing.
void replayed(Host &host) {
    auto source = setup(host, not_in_deck);
    OCG_DuelDrawDeckTops(source.get(), 1);
    OCG_DuelRecordRandomResults(source.get());
    auto copy = clone(*source, host);
    read_top(*source, 0, 3);
    std::vector<int32_t> results;
    for (const auto &draw : source->random_results.recorded)
        results.push_back(draw.result);
    require(results.size() == 3, "each card past the queue not drawn");
    require(OCG_DuelForceChanceResults(copy.get(), results.data(), 3) == OCG_RANDOM_SUCCESS, "results not forced");
    read_top(*copy, 0, 3);
    for (std::size_t depth = 0; depth < 3; ++depth)
        require(top(*copy, 0, depth) == top(*source, 0, depth), "a replay drew another card");
    auto plain = setup(host, not_in_deck);
    OCG_DuelRecordRandomResults(plain.get());
    read_top(*plain, 0, 3);
    require(card_draws(*plain) == 0, "a read drew without drawing set");
}
}

int main(int argc, char **argv) {
    try {
        require(argc == 3, "expected scripts and fixtures paths");
        Host host;
        host.scripts = argv[1];
        host.fixtures = argv[2];
        host.load_cards();
        statuses(host);
        opening_hand(host);
        looked_then_shuffled(host);
        taken(host);
        moved_to_bottom(host);
        missing_and_any(host);
        alias(host);
        per_player(host);
        settled(host);
        searched(host);
        put_on_top(host);
        carried(host);
        drawn(host);
        replayed(host);
        require(host.errors.empty(), "script errors");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
