// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/persistence/api.h"
#include "support/duel_fixture.h"
using namespace ocg::test;
int main(int argc, char **argv) {
    try {
        require(argc == 3, "expected scripts and fixtures paths");
        Host host;
        host.scripts = argv[1];
        host.fixtures = argv[2];
        host.load_cards();
        auto source = setup(host, 100000001);
        for (int boundary = 0; boundary < 30 && !source->has_ended; ++boundary) {
            auto data = ocg::persistence::save(*source);
            auto loaded = ocg::persistence::load(data, callbacks(host));
            require(state(*loaded) == state(*source), "snapshot state differs");
            require(bytes(*loaded) == bytes(*source), "snapshot messages differ");
            auto action = response(*source);
            respond(*loaded, action);
            respond(*source, action);
            advance(*loaded);
            advance(*source);
            require(state(*loaded) == state(*source), "snapshot continuation state differs");
            require(bytes(*loaded) == bytes(*source), "snapshot continuation messages differ");
            if (!source->has_ended) {
                auto independent = ocg::persistence::load(std::move(data), callbacks(host));
                source.reset();
                source = clone(*loaded, host);
                respond(*independent, action);
                advance(*independent);
                require(state(*independent) == state(*source),
                        "snapshot retained source ownership");
            }
        }
        require(host.errors.empty(), "script errors");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
