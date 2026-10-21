// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ocgapi.h"
#include "clone/graph/copy.h"

int OCG_CloneDuel(OCG_Duel* output, OCG_Duel input, const OCG_CloneCallbacks* callbacks) {
    if(!output) return OCG_CLONE_INVALID_ARGUMENT;
    *output=nullptr;
    if(!input || !callbacks || !callbacks->cardReader || !callbacks->scriptReader)
        return OCG_CLONE_INVALID_ARGUMENT;
    auto& source=*static_cast<duel*>(input);
    if(source.has_ended || source.last_status!=OCG_DUEL_STATUS_AWAITING) return OCG_CLONE_NOT_AWAITING;
    if(!source.lua->clone_safe) return OCG_CLONE_UNSUPPORTED;
    return duel_clone::create(output,source,*callbacks);
}
