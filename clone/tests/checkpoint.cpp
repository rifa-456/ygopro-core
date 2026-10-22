// SPDX-License-Identifier: AGPL-3.0-or-later
#include "clone/lua/heap.h"
#include "support/duel_fixture.h"
using namespace ocg::test;
namespace {
struct Drop {
    void operator()(void *p) const {
        OCG_DestroyCheckpoint(p);
    }
};
using Checkpoint = std::unique_ptr<void, Drop>;
Checkpoint capture(duel &source) {
    OCG_Checkpoint p = nullptr;
    const auto result = OCG_CreateCheckpoint(&p, &source);
    if (result)
        throw std::runtime_error("capture failed status=" + std::to_string(result));
    return Checkpoint(p);
}
void run(duel &source, int count, int choice = 0) {
    for (int i = 0; i < count && !source.has_ended; ++i) {
        respond(source, response(source, choice));
        advance(source);
    }
}
std::string digest(duel &d) {
    auto result = state(d);
    auto data = bytes(d);
    result.append(reinterpret_cast<const char *>(data.data()), data.size());
    return result;
}
void allocator() {
    ocg::lua_clone::Heap heap(256 * 1024);
    auto *first = ocg::lua_clone::Heap::allocate(&heap, nullptr, 0, 32);
    require(first, "allocate");
    std::memset(first, 42, 32);
    const auto root = heap.snapshot();
    const auto blocks = heap.blocks();
    auto *grown = ocg::lua_clone::Heap::allocate(&heap, first, 32, 100000);
    require(grown && static_cast<unsigned char *>(grown)[0] == 42, "resize");
    require(!ocg::lua_clone::Heap::allocate(&heap, grown, 100000, 512 * 1024), "reserve limit");
    require(static_cast<unsigned char *>(grown)[0] == 42, "failed realloc damaged original");
    heap.restore(root);
    require(heap.blocks() == blocks && static_cast<unsigned char *>(first)[31] == 42,
            "allocation census rewind");
    heap.fail_after = heap.allocations;
    require(!ocg::lua_clone::Heap::allocate(&heap, first, 32, 64), "allocation fault ignored");
    require(static_cast<unsigned char *>(first)[0] == 42, "fault changed original");
    heap.fail_after = std::numeric_limits<std::size_t>::max();
    ocg::lua_clone::Heap::allocate(&heap, first, 32, 0);
    require(heap.empty(), "free census");
    heap.restore(root);
    require(heap.blocks() == blocks, "repeat allocator restore");
}
void check(Host &host, uint32_t spell) {
    auto source = setup(host, spell);
    auto root = digest(*source);
    auto a = capture(*source);
    std::string expected[2];
    for (int i = 0; i < 2; ++i) {
        auto reference = clone(*source, host);
        run(*reference, 16, i);
        expected[i] = digest(*reference);
    }
    for (int cycle = 0; cycle < 150; ++cycle) {
        run(*source, 16, cycle % 2);
        require(digest(*source) == expected[cycle % 2], "checkpoint continuation differs");
        lua_gc(source->lua->lua_state, LUA_GCCOLLECT);
        require(!OCG_RestoreCheckpoint(source.get(), a.get()), "repeat restore");
        require(digest(*source) == root, "root differs");
        auto copied = clone(*source, host);
        require(digest(*copied) == root, "clone after restore");
    }
    run(*source, 2);
    auto second = digest(*source);
    auto b = capture(*source);
    require(!OCG_RestoreCheckpoint(source.get(), a.get()), "restore earlier");
    require(!OCG_RestoreCheckpoint(source.get(), b.get()) && digest(*source) == second,
            "restore later");
    auto other = setup(host, spell);
    auto before = digest(*other);
    require(OCG_RestoreCheckpoint(other.get(), a.get()) == OCG_CHECKPOINT_WRONG_OWNER &&
                digest(*other) == before,
            "foreign checkpoint changed owner");
    require(!OCG_RestoreCheckpointWithSeed(source.get(), a.get(), 42), "seed restore");
    run(*source, 8);
    const auto seeded = digest(*source);
    require(!OCG_RestoreCheckpointWithSeed(source.get(), a.get(), 42), "repeat seed restore");
    run(*source, 8);
    require(digest(*source) == seeded, "seeded restoration differs");
    require(!OCG_RestoreCheckpoint(source.get(), a.get()), "restore before terminal");
    run(*source, 2048);
    require(source->has_ended, "fixture did not terminate");
    require(!OCG_RestoreCheckpoint(source.get(), a.get()) && !source->has_ended,
            "restore terminal");
    source->storage.fail_after = source->storage.requests + 2;
    require(OCG_RestoreCheckpoint(source.get(), a.get()) == OCG_CHECKPOINT_ALLOCATION_FAILED,
            "native failure not reported");
    require(source->lua->lua_state == nullptr && source->last_status == -1,
            "failed restore left a live partial VM");
    source.reset();
    a.reset();
    b.reset();
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "scripts and fixtures required");
        allocator();
        Host host;
        host.scripts = argv[1];
        host.fixtures = argv[2];
        host.load_cards();
        for (auto code : {55144522u, 83764718u, 100000001u})
            check(host, code);
        require(host.errors.empty(), "script errors");
        std::cout << "PASS checkpoint continuations, census, ownership, RNG, terminal restore, and "
                     "allocation failure\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
