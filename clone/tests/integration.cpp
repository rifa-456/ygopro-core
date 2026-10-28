// SPDX-License-Identifier: AGPL-3.0-or-later
#include <thread>
#include "support/duel_fixture.h"
using namespace ocg::test;

void check_seeded_clone(duel& source, Host& host) {
    auto cb = callbacks(host);
    OCG_Duel first = nullptr, second = nullptr;
    const auto before = source.lua->heap.fingerprint();
    require(OCG_CloneDuelWithSeed(nullptr, &source, &cb, 0) == OCG_CLONE_INVALID_ARGUMENT,
            "seeded clone accepted null output");
    require(OCG_CloneDuelWithSeed(&first, &source, &cb, 42) == OCG_CLONE_SUCCESS,
            "seeded clone failed");
    DuelPtr a(static_cast<duel*>(first));
    require(OCG_CloneDuelWithSeed(&second, &source, &cb, 42) == OCG_CLONE_SUCCESS,
            "second seeded clone failed");
    DuelPtr b(static_cast<duel*>(second));
    require(before == source.lua->heap.fingerprint(), "reseed mutated source VM");
    for(int i = 0; i < 8; ++i) {
        require(a->get_next_integer(1, 1000000) == b->get_next_integer(1, 1000000),
                "seeded core streams diverged");
        auto sample = [](duel& d) {
            auto* L = d.lua->lua_state;
            lua_getglobal(L, "math");
            lua_getfield(L, -1, "random");
            lua_pushinteger(L, 1);
            lua_pushinteger(L, 1000000);
            require(lua_pcall(L, 2, 1, 0) == LUA_OK, "math.random failed");
            auto value = lua_tointeger(L, -1);
            lua_pop(L, 2);
            return value;
        };
        require(sample(*a) == sample(*b), "seeded Lua streams diverged");
    }
    auto exact = clone(*a, host);
    require(a->get_next_integer(1, 1000000) == exact->get_next_integer(1, 1000000),
            "exact clone did not preserve seeded core stream");
}
void check_profiling(duel& source, Host& host) {
    if(!OCG_CloneProfileEnabled()) return;
    require(OCG_CloneProfileBegin(), "could not begin clone profiling");
    auto branch = clone(source, host);
    branch.reset();
    OCG_CloneProfileRecord record{};
    require(OCG_CloneProfileEnd(&record), "could not end clone profiling");
    require(record.counters[OCG_PROFILE_CLONE_ATTEMPTS] == 1, "wrong clone attempt count");
    require(record.counters[OCG_PROFILE_CLONE_SUCCESSES] == 1, "wrong clone success count");
    require(record.counters[OCG_PROFILE_LUA_BLOCKS] > 0, "Lua block count missing");
    require(record.counters[OCG_PROFILE_LUA_BYTES] > 0, "Lua byte count missing");
    require(record.counters[OCG_PROFILE_LUA_TRANSLATIONS] > 0, "Lua translation count missing");
    require(record.nanoseconds[OCG_PROFILE_CORE_CLONE] > 0, "clone timing missing");
    require(record.nanoseconds[OCG_PROFILE_LUA_CLONE] >= record.nanoseconds[OCG_PROFILE_LUA_COPY],
            "nested Lua timing exceeds parent");
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: integration CardScripts fixture-directory");
        int boundaries=0,suspended=0;
        for(auto spell:{100000001u,55144522u,83764718u}) {
            Host h{argv[1],argv[2]};h.load_cards();Host other=h;
            auto original=setup(h,spell);
            check_seeded_clone(*original, other);
            check_profiling(*original, other);
            {
                int unknown=0;auto* L=original->lua->lua_state;
                lua_pushlightuserdata(L,&unknown);lua_setglobal(L,"clone_failure_probe");
                auto before=original->lua->heap.fingerprint();
                auto cb=callbacks(other);OCG_Duel rejected=nullptr;
                require(OCG_CloneDuel(&rejected,original.get(),&cb)==OCG_CLONE_UNSUPPORTED && !rejected,"unknown host pointer accepted");
                require(before==original->lua->heap.fingerprint(),"failed clone mutated source");
                require(h.errors.empty(),"failed clone used source log callback");
                other.errors.clear();
                lua_pushnil(L);lua_setglobal(L,"clone_failure_probe");
                original->lua->clone_safe=false;
                require(OCG_CloneDuel(&rejected,original.get(),&cb)==OCG_CLONE_UNSUPPORTED,"unsafe profile accepted");
                original->lua->clone_safe=true;
            }
            for(int step=0;step<300 && original->last_status==OCG_DUEL_STATUS_AWAITING && !has_win(*original);++step) {
                if(step%7==0) original->lua->collect(true);
                auto branch=clone(*original,other);
                require(state(*original)==state(*branch),"state differs at clone");
                require(bytes(*original)==bytes(*branch),"message differs at clone");
                if(!original->lua->coroutines.empty())++suspended;
                auto reclone=clone(*branch,other);
                if(P::get_opt_variant<P::SelectYesNo>(original->game_field->core.units.front())) {
                    auto x=clone(*original,other);auto y=clone(*original,other);
                    auto before=state(*original);
                    respond(*x,response(*x,0));advance(*x);
                    respond(*y,response(*y,1));advance(*y);
                    require(state(*x)!=state(*y),"different choices did not produce independent outcomes");
                    require(state(*original)==before,"divergent branches changed source");
                }
                auto r=response(*original);
                auto fingerprint=original->lua->heap.fingerprint();
                auto original_state=state(*original);
                if(step%8==0 || !original->lua->coroutines.empty()) {
                auto doomed=clone(*original,other);
                auto survivor=clone(*doomed,other);
                doomed.reset();
                auto reads=h.reads;
                require(survivor->read_script("clone-callback.lua"),"destination callback failed");
                require(h.reads==reads,"clone used source callback payload");
                respond(*survivor,r);int survivor_status=advance(*survivor);
                respond(*branch,r);int b=advance(*branch);
                require(survivor_status==b && bytes(*survivor)==bytes(*branch),"source-first destruction broke continuation");
                } else { respond(*branch,r);advance(*branch); }
                int b=branch->last_status;
                require(state(*original)==original_state,"branch modified native source state");
                require(fingerprint==original->lua->heap.fingerprint(),"branch modified source");
                respond(*original,r);
                auto cb=callbacks(other);OCG_Duel denied=nullptr;
                require(OCG_CloneDuel(&denied,original.get(),&cb)==OCG_CLONE_NOT_AWAITING,"clone accepted a supplied response");
                respond(*reclone,r);
                int a=0,c=0;std::exception_ptr left_error,right_error;
                std::thread left([&]{try {a=advance(*original);}catch(...) {left_error=std::current_exception();}});
                std::thread right([&]{try {c=advance(*reclone);}catch(...) {right_error=std::current_exception();}});
                left.join();right.join();
                if(left_error) std::rethrow_exception(left_error);
                if(right_error) std::rethrow_exception(right_error);
                require(a==b && b==c,"status differs");
                require(bytes(*original)==bytes(*branch) && bytes(*branch)==bytes(*reclone),"messages diverged");
                require(state(*original)==state(*branch) && state(*branch)==state(*reclone),"states diverged");
                ++boundaries;
            }
            require(has_win(*original),"test duel did not finish");
            auto cb=callbacks(other);OCG_Duel denied=nullptr;
            require(OCG_CloneDuel(&denied,original.get(),&cb)==OCG_CLONE_NOT_AWAITING,"terminal clone accepted");
            for(auto& e:h.errors)std::cerr<<e<<'\n';
            require(h.errors.empty() && other.errors.empty(),"script errors");
        }
        require(suspended>0,"no suspended effect tested");
        std::cout<<"PASS "<<boundaries<<" boundaries, "<<suspended<<" suspended coroutine boundaries\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
