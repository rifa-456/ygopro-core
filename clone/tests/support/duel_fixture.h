#pragma once
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "duel.h"
#include "field.h"
#include "ocgapi.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ocg::test {
struct Host {
    std::filesystem::path scripts, fixtures;
    std::vector<std::string> errors;
    int reads = 0;
    std::map<uint32_t, OCG_CardData> definitions;
    void load_cards() {
        std::ifstream in(fixtures / "cards.tsv");
        OCG_CardData c{};
        while (in >> c.code >> c.type >> c.level >> c.attribute >> c.race >> c.attack >> c.defense)
            definitions.emplace(c.code, c);
        require_data();
    }
    void require_data() {
        if (definitions.size() != 42)
            throw std::runtime_error("missing card definitions");
    }
};
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void reader(void *opaque, uint32_t code, OCG_CardData *data) {
    ++static_cast<Host *>(opaque)->reads;
    *data = {};
    data->code = code;
    auto &cards = static_cast<Host *>(opaque)->definitions;
    auto found = cards.find(code);
    if (found != cards.end())
        *data = found->second;
    else if (code == 100000001)
        data->type = TYPE_SPELL;
    else
        throw std::runtime_error("unknown fixture card");
}
int script(void *opaque, OCG_Duel duel, const char *name) {
    auto &host = *static_cast<Host *>(opaque);
    ++host.reads;
    for (auto dir : {host.fixtures, host.scripts, host.scripts / "official"}) {
        std::ifstream in(dir / name, std::ios::binary);
        if (!in)
            continue;
        std::string text((std::istreambuf_iterator<char>(in)), {});
        return OCG_LoadScript(duel, text.data(), static_cast<uint32_t>(text.size()), name);
    }
    return 0;
}
void log(void *opaque, const char *message, int type) {
    if (type == OCG_LOG_TYPE_ERROR) {
        static_cast<Host *>(opaque)->errors.emplace_back(message);
        std::cerr << message << "\n";
    }
}
OCG_CloneCallbacks callbacks(Host &h) {
    return {reader, &h, script, &h, log, &h, nullptr, nullptr};
}
struct Destroy {
    void operator()(duel *d) const {
        OCG_DestroyDuel(d);
    }
};
using DuelPtr = std::unique_ptr<duel, Destroy>;
DuelPtr clone(duel &d, Host &host) {
    auto c = callbacks(host);
    OCG_Duel output = nullptr;
    auto before = d.lua->heap.fingerprint();
    int result = OCG_CloneDuel(&output, &d, &c);
    if (result) {
        for (auto &e : host.errors)
            std::cerr << e << '\n';
        throw std::runtime_error("core clone failed status=" + std::to_string(result));
    }
    require(before == d.lua->heap.fingerprint(), "clone mutated source VM");
    return DuelPtr(static_cast<duel *>(output));
}
std::vector<uint8_t> bytes(duel &d) {
    uint32_t size;
    auto *p = static_cast<uint8_t *>(OCG_DuelGetMessage(&d, &size));
    return {p, p + size};
}
bool has_win(duel &d) {
    auto message = bytes(d);
    for (size_t i = 0; i + 4 < message.size();) {
        uint32_t size;
        std::memcpy(&size, message.data() + i, 4);
        if (message[i + 4] == MSG_WIN)
            return true;
        i += 4 + size;
    }
    return false;
}
int advance(duel &d) {
    std::vector<uint8_t> all;
    for (int n = 0; n < 10000; ++n) {
        int s = OCG_DuelProcess(&d);
        auto chunk = bytes(d);
        all.insert(all.end(), chunk.begin(), chunk.end());
        if (s != OCG_DUEL_STATUS_CONTINUE) {
            d.buff = std::move(all);
            auto message = bytes(d);
            for (size_t i = 0; i + 4 < message.size();) {
                uint32_t size;
                std::memcpy(&size, message.data() + i, 4);
                require(size && i + 4 + size <= message.size(), "malformed message");
                require(message[i + 4] != MSG_RETRY, "core rejected test response");
                i += 4 + size;
            }
            return s;
        }
    }
    throw std::runtime_error("processing did not reach boundary");
}
std::vector<uint8_t> response(duel &d, int choice = 0) {
    auto &f = *d.game_field;
    std::vector<uint8_t> result;
    auto integer = [&](int32_t v) {
        auto *p = reinterpret_cast<uint8_t *>(&v);
        result.insert(result.end(), p, p + 4);
    };
    auto visit = [&](const auto &self, const auto &p) -> void {
        using T = std::decay_t<decltype(p)>;
        if constexpr (!P::IsProcess<T>)
            std::visit([&](const auto &v) { self(self, v); }, p);
        else if constexpr (std::is_same_v<T, P::SelectIdleCmd>)
            integer(!f.core.select_chains.empty() ? 5 : (!f.core.summonable_cards.empty() ? 0 : 7));
        else if constexpr (std::is_same_v<T, P::SelectChain>)
            integer(p.forced ? 0 : -1);
        else if constexpr (std::is_same_v<T, P::SelectYesNo> ||
                           std::is_same_v<T, P::SelectEffectYesNo>)
            integer(choice);
        else if constexpr (std::is_same_v<T, P::SelectOption>)
            integer(0);
        else if constexpr (std::is_same_v<T, P::SelectPosition>)
            integer(p.positions & -p.positions);
        else if constexpr (std::is_same_v<T, P::SelectCard>) {
            integer(0);
            integer(p.min);
            for (int i = 0; i < p.min; ++i)
                integer((i + choice) % f.core.select_cards.size());
        } else if constexpr (std::is_same_v<T, P::SelectPlace>) {
            for (int bit = 0, count = 0; bit < 32 && count < p.count; ++bit)
                if (!(p.flag & (1u << bit))) {
                    result.push_back(bit < 16 ? p.playerid : 1 - p.playerid);
                    result.push_back((bit % 16) < 8 ? LOCATION_MZONE : LOCATION_SZONE);
                    result.push_back(bit % 8);
                    ++count;
                }
        } else
            throw std::runtime_error("unsupported test input processor");
    };
    std::visit([&](const auto &p) { visit(visit, p); }, f.core.units.front());
    return result;
}
void respond(duel &d, const std::vector<uint8_t> &r) {
    OCG_DuelSetResponse(&d, r.data(), r.size());
}
std::string state(duel &d) {
    auto &f = *d.game_field;
    std::ostringstream out;
    out << f.infos.turn_id << ':' << f.infos.phase << ':' << int(f.infos.turn_player) << ':'
        << f.player[0].lp << ':' << f.player[1].lp;
    std::vector<card *> cards(d.cards.begin(), d.cards.end());
    std::sort(cards.begin(), cards.end(), [](auto *a, auto *b) { return a->cardid < b->cardid; });
    for (auto *c : cards)
        out << '|' << c->cardid << ',' << c->data.code << ',' << int(c->current.controler) << ','
            << int(c->current.location) << ',' << c->current.sequence << ',' << c->current.position;
    return out.str();
}
DuelPtr setup(Host &h, uint32_t spell_code) {
    OCG_DuelOptions o{};
    o.seed[0] = 123;
    o.seed[1] = 456;
    o.seed[2] = 789;
    o.seed[3] = 12;
    o.flags = DUEL_MODE_MR5;
    o.team1 = {8000, 0, 1};
    o.team2 = {8000, 0, 1};
    o.cardReader = reader;
    o.payload1 = &h;
    o.scriptReader = script;
    o.payload2 = &h;
    o.logHandler = log;
    o.payload3 = &h;
    OCG_Duel raw = nullptr;
    require(OCG_CreateDuel(&raw, &o) == 0, "create failed");
    DuelPtr d(static_cast<duel *>(raw));
    require(script(&h, raw, "constant.lua"), "constant load failed");
    require(script(&h, raw, "utility.lua"), "utility load failed");
    std::vector<uint32_t> normal;
    for (const auto &[code, data] : h.definitions)
        if (data.type == (TYPE_MONSTER | TYPE_NORMAL))
            normal.push_back(code);
    for (int player = 0; player < 2; ++player) {
        for (int i = 0; i < 8; ++i) {
            OCG_NewCardInfo card{};
            card.team = player;
            card.con = player;
            card.code = normal[i];
            card.loc = LOCATION_DECK;
            card.pos = POS_FACEDOWN_DEFENSE;
            OCG_DuelNewCard(raw, &card);
        }
        OCG_NewCardInfo grave{};
        grave.team = player;
        grave.con = player;
        grave.code = normal[player];
        grave.loc = LOCATION_GRAVE;
        grave.pos = POS_FACEUP_ATTACK;
        OCG_DuelNewCard(raw, &grave);
    }
    OCG_NewCardInfo card{};
    card.code = spell_code;
    card.loc = LOCATION_HAND;
    card.pos = POS_FACEDOWN_DEFENSE;
    OCG_DuelNewCard(raw, &card);
    OCG_Duel invalid = nullptr;
    auto c = callbacks(h);
    require(OCG_CloneDuel(&invalid, raw, &c) == OCG_CLONE_NOT_AWAITING, "prestart clone accepted");
    OCG_StartDuel(raw);
    require(advance(*d) == OCG_DUEL_STATUS_AWAITING, "no initial decision");
    return d;
}

} // namespace ocg::test
