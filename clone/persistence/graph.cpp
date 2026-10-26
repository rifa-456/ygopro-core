// SPDX-License-Identifier: AGPL-3.0-or-later
#include "archive.h"
#include "vm.h"
#include "../groups/cursor.h"
duel_archive::duel_archive(duel &value, std::vector<unsigned char> &data, bool reading)
    : duel_(value), stream_(data, reading) {}
void duel_archive::register_object(void *value) {
    if (!value || ids_.count(value))
        throw std::invalid_argument("duplicate snapshot native object");
    ids_.emplace(value, objects_.size());
    objects_.push_back(value);
}
std::uint64_t duel_archive::native(const void *value) const {
    if (!value)
        return 0;
    auto found = ids_.find(value);
    if (found == ids_.end())
        throw std::invalid_argument("unregistered snapshot native pointer");
    return found->second;
}
void *duel_archive::native(std::uint64_t id) const {
    if (id >= objects_.size())
        throw std::invalid_argument("invalid snapshot native reference");
    return objects_[id];
}
void duel_archive::lua_base(lua_obj &v) {
    io(v.ref_handle);
    io(v.weak_ref_handle);
    io(v.num_ref);
    io(v.lua_type);
}
void duel_archive::objects() {
    auto &d = duel_;
    register_object(&d);
    register_object(d.game_field);
    register_object(d.lua);
    register_object(&d.lua->heap);
    register_object(&d.lua->deleted);
    if (!stream_.reading) {
        cards_.assign(d.cards.begin(), d.cards.end());
        effects_.assign(d.effects.begin(), d.effects.end());
        groups_.assign(d.groups.begin(), d.groups.end());
        counters_.assign(d.cloned_chain_counters.begin(), d.cloned_chain_counters.end());
    }
    auto n = stream_.count(cards_.size());
    for (std::size_t i = 0; i < n; ++i) {
        uint32_t id = stream_.reading ? 0 : cards_[i]->cardid;
        io(id);
        if (stream_.reading) {
            auto p = std::make_unique<card>(&d);
            p->cardid = id;
            d.cards.insert(p.get());
            cards_.push_back(p.release());
        }
        register_object(cards_[i]);
    }
    n = stream_.count(effects_.size());
    for (std::size_t i = 0; i < n; ++i) {
        uint32_t id = stream_.reading ? 0 : effects_[i]->id;
        io(id);
        if (stream_.reading) {
            auto p = std::make_unique<effect>(&d);
            p->id = id;
            d.effects.insert(p.get());
            effects_.push_back(p.release());
        }
        register_object(effects_[i]);
    }
    n = stream_.count(groups_.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (stream_.reading) {
            auto p = std::make_unique<group>(&d);
            d.groups.insert(p.get());
            groups_.push_back(p.release());
        }
        register_object(groups_[i]);
    }
    n = stream_.count(counters_.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (stream_.reading) {
            auto p = std::make_unique<std::vector<uint32_t>>();
            d.cloned_chain_counters.insert(p.get());
            counters_.push_back(p.release());
        }
        register_object(counters_[i]);
    }
    for (auto *v : counters_)
        io(*v);
    for (auto *v : cards_)
        card_record(*v);
    for (auto *v : effects_)
        effect_record(*v);
    for (auto *v : groups_) {
        lua_base(*v);
        io(v->container);
        io(v->is_readonly);
        io(v->is_iterator_dirty);
        card *cursor = stream_.reading ? nullptr : ocg::groups::cursor(*v);
        io(cursor);
        if (stream_.reading)
            v->it = cursor ? v->container.find(cursor) : v->container.end();
    }
}
void duel_archive::metadata() {
    auto &d = duel_;
    io(d.assumes);
    io(d.uncopy);
    io(d.data_cache);
    io(d.buff);
    io(d.query_buffer);
    auto n = stream_.count(d.messages.size());
    if (stream_.reading)
        for (std::size_t i = 0; i < n; ++i)
            d.messages.emplace_back(0);
    for (auto &message : d.messages)
        io(message.data);
    stream_.scalar(d.random);
    io(d.has_ended);
    io(d.last_status);
}
void duel_archive::vm() {
    auto &vm = *duel_.lua;
    ocg::persistence::VmArchive archive(*this, stream_, vm);
    archive.run();
    io(vm.no_action);
    io(vm.call_depth);
    io(vm.weak_lua_references);
    io(vm.loaded_scripts_table);
    lua_base(vm.deleted);
    auto n = stream_.count(vm.params.size());
    if (stream_.reading)
        vm.params.resize(n);
    for (auto &[param, type] : vm.params) {
        io(type);
        switch (type) {
        case LuaParam::CARD:
        case LuaParam::GROUP:
        case LuaParam::EFFECT:
        case LuaParam::DELETED:
            io(param.ptr);
            break;
        case LuaParam::STRING:
            throw std::invalid_argument("pending borrowed string in snapshot");
        default:
            io(param.integer);
            break;
        }
    }
}
void duel_archive::run() {
    std::uint64_t marker = 0x3150414e5347434fULL;
    auto expected = marker;
    stream_.scalar(marker);
    if (marker != expected)
        throw std::invalid_argument("unsupported core snapshot format");
    objects();
    field_state();
    metadata();
    vm();
    stream_.finish();
}
void duel_archive::copy(card_data &d, const card_data &) {
    io(d.code);
    io(d.alias);
    io(d.setcodes);
    io(d.type);
    io(d.level);
    io(d.attribute);
    io(d.race);
    io(d.attack);
    io(d.defense);
    io(d.lscale);
    io(d.rscale);
    io(d.link_marker);
}
