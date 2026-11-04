// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/graph/copy.h"
#include "clone/persistence/api.h"
#include "support/duel_fixture.h"
using namespace ocg::test;
namespace {
std::vector<OCG_RandomResult> recorded(duel &d, int expected = OCG_RANDOM_SUCCESS) {
    uint32_t count = 0;
    require(OCG_DuelGetRandomResults(&d, nullptr, 0, &count) == expected, "unexpected status counting results");
    std::vector<OCG_RandomResult> results(count);
    require(OCG_DuelGetRandomResults(&d, results.data(), count, &count) == expected, "unexpected status copying results");
    return results;
}
bool same(const std::vector<OCG_RandomResult> &a, const std::vector<OCG_RandomResult> &b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](const auto &x, const auto &y) {
        return x.low == y.low && x.high == y.high && x.result == y.result && x.forced == y.forced && x.kind == y.kind;
    });
}
void statuses(Host &host) {
    auto source = setup(host, 100000001);
    uint32_t count = 0;
    const int32_t result = 1;
    require(OCG_DuelForceRandomResults(nullptr, &result, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    require(OCG_DuelForceRandomResults(source.get(), nullptr, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no results accepted");
    require(OCG_DuelForceRandomResults(source.get(), nullptr, 0) == OCG_RANDOM_SUCCESS, "no results to force refused");
    require(OCG_DuelRecordRandomResults(nullptr) == OCG_RANDOM_INVALID_ARGUMENT, "no duel recorded");
    require(OCG_DuelGetRandomResults(source.get(), nullptr, 0, nullptr) == OCG_RANDOM_INVALID_ARGUMENT,
            "no count accepted");
    require(OCG_DuelGetRandomResults(source.get(), nullptr, 1, &count) == OCG_RANDOM_INVALID_ARGUMENT,
            "no output accepted");
}
// Forced results are taken in order and leave the random state where it was.
void forced(Host &host) {
    auto source = setup(host, 100000001);
    auto unforced = clone(*source, host);
    const int32_t results[] = {3, 0};
    require(OCG_DuelRecordRandomResults(source.get()) == OCG_RANDOM_SUCCESS, "record failed");
    require(OCG_DuelForceRandomResults(source.get(), results, 2) == OCG_RANDOM_SUCCESS, "force failed");
    require(source->get_next_integer(0, 5) == 3 && source->get_next_integer(0, 1) == 0, "forced results not taken");
    require(source->get_next_integer(0, 1000) == unforced->get_next_integer(0, 1000), "random state advanced");
    const auto taken = recorded(*source);
    require(taken.size() == 3 && taken[0].forced && taken[1].forced && !taken[2].forced, "draws not recorded");
    require(taken[0].low == 0 && taken[0].high == 5 && taken[0].result == 3, "draw recorded wrong");
}
// A result outside its draw's range is refused, and so are the results after it.
void refused(Host &host) {
    auto source = setup(host, 100000001);
    auto unforced = clone(*source, host);
    const int32_t results[] = {7, 1};
    require(OCG_DuelForceRandomResults(source.get(), results, 2) == OCG_RANDOM_SUCCESS, "force failed");
    require(source->get_next_integer(0, 5) == unforced->get_next_integer(0, 5), "refused result not drawn");
    require(source->get_next_integer(0, 1000) == unforced->get_next_integer(0, 1000), "dropped result taken");
    require(recorded(*source, OCG_RANDOM_REFUSED).empty(), "refusal not reported");
}
// Clones, snapshots and checkpoints carry the results left to force and those recorded.
void carried(Host &host) {
    auto source = setup(host, 100000001);
    const int32_t results[] = {2, 4};
    OCG_DuelRecordRandomResults(source.get());
    OCG_DuelForceRandomResults(source.get(), results, 2);
    require(source->get_next_integer(0, 9) == 2, "first result not taken");
    auto copy = clone(*source, host);
    auto loaded = ocg::persistence::load(ocg::persistence::save(*source), callbacks(host));
    OCG_Checkpoint checkpoint = nullptr;
    require(OCG_CreateCheckpoint(&checkpoint, source.get()) == OCG_CHECKPOINT_SUCCESS, "capture failed");
    require(source->get_next_integer(0, 9) == 4, "second result not taken");
    require(OCG_RestoreCheckpoint(source.get(), checkpoint) == OCG_CHECKPOINT_SUCCESS, "restore failed");
    OCG_DestroyCheckpoint(checkpoint);
    for (auto *d : {source.get(), copy.get(), loaded.get()}) {
        require(d->get_next_integer(0, 9) == 4, "result left not carried");
        require(recorded(*d).size() == 2, "recorded results not carried");
    }
}
// Coin tosses and die rolls take the results forced for their kind, behind any chance result, and say what they
// were for when recorded.
void kinds(Host &host) {
    auto source = setup(host, 100000001);
    const int32_t coins[] = {1, 0};
    const int32_t dice[] = {6};
    const int32_t chance = 0;
    require(OCG_DuelForceCoinResults(nullptr, coins, 2) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    require(OCG_DuelForceDieResults(source.get(), nullptr, 1) == OCG_RANDOM_INVALID_ARGUMENT, "no results accepted");
    OCG_DuelRecordRandomResults(source.get());
    require(OCG_DuelForceCoinResults(source.get(), coins, 2) == OCG_RANDOM_SUCCESS, "coins refused");
    require(OCG_DuelForceDieResults(source.get(), dice, 1) == OCG_RANDOM_SUCCESS, "dice refused");
    OCG_DuelForceChanceResults(source.get(), &chance, 1);
    auto &results = source->random_results;
    {
        const duel_random_results::drawing tossing(results, OCG_DRAW_COIN);
        require(source->get_next_integer(0, 1) == 0, "chance result not first");
        require(source->get_next_integer(0, 1) == 1, "first coin not taken");
    }
    {
        const duel_random_results::drawing rolling(results, OCG_DRAW_DIE);
        require(source->get_next_integer(1, 6) == 6, "die not taken");
    }
    require(results.coins.size() == 1 && results.dice.empty(), "results left wrong");
    const int32_t again[] = {1};
    OCG_DuelForceCoinResults(source.get(), again, 1);
    require(results.coins.size() == 1 && results.coins.front() == 1, "coins not replaced");
    const auto taken = recorded(*source);
    require(taken.size() == 3 && taken[0].kind == OCG_DRAW_COIN && taken[2].kind == OCG_DRAW_DIE, "kinds not recorded");
    require(taken[1].forced && taken[2].forced, "kind results not recorded as forced");
}
// A random pick takes the first queued card the cards it picks from hold and it has not picked, and leaves the
// others queued; a chance result forced ahead of it goes first.
void picked(Host &host) {
    auto source = setup(host, 100000001);
    card_set from;
    for (auto *pcard : source->game_field->player[0].list_main) {
        const auto same = std::any_of(from.begin(), from.end(), [&](auto *c) { return c->data.code == pcard->data.code; });
        if (!same)
            from.insert(pcard);
        if (from.size() == 4)
            break;
    }
    require(from.size() == 4, "the fixture deck has too few different cards");
    std::vector<card *> order(from.begin(), from.end());
    const uint32_t codes[] = {12345678, order[2]->data.code, order[0]->data.code};
    require(OCG_DuelForceRandomCards(nullptr, codes, 3) == OCG_RANDOM_INVALID_ARGUMENT, "no duel accepted");
    OCG_DuelRecordRandomResults(source.get());
    require(OCG_DuelForceRandomCards(source.get(), codes, 3) == OCG_RANDOM_SUCCESS, "cards refused");
    auto &results = source->random_results;
    require(results.pick(*source, from, {}) == 2, "first held card not picked");
    require(results.pick(*source, from, {order[2]}) == 0, "next held card not picked");
    require(results.cards.size() == 1 && results.cards.front() == 12345678, "card not held not left queued");
    const int32_t chance = 3;
    OCG_DuelForceRandomCards(source.get(), codes + 1, 1);
    OCG_DuelForceChanceResults(source.get(), &chance, 1);
    require(results.pick(*source, from, {}) == 3, "chance result not first");
    require(results.pick(*source, from, {order[3]}) == 2, "queued card not picked after the chance result");
    const auto taken = recorded(*source);
    require(taken.size() == 4, "picks not recorded");
    for (const auto &draw : taken)
        require(draw.kind == OCG_DRAW_CARD && draw.forced && draw.low == 0 && draw.high == 3, "pick recorded wrong");
    require(taken[0].result == 2 && taken[1].result == 0 && taken[2].result == 3, "picked places recorded wrong");
}
// A duel given every result another duel drew plays the same game, whatever its own random state. Only the
// engine's random state is reseeded: the fixture card also rolls Lua's math.random, which no card script uses.
void reproduced(Host &host) {
    auto source = setup(host, 100000001);
    auto follower = clone(*source, host);
    duel_clone::reseed_core(*follower, 99);
    OCG_DuelRecordRandomResults(source.get());
    std::vector<std::vector<uint8_t>> responses;
    for (int i = 0; i < 40 && !source->has_ended; ++i) {
        responses.push_back(response(*source));
        respond(*source, responses.back());
        advance(*source);
    }
    const auto draws = recorded(*source);
    require(!draws.empty(), "the game drew no results");
    std::vector<int32_t> results;
    for (const auto &draw : draws)
        results.push_back(draw.result);
    OCG_DuelRecordRandomResults(follower.get());
    OCG_DuelForceRandomResults(follower.get(), results.data(), static_cast<uint32_t>(results.size()));
    for (const auto &action : responses) {
        respond(*follower, action);
        advance(*follower);
    }
    require(state(*follower) == state(*source), "forced game differs");
    auto followed = recorded(*follower);
    for (auto &draw : followed)
        draw.forced = 0;
    require(same(followed, draws), "forced game drew differently");
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
        refused(host);
        carried(host);
        kinds(host);
        picked(host);
        reproduced(host);
        require(host.errors.empty(), "script errors");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
