// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/graph/copy.h"
#include "clone/persistence/api.h"
#include "support/duel_fixture.h"
using namespace ocg::test;
namespace {
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
std::vector<OCG_RandomResult> recorded(duel &d) {
    uint32_t count = 0;
    require(OCG_DuelGetRandomResults(&d, nullptr, 0, &count) == OCG_RANDOM_SUCCESS, "counting results failed");
    std::vector<OCG_RandomResult> results(count);
    require(OCG_DuelGetRandomResults(&d, results.data(), count, &count) == OCG_RANDOM_SUCCESS, "copying results failed");
    return results;
}
bool left(duel &d, uint32_t first, uint32_t second, int expected = OCG_RANDOM_SUCCESS) {
    uint32_t counts[2] = {};
    return OCG_DuelGetDeckShuffleStatus(&d, counts) == expected && counts[0] == first && counts[1] == second;
}
void statuses(Host &host) {
    auto source = setup(host, 100000001);
    const uint32_t code = 1;
    uint32_t counts[2] = {};
    require(OCG_DuelForceDeckShuffle(nullptr, 0, &code, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    require(OCG_DuelForceDeckShuffle(source.get(), 2, &code, 1) == OCG_RANDOM_INVALID_ARGUMENT, "player 2 accepted");
    require(OCG_DuelForceDeckShuffle(source.get(), 0, nullptr, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no codes accepted");
    require(OCG_DuelGetDeckShuffleStatus(nullptr, counts) == OCG_RANDOM_INVALID_ARGUMENT, "no duel counted");
    require(OCG_DuelGetDeckShuffleStatus(source.get(), nullptr) == OCG_RANDOM_INVALID_ARGUMENT, "no output accepted");
    require(left(*source, 0, 0), "a duel starts with shuffles queued");
}
// A forced shuffle puts the queued cards on top, the first highest, through forced draws that are recorded.
void forced(Host &host) {
    auto source = setup(host, 100000001);
    const auto deck = deck_codes(*source, 0);
    const uint32_t wanted[] = {deck[0], deck[2]};
    OCG_DuelRecordRandomResults(source.get());
    require(OCG_DuelForceDeckShuffle(source.get(), 0, wanted, 2) == OCG_RANDOM_SUCCESS, "force failed");
    require(left(*source, 1, 0), "entry not queued");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(top(*source, 0) == wanted[0] && top(*source, 0, 1) == wanted[1], "queued cards not on top");
    const auto draws = recorded(*source);
    require(draws.size() == deck.size() - 1, "shuffle drew a result per position but the last");
    require(std::all_of(draws.begin(), draws.end(), [](const auto &draw) { return draw.forced; }), "draws not forced");
    require(left(*source, 0, 0), "entry not taken");
}
// The recorded results give the same order to a duel that queued nothing.
void replayed(Host &host) {
    auto source = setup(host, 100000001);
    auto follower = clone(*source, host);
    const uint32_t wanted[] = {deck_codes(*source, 0)[1]};
    OCG_DuelRecordRandomResults(source.get());
    OCG_DuelForceDeckShuffle(source.get(), 0, wanted, 1);
    source->game_field->shuffle(0, LOCATION_DECK);
    std::vector<int32_t> results;
    for (const auto &draw : recorded(*source))
        results.push_back(draw.result);
    OCG_DuelForceRandomResults(follower.get(), results.data(), static_cast<uint32_t>(results.size()));
    follower->game_field->shuffle(0, LOCATION_DECK);
    require(deck_codes(*follower, 0) == deck_codes(*source, 0), "recorded shuffle not replayed");
}
// Each shuffle of a player's deck takes that player's next entry, ahead of results forced before it; other shuffles
// take none, and an empty entry leaves its shuffle to the random state.
void in_order(Host &host) {
    auto source = setup(host, 100000001);
    auto unforced = clone(*source, host);
    const auto deck = deck_codes(*source, 0);
    const uint32_t first[] = {deck[0]};
    const uint32_t second[] = {deck[1]};
    const int32_t coin = 1;
    OCG_DuelForceDeckShuffle(source.get(), 0, first, 1);
    OCG_DuelForceDeckShuffle(source.get(), 0, nullptr, 0);
    OCG_DuelForceDeckShuffle(source.get(), 0, second, 1);
    OCG_DuelForceRandomResults(source.get(), &coin, 1);
    source->game_field->shuffle(0, LOCATION_DECK);
    require(top(*source, 0) == first[0], "first entry not taken first");
    require(source->get_next_integer(0, 1) == coin, "result forced before the shuffle not kept for the next draw");
    source->game_field->shuffle(1, LOCATION_DECK);
    unforced->game_field->shuffle(1, LOCATION_DECK);
    require(deck_codes(*source, 1) == deck_codes(*unforced, 1), "the other player's shuffle took an entry");
    source->game_field->shuffle(0, LOCATION_HAND);
    require(left(*source, 2, 0), "a shuffle of another deck or a hand took an entry");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(left(*source, 1, 0), "empty entry not taken");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(top(*source, 0) == second[0] && left(*source, 0, 0), "last entry not taken last");
}
// An entry naming a card its deck does not hold is refused: its shuffle uses the random state and every entry left
// is dropped.
void refused(Host &host) {
    auto source = setup(host, 100000001);
    auto unforced = clone(*source, host);
    const uint32_t in_hand[] = {100000001};
    const uint32_t valid[] = {deck_codes(*source, 1)[0]};
    OCG_DuelForceDeckShuffle(source.get(), 0, in_hand, 1);
    OCG_DuelForceDeckShuffle(source.get(), 1, valid, 1);
    source->game_field->shuffle(0, LOCATION_DECK);
    unforced->game_field->shuffle(0, LOCATION_DECK);
    require(deck_codes(*source, 0) == deck_codes(*unforced, 0), "refused shuffle not drawn from the random state");
    require(left(*source, 0, 0, OCG_RANDOM_REFUSED), "refusal not reported or entries left");
}
// Clones, snapshots and checkpoints carry the entries left.
void carried(Host &host) {
    auto source = setup(host, 100000001);
    const uint32_t wanted[] = {deck_codes(*source, 0)[0]};
    OCG_DuelForceDeckShuffle(source.get(), 0, wanted, 1);
    auto copy = clone(*source, host);
    auto loaded = ocg::persistence::load(ocg::persistence::save(*source), callbacks(host));
    OCG_Checkpoint checkpoint = nullptr;
    require(OCG_CreateCheckpoint(&checkpoint, source.get()) == OCG_CHECKPOINT_SUCCESS, "capture failed");
    source->game_field->shuffle(0, LOCATION_DECK);
    require(OCG_RestoreCheckpoint(source.get(), checkpoint) == OCG_CHECKPOINT_SUCCESS, "restore failed");
    OCG_DestroyCheckpoint(checkpoint);
    for (auto *d : {source.get(), copy.get(), loaded.get()}) {
        require(left(*d, 1, 0), "entry not carried");
        d->game_field->shuffle(0, LOCATION_DECK);
        require(top(*d, 0) == wanted[0], "carried entry not taken");
    }
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
        forced(host);
        replayed(host);
        in_order(host);
        refused(host);
        carried(host);
        require(host.errors.empty(), "script errors");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
