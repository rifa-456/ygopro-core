// SPDX-License-Identifier: AGPL-3.0-or-later
#include <thread>
#include "ocgapi.h"
#include "duel.h"
#include "field.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

struct Host {
    std::filesystem::path scripts,fixtures;
    std::vector<std::string> errors;
    int reads=0;
    std::map<uint32_t,OCG_CardData> definitions;
    void load_cards() {
        std::ifstream in(fixtures/"cards.tsv");
        OCG_CardData c{};
        while(in>>c.code>>c.type>>c.level>>c.attribute>>c.race>>c.attack>>c.defense) definitions.emplace(c.code,c);
        require_data();
    }
    void require_data() {if(definitions.size()!=42) throw std::runtime_error("missing card definitions");}
};
void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
void reader(void* opaque,uint32_t code,OCG_CardData* data) {
    ++static_cast<Host*>(opaque)->reads;
    *data={};data->code=code;
    auto& cards=static_cast<Host*>(opaque)->definitions;
    auto found=cards.find(code);
    if(found!=cards.end()) *data=found->second;
    else if(code==100000001) data->type=TYPE_SPELL;
    else throw std::runtime_error("unknown fixture card");
}
int script(void* opaque,OCG_Duel duel,const char* name) {
    auto& host=*static_cast<Host*>(opaque);++host.reads;
    for(auto dir:{host.fixtures,host.scripts,host.scripts/"official"}) {
        std::ifstream in(dir/name,std::ios::binary);if(!in) continue;
        std::string text((std::istreambuf_iterator<char>(in)),{});
        return OCG_LoadScript(duel,text.data(),static_cast<uint32_t>(text.size()),name);
    }
    return 0;
}
void log(void* opaque,const char* message,int type) {
    if(type==OCG_LOG_TYPE_ERROR) static_cast<Host*>(opaque)->errors.emplace_back(message);
}
OCG_CloneCallbacks callbacks(Host& h) {return {reader,&h,script,&h,log,&h,nullptr,nullptr};}
struct Destroy {void operator()(duel* d)const {OCG_DestroyDuel(d);}};
using DuelPtr=std::unique_ptr<duel,Destroy>;
DuelPtr clone(duel& d,Host& host) {
    auto c=callbacks(host);OCG_Duel output=nullptr;
    auto before=d.lua->heap.fingerprint();
    int result=OCG_CloneDuel(&output,&d,&c);
    if(result) {
        for(auto& e:host.errors) std::cerr<<e<<'\n';
        throw std::runtime_error("core clone failed status="+std::to_string(result));
    }
    require(before==d.lua->heap.fingerprint(),"clone mutated source VM");
    return DuelPtr(static_cast<duel*>(output));
}
std::vector<uint8_t> bytes(duel& d) {uint32_t size;auto* p=static_cast<uint8_t*>(OCG_DuelGetMessage(&d,&size));return {p,p+size};}
bool has_win(duel& d) {
    auto message=bytes(d);
    for(size_t i=0;i+4<message.size();) {
        uint32_t size;std::memcpy(&size,message.data()+i,4);
        if(message[i+4]==MSG_WIN) return true;
        i+=4+size;
    }
    return false;
}
int advance(duel& d) {
    std::vector<uint8_t> all;
    for(int n=0;n<10000;++n) {
        int s=OCG_DuelProcess(&d);
        auto chunk=bytes(d);all.insert(all.end(),chunk.begin(),chunk.end());
        if(s!=OCG_DUEL_STATUS_CONTINUE) {
            d.buff=std::move(all);
            auto message=bytes(d);
            for(size_t i=0;i+4<message.size();) {
                uint32_t size;std::memcpy(&size,message.data()+i,4);
                require(size && i+4+size<=message.size(),"malformed message");
                require(message[i+4]!=MSG_RETRY,"core rejected test response");
                i+=4+size;
            }
            return s;
        }}
    throw std::runtime_error("processing did not reach boundary");
}
std::vector<uint8_t> response(duel& d,int choice=0) {
    auto& f=*d.game_field;std::vector<uint8_t> result;
    auto integer=[&](int32_t v){auto* p=reinterpret_cast<uint8_t*>(&v);result.insert(result.end(),p,p+4);};
    auto visit=[&](const auto& self,const auto& p)->void {
        using T=std::decay_t<decltype(p)>;
        if constexpr(!P::IsProcess<T>) std::visit([&](const auto& v){self(self,v);},p);
        else if constexpr(std::is_same_v<T,P::SelectIdleCmd>) integer(!f.core.select_chains.empty()?5:(!f.core.summonable_cards.empty()?0:7));
        else if constexpr(std::is_same_v<T,P::SelectChain>) integer(p.forced?0:-1);
        else if constexpr(std::is_same_v<T,P::SelectYesNo> || std::is_same_v<T,P::SelectEffectYesNo>) integer(choice);
        else if constexpr(std::is_same_v<T,P::SelectOption>) integer(0);
        else if constexpr(std::is_same_v<T,P::SelectPosition>) integer(p.positions & -p.positions);
        else if constexpr(std::is_same_v<T,P::SelectCard>) {
            integer(0);integer(p.min);for(int i=0;i<p.min;++i)integer((i+choice)%f.core.select_cards.size());
        } else if constexpr(std::is_same_v<T,P::SelectPlace>) {
            for(int bit=0,count=0;bit<32 && count<p.count;++bit) if(!(p.flag&(1u<<bit))) {
                result.push_back(bit<16 ? p.playerid : 1-p.playerid);
                result.push_back((bit%16)<8 ? LOCATION_MZONE : LOCATION_SZONE);
                result.push_back(bit%8);++count;
            }
        } else throw std::runtime_error("unsupported test input processor");
    };
    std::visit([&](const auto& p){visit(visit,p);},f.core.units.front());
    return result;
}
void respond(duel& d,const std::vector<uint8_t>& r) {OCG_DuelSetResponse(&d,r.data(),r.size());}
std::string state(duel& d) {
    auto& f=*d.game_field;std::ostringstream out;
    out<<f.infos.turn_id<<':'<<f.infos.phase<<':'<<int(f.infos.turn_player)<<':'<<f.player[0].lp<<':'<<f.player[1].lp;
    std::vector<card*> cards(d.cards.begin(),d.cards.end());
    std::sort(cards.begin(),cards.end(),[](auto* a,auto* b){return a->cardid<b->cardid;});
    for(auto* c:cards)out<<'|'<<c->cardid<<','<<c->data.code<<','<<int(c->current.controler)<<','<<int(c->current.location)<<','<<c->current.sequence<<','<<c->current.position;
    return out.str();
}
DuelPtr setup(Host& h,uint32_t spell_code) {
    OCG_DuelOptions o{};o.seed[0]=123;o.seed[1]=456;o.seed[2]=789;o.seed[3]=12;
    o.flags=DUEL_MODE_MR5;o.team1={8000,0,1};o.team2={8000,0,1};
    o.cardReader=reader;o.payload1=&h;o.scriptReader=script;o.payload2=&h;o.logHandler=log;o.payload3=&h;
    OCG_Duel raw=nullptr;require(OCG_CreateDuel(&raw,&o)==0,"create failed");DuelPtr d(static_cast<duel*>(raw));
    require(script(&h,raw,"constant.lua"),"constant load failed");
    require(script(&h,raw,"utility.lua"),"utility load failed");
    std::vector<uint32_t> normal;
    for(const auto& [code,data]:h.definitions) if(data.type==(TYPE_MONSTER|TYPE_NORMAL)) normal.push_back(code);
    for(int player=0;player<2;++player) {
        for(int i=0;i<8;++i) {OCG_NewCardInfo card{};card.team=player;card.con=player;card.code=normal[i];card.loc=LOCATION_DECK;card.pos=POS_FACEDOWN_DEFENSE;OCG_DuelNewCard(raw,&card);}
        OCG_NewCardInfo grave{};grave.team=player;grave.con=player;grave.code=normal[player];grave.loc=LOCATION_GRAVE;grave.pos=POS_FACEUP_ATTACK;OCG_DuelNewCard(raw,&grave);
    }
    OCG_NewCardInfo card{};card.code=spell_code;card.loc=LOCATION_HAND;card.pos=POS_FACEDOWN_DEFENSE;OCG_DuelNewCard(raw,&card);
    OCG_Duel invalid=nullptr;auto c=callbacks(h);
    require(OCG_CloneDuel(&invalid,raw,&c)==OCG_CLONE_NOT_AWAITING,"prestart clone accepted");
    OCG_StartDuel(raw);require(advance(*d)==OCG_DUEL_STATUS_AWAITING,"no initial decision");return d;
}
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
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: integration CardScripts fixture-directory");
        int boundaries=0,suspended=0;
        for(auto spell:{100000001u,55144522u,83764718u}) {
            Host h{argv[1],argv[2]};h.load_cards();Host other=h;
            auto original=setup(h,spell);
            check_seeded_clone(*original, other);
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
