// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../graph/copy.h"
#include "stream.h"
#include <deque>
#include <string>
#include <tuple>
template <class T> struct snapshot_variant : std::false_type {};
template <class... T> struct snapshot_variant<std::variant<T...>> : std::true_type {};
class duel_archive {
  public:
    duel_archive(duel &value, std::vector<unsigned char> &data, bool reading);
    void run();
    static void callbacks(duel &value, const OCG_CloneCallbacks &callbacks);
    void *native(std::uint64_t id) const;
    std::uint64_t native(const void *pointer) const;

  private:
    duel &duel_;
    ocg::persistence::Stream stream_;
    std::vector<void *> objects_{nullptr};
    std::unordered_map<const void *, std::uint64_t> ids_;
    std::vector<card *> cards_;
    std::vector<effect *> effects_;
    std::vector<group *> groups_;
    std::vector<std::vector<uint32_t> *> counters_;
    void register_object(void *value);
    void objects();
    void card_record(card &value);
    void effect_record(effect &value);
    void field_state();
    void metadata();
    void vm();
    void lua_base(lua_obj &value);
    template <class T> static auto constructor_arguments() {
        if constexpr (P::IsProcess<T>)
            return std::make_tuple(P::CloneTag{});
        else if constexpr (std::is_same_v<T, player_info>)
            return std::make_tuple(OCG_Player{});
        else if constexpr (std::is_same_v<T, processor::chain_limit_t>)
            return std::make_tuple(0, 0);
        else if constexpr (snapshot_variant<T>::value)
            return std::tuple_cat(std::make_tuple(std::in_place_index<0>),
                                  constructor_arguments<std::variant_alternative_t<0, T>>());
        else
            return std::tuple<>();
    }
    template <class T> T make() {
        return std::apply([](auto... arguments) { return T(arguments...); },
                          constructor_arguments<T>());
    }
    template <class T> void io(T &value) {
        copy(value, value);
    }
    template <class T> void copy(T &value, const T &) {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>,
                      "missing snapshot field handler");
        stream_.scalar(value);
    }
    template <class T> void copy(T *&value, T *const &) {
        auto id = stream_.reading ? std::uint64_t{} : native(value);
        stream_.scalar(id);
        if (stream_.reading)
            value = static_cast<T *>(native(id));
    }
    template <class T> void copy(owned_lua<T> &value, const owned_lua<T> &) {
        io(value.obj);
    }
    template <class T, std::size_t N> void copy(T (&value)[N], const T (&)[N]) {
        for (auto &item : value)
            io(item);
    }
    template <class T, std::size_t N> void copy(std::array<T, N> &value, const std::array<T, N> &) {
        for (auto &item : value)
            io(item);
    }
    template <class A, class B> void copy(std::pair<A, B> &value, const std::pair<A, B> &) {
        io(value.first);
        io(value.second);
    }
    template <class C> void sequence(C &value) {
        auto n = stream_.count(value.size());
        if (stream_.reading) {
            value.clear();
            for (std::uint64_t i = 0; i < n; ++i) {
                auto item = make<typename C::value_type>();
                io(item);
                value.push_back(std::move(item));
            }
        } else
            for (auto &item : value)
                io(item);
    }
    template <class T, class A> void copy(std::vector<T, A> &v, const std::vector<T, A> &) {
        sequence(v);
    }
    template <class A> void copy(std::vector<bool, A> &v, const std::vector<bool, A> &) {
        auto n = stream_.count(v.size());
        if (stream_.reading)
            v.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            bool item = v[i];
            io(item);
            if (stream_.reading)
                v[i] = item;
        }
    }
    template <class T, class A> void copy(std::list<T, A> &v, const std::list<T, A> &) {
        sequence(v);
    }
    template <class T, class A> void copy(std::deque<T, A> &v, const std::deque<T, A> &) {
        sequence(v);
    }
    template <class C> void set(C &value) {
        auto n = stream_.count(value.size());
        if (stream_.reading) {
            value.clear();
            for (std::uint64_t i = 0; i < n; ++i) {
                auto item = make<typename C::value_type>();
                io(item);
                value.insert(std::move(item));
            }
        } else
            for (auto &item : value) {
                auto mutable_item = item;
                io(mutable_item);
            }
    }
    template <class T, class C, class A>
    void copy(std::set<T, C, A> &v, const std::set<T, C, A> &) {
        set(v);
    }
    template <class T, class H, class E, class A>
    void copy(std::unordered_set<T, H, E, A> &v, const std::unordered_set<T, H, E, A> &) {
        set(v);
    }
    template <class C> void map(C &value) {
        auto n = stream_.count(value.size());
        if (stream_.reading) {
            value.clear();
            for (std::uint64_t i = 0; i < n; ++i) {
                auto key = make<typename C::key_type>();
                auto item = make<typename C::mapped_type>();
                io(key);
                io(item);
                value.emplace(std::move(key), std::move(item));
            }
        } else
            for (auto &[key, item] : value) {
                auto mutable_key = key;
                io(mutable_key);
                io(item);
            }
    }
    template <class K, class V, class C, class A>
    void copy(std::map<K, V, C, A> &v, const std::map<K, V, C, A> &) {
        map(v);
    }
    template <class K, class V, class C, class A>
    void copy(std::multimap<K, V, C, A> &v, const std::multimap<K, V, C, A> &) {
        map(v);
    }
    template <class K, class V, class H, class E, class A>
    void copy(std::unordered_map<K, V, H, E, A> &v, const std::unordered_map<K, V, H, E, A> &) {
        map(v);
    }
    template <class T> void copy(std::optional<T> &v, const std::optional<T> &) {
        bool present = v.has_value();
        io(present);
        if (stream_.reading) {
            if (present) {
                std::apply([&](auto... arguments) { v.emplace(arguments...); },
                           constructor_arguments<T>());
            } else
                v.reset();
        }
        if (present)
            io(*v);
    }
    template <class T> void copy(std::unique_ptr<T> &v, const std::unique_ptr<T> &) {
        bool present = bool(v);
        io(present);
        if (stream_.reading) {
            if (present) {
                v = std::apply([](auto... arguments) { return std::make_unique<T>(arguments...); },
                               constructor_arguments<T>());
            } else
                v.reset();
        }
        if (present)
            io(*v);
    }
    template <std::size_t I = 0, class... T>
    void alternative(std::variant<T...> &v, std::size_t index) {
        if constexpr (I < sizeof...(T)) {
            if (I == index) {
                using Item = std::variant_alternative_t<I, std::variant<T...>>;
                std::apply([&](auto... arguments) { v.template emplace<I>(arguments...); },
                           constructor_arguments<Item>());
            } else
                alternative<I + 1>(v, index);
        } else
            throw std::invalid_argument("invalid snapshot variant");
    }
    template <class... T> void copy(std::variant<T...> &v, const std::variant<T...> &) {
        auto index = stream_.count(v.index());
        if (stream_.reading)
            alternative(v, index);
        std::visit([&](auto &item) { io(item); }, v);
    }
    void copy(card::attacker_map &v, const card::attacker_map &) {
        using Base = std::unordered_map<uint32_t, std::pair<card *, uint32_t>>;
        io(static_cast<Base &>(v));
    }
    void copy(card_data &v, const card_data &);
    void copy_iterator(card_vector::iterator &it, card_vector &v, card_vector::iterator,
                       const card_vector &) {
        auto index = stream_.count(stream_.reading ? 0 : std::distance(v.begin(), it));
        if (index > v.size())
            throw std::invalid_argument("invalid snapshot iterator");
        if (stream_.reading)
            it = v.begin() + index;
    }
#include "../graph/field.inc"
#include "../graph/processors/announcements.inc"
#include "../graph/processors/battle.inc"
#include "../graph/processors/choices.inc"
#include "../graph/processors/flow.inc"
#include "../graph/processors/movement.inc"
#include "../graph/processors/resources.inc"
#include "../graph/processors/summons.inc"
#include "../graph/records.inc"
};
