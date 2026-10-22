// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../../ocgapi.h"
#include "checkpoint.h"
namespace {
int restore_checkpoint(OCG_Duel raw, OCG_Checkpoint handle, std::optional<uint64_t> seed) {
    if (!raw || !handle)
        return OCG_CHECKPOINT_INVALID_ARGUMENT;
    auto &owner = *static_cast<duel *>(raw);
    auto &checkpoint = *static_cast<duel_checkpoint *>(handle);
    if (!checkpoint.belongs_to(owner))
        return OCG_CHECKPOINT_WRONG_OWNER;
    if (owner.last_status != OCG_DUEL_STATUS_AWAITING &&
        !(owner.has_ended && owner.last_status == OCG_DUEL_STATUS_END))
        return OCG_CHECKPOINT_NOT_AWAITING;
    try {
        checkpoint.restore(seed);
        return OCG_CHECKPOINT_SUCCESS;
    } catch (const std::bad_alloc &) {
        return OCG_CHECKPOINT_ALLOCATION_FAILED;
    } catch (...) {
        return OCG_CHECKPOINT_RESTORE_FAILED;
    }
}
} // namespace
int OCG_CreateCheckpoint(OCG_Checkpoint *output, OCG_Duel raw) {
    if (!output)
        return OCG_CHECKPOINT_INVALID_ARGUMENT;
    *output = nullptr;
    if (!raw)
        return OCG_CHECKPOINT_INVALID_ARGUMENT;
    auto &owner = *static_cast<duel *>(raw);
    if (owner.has_ended || owner.last_status != OCG_DUEL_STATUS_AWAITING)
        return OCG_CHECKPOINT_NOT_AWAITING;
    if (!owner.lua->clone_safe)
        return OCG_CHECKPOINT_UNSUPPORTED;
    try {
        *output = new duel_checkpoint(owner);
        return OCG_CHECKPOINT_SUCCESS;
    } catch (const std::bad_alloc &) {
        return OCG_CHECKPOINT_ALLOCATION_FAILED;
    } catch (const std::exception &error) {
        try {
            owner.handle_message(error.what(), OCG_LOG_TYPE_ERROR);
        } catch (...) {
        }
        return OCG_CHECKPOINT_UNSUPPORTED;
    } catch (...) {
        return OCG_CHECKPOINT_UNSUPPORTED;
    }
}
void OCG_DestroyCheckpoint(OCG_Checkpoint handle) {
    delete static_cast<duel_checkpoint *>(handle);
}
int OCG_RestoreCheckpoint(OCG_Duel owner, OCG_Checkpoint handle) {
    return restore_checkpoint(owner, handle, std::nullopt);
}
int OCG_RestoreCheckpointWithSeed(OCG_Duel owner, OCG_Checkpoint handle, uint64_t seed) {
    return restore_checkpoint(owner, handle, seed);
}
