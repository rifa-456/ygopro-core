// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/persistence/api.h"
#include "support/duel_fixture.h"
#include <optional>
using namespace ocg::test;

std::vector<uint32_t> ids(const group &value) {
    std::vector<uint32_t> result;
    for (auto *pcard : value.container)
        result.push_back(pcard->cardid);
    return result;
}

std::optional<uint32_t> cursor(duel &d, const std::vector<uint32_t> &members) {
    for (auto *pgroup : d.groups)
        if (ids(*pgroup) == members)
            return pgroup->it == pgroup->container.end() ? std::nullopt
                                                         : std::optional<uint32_t>((*pgroup->it)->cardid);
    throw std::runtime_error("copied group not found");
}

int main(int argc, char **argv) {
    try {
        require(argc == 3, "expected scripts and fixtures paths");
        Host host;
        host.scripts = argv[1];
        host.fixtures = argv[2];
        host.load_cards();
        auto source = setup(host, 100000001);
        auto &deck = source->game_field->player[0].list_main;
        auto held = source->new_group(card_vector(deck.begin(), deck.begin() + 3));
        const auto members = ids(*held);
        held->it = std::next(held->container.begin());
        held->is_iterator_dirty = true;
        const auto expected = std::optional<uint32_t>((*held->it)->cardid);
        require(cursor(*clone(*source, host), members) == expected, "clone moved a dirty group iterator");
        auto data = ocg::persistence::save(*source);
        require(cursor(*ocg::persistence::load(data, callbacks(host)), members) == expected,
                "snapshot moved a dirty group iterator");
        held->it = held->container.end();
        require(!cursor(*clone(*source, host), members), "clone moved an iterator at the end");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
