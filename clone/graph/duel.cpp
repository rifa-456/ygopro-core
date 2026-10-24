// SPDX-License-Identifier: AGPL-3.0-or-later
#include "copy.h"

void duel_clone::run() {
    run_core();
    copy_vm();
}

void duel_clone::run_core() {
    auto& d=destination;
    {
    OCG_PROFILE_SCOPE(OCG_PROFILE_CORE_SETUP);
    d.lua=new interpreter(&d,interpreter::clone_tag{});
    OCG_DuelOptions options{};
    d.game_field=new (d.storage) field(&d,options);
    objects.emplace(source.game_field,d.game_field);
    }
    allocate_objects();
    copy_objects();
    copy_field();
    copy_metadata();
    copy_interpreter();
    d.last_status=source.last_status;
}

int duel_clone::create(OCG_Duel* output, duel& source, const OCG_CloneCallbacks& c) {
    std::unique_ptr<duel> d;
    try {
        d=std::make_unique<duel>(duel::clone_tag{});
        d->clone_constructing=true;
        d->read_card_callback=c.cardReader; d->read_card_payload=c.cardPayload;
        d->read_script_callback=c.scriptReader; d->read_script_payload=c.scriptPayload;
        d->handle_message_callback=c.logHandler ? c.logHandler : +[](void*,const char*,int) {};
        d->handle_message_payload=c.logPayload;
        d->read_card_done_callback=c.cardReaderDone ? c.cardReaderDone : +[](void*,OCG_CardData*) {};
        d->read_card_done_payload=c.cardReaderDonePayload;
        duel_clone(source,*d).run();
        d->clone_constructing=false;
        *output=d.release();
        return OCG_CLONE_SUCCESS;
    } catch(const std::bad_alloc&) {
        return OCG_CLONE_ALLOCATION_FAILED;
    } catch(const std::exception& error) {
        try { if(d) d->handle_message(error.what(),OCG_LOG_TYPE_ERROR); } catch(...) {}
        return OCG_CLONE_UNSUPPORTED;
    } catch(...) {
        return OCG_CLONE_UNSUPPORTED;
    }
}

void duel_clone::copy_metadata() {
    auto& d=destination;
    copy(d.assumes,source.assumes);copy(d.uncopy,source.uncopy);
    d.data_cache=source.data_cache;d.buff=source.buff;d.query_buffer=source.query_buffer;
    d.messages=source.messages;d.random=source.random;d.has_ended=source.has_ended;
}
