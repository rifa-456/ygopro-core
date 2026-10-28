// SPDX-License-Identifier: AGPL-3.0-or-later
#include "support/duel_fixture.h"
using namespace ocg::test;
namespace {
uint32_t other_normal(const Host &host, uint32_t code) {
    for (const auto &[candidate, data] : host.definitions)
        if (candidate != code && data.type == (TYPE_MONSTER | TYPE_NORMAL))
            return candidate;
    throw std::runtime_error("no second normal monster");
}
void statuses(Host &host) {
    auto source = setup(host, 100000001);
    auto *deck = source->game_field->get_field_card(0, LOCATION_DECK, 0);
    require(deck, "no deck card");
    const auto code = other_normal(host, deck->data.code);
    require(OCG_ReidentifyCard(source.get(), 2, LOCATION_DECK, 0, code) == OCG_IDENTITY_INVALID_ARGUMENT,
            "player 2 accepted");
    require(OCG_ReidentifyCard(source.get(), 0, LOCATION_DECK, 0, 0) == OCG_IDENTITY_INVALID_ARGUMENT,
            "code 0 accepted");
    require(OCG_ReidentifyCard(source.get(), 0, LOCATION_DECK, 99, code) == OCG_IDENTITY_NOT_FOUND,
            "missing card found");
    require(OCG_ReidentifyCard(source.get(), 0, LOCATION_GRAVE, 0, code) == OCG_IDENTITY_NOT_HIDDEN,
            "face-up graveyard card re-identified");
}
// A re-identified card keeps its place and number and takes the new identity, and a clone of the duel follows it.
void identity(Host &host) {
    auto source = setup(host, 100000001);
    auto *deck = source->game_field->get_field_card(1, LOCATION_DECK, 2);
    require(deck, "no deck card");
    const auto id = deck->cardid;
    const auto code = other_normal(host, deck->data.code);
    require(OCG_ReidentifyCard(source.get(), 1, LOCATION_DECK, 2, code) == OCG_IDENTITY_SUCCESS, "re-identify failed");
    require(deck->data.code == code && deck->cardid == id, "identity not changed in place");
    require(source->game_field->get_field_card(1, LOCATION_DECK, 2) == deck, "card moved");
    auto copy = clone(*source, host);
    require(state(*copy) == state(*source), "clone differs after re-identify");
    for (int i = 0; i < 20 && !source->has_ended; ++i) {
        respond(*source, response(*source));
        respond(*copy, response(*copy));
        advance(*source);
        advance(*copy);
        require(state(*copy) == state(*source), "clone diverged after re-identify");
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
        identity(host);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
