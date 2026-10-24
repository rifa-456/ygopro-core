// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <array>
#include <exception>
#include <thread>
#include "support/duel_fixture.h"
using namespace ocg::test;
namespace {
constexpr int workers = 4;
constexpr int steps = 400;
struct CheckpointDestroy {
    void operator()(void *p) const {
        OCG_DestroyCheckpoint(p);
    }
};
std::string play(duel &d, int choice) {
    std::string trace;
    for (int step = 0; step < steps && d.last_status == OCG_DUEL_STATUS_AWAITING && !has_win(d); ++step) {
        respond(d, response(d, choice));
        advance(d);
        trace += state(d) + '\n';
    }
    return trace;
}
std::string branch(duel &source, Host &host, int choice) {
    auto copy = clone(source, host);
    OCG_Checkpoint raw = nullptr;
    require(OCG_CreateCheckpoint(&raw, copy.get()) == OCG_CHECKPOINT_SUCCESS, "checkpoint failed");
    std::unique_ptr<void, CheckpointDestroy> checkpoint(raw);
    const auto first = play(*copy, choice);
    require(OCG_RestoreCheckpoint(copy.get(), checkpoint.get()) == OCG_CHECKPOINT_SUCCESS, "restore failed");
    require(play(*copy, choice) == first, "restored branch diverged");
    return first;
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "usage: threads CardScripts fixture-directory");
        Host host{argv[1], argv[2]};
        host.load_cards();
        auto source = setup(host, 55144522u);
        for (int step = 0; step < 6 && !has_win(*source); ++step) {
            respond(*source, response(*source));
            advance(*source);
        }
        std::array<std::string, workers> expected, actual;
        std::array<Host, workers> hosts{host, host, host, host};
        for (int i = 0; i < workers; ++i)
            expected[i] = branch(*source, hosts[i], i % 2);
        std::array<std::exception_ptr, workers> errors{};
        std::array<std::thread, workers> threads;
        for (int i = 0; i < workers; ++i)
            threads[i] = std::thread([&, i] {
                try {
                    actual[i] = branch(*source, hosts[i], i % 2);
                } catch (...) {
                    errors[i] = std::current_exception();
                }
            });
        for (auto &thread : threads)
            thread.join();
        for (auto &error : errors)
            if (error)
                std::rethrow_exception(error);
        for (int i = 0; i < workers; ++i)
            require(actual[i] == expected[i], "concurrent branch differs from sequential branch");
        require(std::count(expected[0].begin(), expected[0].end(), '\n') > 10, "branches played too few steps");
        std::cout << "PASS " << workers << " concurrent branches\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
