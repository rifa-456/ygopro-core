// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../profiling/metrics.h"
#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include "../../duel.h"
#include "../../field.h"
#include "../../effect.h"
#include "../lua/vm.h"

class duel_clone {
    friend class duel_checkpoint;
    duel& source;
    duel& destination;
    std::unordered_map<const void*, void*> objects;
    std::unordered_map<const std::vector<uint32_t>*, std::vector<uint32_t>*> counters;

    template<class T> T* mapped(T* p) const {
        if(!p) return nullptr;
        OCG_PROFILE_COUNT(OCG_PROFILE_CORE_MAPPED_LOOKUPS, 1);
        auto it = objects.find(p);
        if(it == objects.end()) throw std::runtime_error(std::string("unmapped core object: ") + (std::is_same_v<T,card> ? "card" : std::is_same_v<T,effect> ? "effect" : std::is_same_v<T,group> ? "group" : "userdata"));
        return static_cast<T*>(it->second);
    }
    template<class... T> std::variant<T...> make(const std::variant<T...>& s) {
        return std::visit([&](const auto& v)->std::variant<T...> { return make(v); },s);
    }
    template<class T> T make(const T& s) {
        T d = [&] {
            if constexpr(P::IsProcess<T>) return T(P::CloneTag{});
            else if constexpr(std::is_same_v<T, player_info>) return T(OCG_Player{});
            else if constexpr(std::is_same_v<T, processor::chain_limit_t>) return T(0,0);
            else return T{};
        }();
        copy(d, s);
        return d;
    }
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void copy(T& d, const T& s) {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "missing explicit clone field handler");
        d = s;
    }
    template<class T> void copy(T*& d, T* const& s) { d = mapped(s); }
    template<class T> void copy(owned_lua<T>& d, const owned_lua<T>& s) {
        // Reference counts and registry handles are copied with lua_obj and the VM.
        // Adoption must not increment or release references in either VM.
        if(d.obj) throw std::runtime_error("clone reference destination is not empty");
        d.obj = mapped(s.obj);
    }
    template<class T, size_t N> void copy(T (&d)[N], const T (&s)[N]) {
        for(size_t i=0;i<N;++i) copy(d[i],s[i]);
    }
    template<class T, size_t N> void copy(std::array<T,N>& d, const std::array<T,N>& s) {
        for(size_t i=0;i<N;++i) copy(d[i],s[i]);
    }
    template<class A, class B> void copy(std::pair<A,B>& d, const std::pair<A,B>& s) {
        copy(d.first,s.first); copy(d.second,s.second);
    }
    template<class T, class A> void copy(std::vector<T,A>& d, const std::vector<T,A>& s) {
        d.clear(); d.reserve(s.size());
        for(const auto& v:s) d.push_back(make<T>(v));
    }
    template<class T, class A> void copy(std::list<T,A>& d, const std::list<T,A>& s) {
        d.clear(); for(const auto& v:s) d.push_back(make(v));
    }
    template<class T, class C, class A> void copy(std::set<T,C,A>& d, const std::set<T,C,A>& s) {
        d.clear(); for(const auto& v:s) d.insert(make(v));
    }
    template<class T, class H, class E, class A> void copy(std::unordered_set<T,H,E,A>& d, const std::unordered_set<T,H,E,A>& s) {
        d.clear(); if(!s.empty()) d.reserve(s.size()); for(const auto& v:s) d.insert(make(v));
    }
    template<class K, class V, class C, class A> void copy(std::map<K,V,C,A>& d, const std::map<K,V,C,A>& s) {
        d.clear(); for(const auto& [k,v]:s) d.emplace(make(k),make(v));
    }
    template<class K, class V, class C, class A> void copy(std::multimap<K,V,C,A>& d, const std::multimap<K,V,C,A>& s) {
        d.clear(); for(const auto& [k,v]:s) d.emplace(make(k),make(v));
    }
    template<class K, class V, class H, class E, class A> void copy(std::unordered_map<K,V,H,E,A>& d, const std::unordered_map<K,V,H,E,A>& s) {
        d.clear(); if(!s.empty()) d.reserve(s.size()); for(const auto& [k,v]:s) d.emplace(make(k),make(v));
    }
    template<class T> void copy(std::unique_ptr<T>& d, const std::unique_ptr<T>& s) {
        d = s ? std::make_unique<T>(make(*s)) : nullptr;
    }
    template<class T> void copy(std::optional<T>& d, const std::optional<T>& s) {
        if(s) d.emplace(make(*s)); else d.reset();
    }
    template<class... T> void copy(std::variant<T...>& d, const std::variant<T...>& s) {
        std::visit([&](const auto& v) { d = make(v); },s);
    }
    void copy(card::attacker_map& d, const card::attacker_map& s) {
        using Base=std::unordered_map<uint32_t,std::pair<card*,uint32_t>>;
        copy(static_cast<Base&>(d),static_cast<const Base&>(s));
    }
    void copy(card_data& d, const card_data& s) { d=s; }
    void copy(chain::applied_chain_counter_t*& d, chain::applied_chain_counter_t* const& s) {
        if(!s) {d=nullptr;return;}
        auto [it, inserted]=counters.try_emplace(s,nullptr);
        if(inserted) {
            auto value=std::make_unique<std::vector<uint32_t>>(*s);
            destination.cloned_chain_counters.insert(value.get());
            it->second=value.release();
        }
        d=it->second;
    }
    #include "records.inc"
    #include "processors/choices.inc"
    #include "processors/flow.inc"
    #include "processors/battle.inc"
    #include "processors/movement.inc"
    #include "processors/resources.inc"
    #include "processors/summons.inc"
    #include "processors/announcements.inc"
    #include "field.inc"

    void allocate_objects();
    void copy_objects();
    void copy_field();
    void copy_vm();
    void copy_metadata();
public:
    static void reseed(duel& branch,uint64_t seed);
    static void reseed_core(duel& branch, uint64_t seed);
    duel_clone(duel& s, duel& d):source(s),destination(d) {}
    void run();
    static int create(OCG_Duel* output, duel& source, const OCG_CloneCallbacks& callbacks);
};
