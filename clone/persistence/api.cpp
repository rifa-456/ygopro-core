// SPDX-License-Identifier: AGPL-3.0-or-later
#include "api.h"
#include "../../ocgapi.h"
#include "archive.h"
void duel_archive::callbacks(duel &d, const OCG_CloneCallbacks &c) {
    d.read_card_callback = c.cardReader;
    d.read_card_payload = c.cardPayload;
    d.read_script_callback = c.scriptReader;
    d.read_script_payload = c.scriptPayload;
    d.handle_message_callback = c.logHandler ? c.logHandler : +[](void *, const char *, int) {};
    d.handle_message_payload = c.logPayload;
    d.read_card_done_callback =
        c.cardReaderDone ? c.cardReaderDone : +[](void *, OCG_CardData *) {};
    d.read_card_done_payload = c.cardReaderDonePayload;
}
namespace ocg::persistence {
std::vector<unsigned char> save(duel &source) {
    if (source.last_status != OCG_DUEL_STATUS_AWAITING || source.has_ended)
        throw std::invalid_argument("snapshot capture requires an awaiting duel");
    if (!source.lua->clone_safe)
        throw std::invalid_argument("snapshot requires safe Lua libraries");
    OCG_Duel copied{};
    OCG_CloneCallbacks callbacks{};
    callbacks.cardReader = +[](void *, uint32_t, OCG_CardData *) {};
    callbacks.scriptReader = +[](void *, OCG_Duel, const char *) { return 0; };
    callbacks.logHandler = +[](void *, const char *, int) {};
    auto status = OCG_CloneDuel(&copied, &source, &callbacks);
    if (status == OCG_CLONE_ALLOCATION_FAILED)
        throw std::bad_alloc();
    if (status != OCG_CLONE_SUCCESS)
        throw std::invalid_argument("snapshot clone is unsupported");
    std::unique_ptr<duel> owned(static_cast<duel *>(copied));
    std::vector<unsigned char> data;
    duel_archive(*owned, data, false).run();
    return data;
}
std::unique_ptr<duel> load(std::vector<unsigned char> data, const OCG_CloneCallbacks &callbacks) {
    if (!callbacks.cardReader || !callbacks.scriptReader)
        throw std::invalid_argument("snapshot restore requires card and script callbacks");
    auto result = std::make_unique<duel>(duel::clone_tag{});
    result->clone_constructing = true;
    duel_archive::callbacks(*result, callbacks);
    result->lua = new interpreter(result.get(), interpreter::clone_tag{});
    result->game_field = new (result->storage) field(result.get(), OCG_DuelOptions{});
    try {
        duel_archive(*result, data, true).run();
        if (result->last_status != OCG_DUEL_STATUS_AWAITING || result->has_ended)
            throw std::invalid_argument("snapshot is not an awaiting duel");
        result->clone_constructing = false;
        return result;
    } catch (...) {
        result->lua->lua_state = nullptr;
        result->lua->current_state = nullptr;
        throw;
    }
}
} // namespace ocg::persistence
