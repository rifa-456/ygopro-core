/*
 * Copyright (c) 2019, Dylam De La Torre, (DyXel)
 * Copyright (c) 2019-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef OCGAPI_H
#define OCGAPI_H
#include "ocgapi_types.h"
#include "clone/profiling/metrics.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#if defined(OCGCORE_EXPORT_FUNCTIONS)
#if defined(_WIN32)
#define OCGAPI EXTERN_C __declspec(dllexport)
#else
#define OCGAPI EXTERN_C __attribute__ ((visibility ("default")))
#endif
#else
#define OCGAPI EXTERN_C
#endif

/*** CORE INFORMATION ***/
OCGAPI void OCG_GetVersion(int* major, int* minor);

/*** DUEL CREATION AND DESTRUCTION ***/
OCGAPI int OCG_CreateDuel(OCG_Duel* out_ocg_duel, const OCG_DuelOptions* options_ptr);
/*** CLONING, CHECKPOINTS AND RE-IDENTIFICATION (YGO-Gym's fork) ***
   Every function here works on a duel waiting for a response (OCG_DUEL_STATUS_AWAITING) and returns a
   status from ocgapi_types.h. Different duels may be used from different threads at once; one duel, and the
   checkpoints of one duel, from one thread at a time. */
/* A copy of an awaiting duel that plays on exactly as the source would, its random state included. The copy
   is a duel of its own: destroy it with OCG_DestroyDuel. The callbacks are the copy's, as in OCG_DuelOptions. */
OCGAPI int OCG_CloneDuel(OCG_Duel* output, OCG_Duel source, const OCG_CloneCallbacks* callbacks);
/* The same, with the engine's random state and Lua's math.random both reseeded from seed. */
OCGAPI int OCG_CloneDuelWithSeed(OCG_Duel* output, OCG_Duel source, const OCG_CloneCallbacks* callbacks, uint64_t seed);
/* Capture an awaiting duel to restore later. The checkpoint belongs to its duel: destroy it with
   OCG_DestroyCheckpoint, before or after the duel. */
OCGAPI int OCG_CreateCheckpoint(OCG_Checkpoint* output, OCG_Duel owner);
OCGAPI void OCG_DestroyCheckpoint(OCG_Checkpoint checkpoint);
/* Return the duel that captured the checkpoint to that moment; it must be awaiting or ended. A checkpoint can be
   restored any number of times. A failed reconstruction invalidates the duel; only destruction is then
   supported. */
OCGAPI int OCG_RestoreCheckpoint(OCG_Duel owner, OCG_Checkpoint checkpoint);
/* The same, with the random state reseeded from seed instead of the captured one. */
OCGAPI int OCG_RestoreCheckpointWithSeed(OCG_Duel owner, OCG_Checkpoint checkpoint, uint64_t seed);
/* Give a card in a deck or hand, or a face-down card, the identity and initial effects of another card.
   Effects the card registered outside itself are left in place. After OCG_IDENTITY_FAILED or
   OCG_IDENTITY_ALLOCATION_FAILED the card may be half changed: restore a checkpoint or destroy the duel. */
OCGAPI int OCG_ReidentifyCard(OCG_Duel duel, uint8_t controller, uint32_t location, uint32_t sequence, uint32_t code);
/* Timers and counters of the cloning phases, compiled in with OCG_CLONE_PROFILING; Enabled is 0 without. */
OCGAPI int OCG_CloneProfileEnabled(void);
OCGAPI int OCG_CloneProfileBegin(void);
OCGAPI int OCG_CloneProfileEnd(OCG_CloneProfileRecord* output);
OCGAPI void OCG_DestroyDuel(OCG_Duel ocg_duel);
OCGAPI void OCG_DuelNewCard(OCG_Duel ocg_duel, const OCG_NewCardInfo* info_ptr);
OCGAPI void OCG_StartDuel(OCG_Duel ocg_duel);

/*** DUEL PROCESSING AND QUERYING ***/
OCGAPI int OCG_DuelProcess(OCG_Duel ocg_duel);
OCGAPI void* OCG_DuelGetMessage(OCG_Duel ocg_duel, uint32_t* length);
OCGAPI void OCG_DuelSetResponse(OCG_Duel ocg_duel, const void* buffer, uint32_t length);
OCGAPI int OCG_LoadScript(OCG_Duel ocg_duel, const char* buffer, uint32_t length, const char* name);

OCGAPI uint32_t OCG_DuelQueryCount(OCG_Duel ocg_duel, uint8_t team, uint32_t loc);
OCGAPI void* OCG_DuelQuery(OCG_Duel ocg_duel, uint32_t* length, const OCG_QueryInfo* info_ptr);
OCGAPI void* OCG_DuelQueryLocation(OCG_Duel ocg_duel, uint32_t* length, const OCG_QueryInfo* info_ptr);
OCGAPI void* OCG_DuelQueryField(OCG_Duel ocg_duel, uint32_t* length);

#undef OCGAPI
#undef EXTERN_C

#endif /* OCGAPI_H */
