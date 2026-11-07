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
/*** CLONING, CHECKPOINTS, RE-IDENTIFICATION AND RANDOM RESULTS (YGO-Gym's fork) ***
   Every function here returns a status from ocgapi_types.h, and all but the random results work on a duel
   waiting for a response (OCG_DUEL_STATUS_AWAITING). Different duels may be used from different threads at
   once; one duel, and the checkpoints of one duel, from one thread at a time. */
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
/* Every random event (a shuffle, a coin, a die, a random pick) draws its results one at a time. Give the duel's
   next draws these results, in order: each draw takes the next one instead of the random state, which does not
   advance. Results already given and not yet drawn come first. A result outside the range its draw asks for is
   refused: that draw and the later ones use the random state, the results left are dropped, and
   OCG_DuelGetRandomResults returns OCG_RANDOM_REFUSED from then on. Allowed before OCG_StartDuel. */
OCGAPI int OCG_DuelForceRandomResults(OCG_Duel duel, const int32_t* results, uint32_t count);
/* Give the duel's next chance draws these results, in order: the draws outside any shuffle (a coin, a die, a
   random pick) take them ahead of any result forced by OCG_DuelForceRandomResults, and shuffles never take them.
   Results already given and not yet drawn come first; a result outside its draw's range is refused as there.
   Allowed before OCG_StartDuel. */
OCGAPI int OCG_DuelForceChanceResults(OCG_Duel duel, const int32_t* results, uint32_t count);
/* Make the duel's next coin tosses take these results, 1 for heads, or its next die rolls these, replacing any left:
   a toss or a roll takes the next one of its kind unless a chance result is forced ahead of it. A result outside its
   draw's range is refused as there. Allowed before OCG_StartDuel. */
OCGAPI int OCG_DuelForceCoinResults(OCG_Duel duel, const int32_t* results, uint32_t count);
OCGAPI int OCG_DuelForceDieResults(OCG_Duel duel, const int32_t* results, uint32_t count);
/* Make the duel's next random picks of cards take these cards, by code, replacing any left: a pick takes the first
   card queued that the cards it picks from hold and that it has not picked already, and leaves the others queued;
   with none of them, or a chance result forced ahead of it, it draws as any draw does. A queued card is taken once.
   The pick is recorded as a forced draw of the picked card's place among the cards picked from. Allowed before
   OCG_StartDuel. */
OCGAPI int OCG_DuelForceRandomCards(OCG_Duel duel, const uint32_t* codes, uint32_t count);
/* As OCG_DuelForceRandomCards, with where each card is to be taken from: its controller times 0x10000 plus its
   location, or 0 for anywhere, so a card a host's log shows leaving a deck is taken only from that deck. Code 0 with a
   deck's place stands for a read of that deck's top past its queued codes that keeps the card already there; once
   one does, the cards queued for that deck are read in order, one per position from the top, and a code the deck no
   longer holds is dropped. */
OCGAPI int OCG_DuelForceRandomCardsFrom(OCG_Duel duel, const uint32_t* codes, const uint32_t* places, uint32_t count);
/* Record every draw from now on, forced or not: its range, its result, whether it was a chance draw and what it was
   for. Allowed before OCG_StartDuel. */
OCGAPI int OCG_DuelRecordRandomResults(OCG_Duel duel);
/* Copy the first draws recorded, up to capacity, into output, oldest first, and set count to how many there are. */
OCGAPI int OCG_DuelGetRandomResults(OCG_Duel duel, OCG_RandomResult* output, uint32_t capacity, uint32_t* count);
/* Queue the codes of the cards player's deck gives from its top, after those already queued; 0 stands for any card.
   Before each read of a deck's top (a draw, a mill, a look at its top cards, a group of them or their sorting), every
   position read that holds no placed card takes the next code not placed yet: the card nearest the top that has that
   code, or that code as its alias, and holds no code moves there. A code is used up only when its card leaves the
   deck; when the deck is shuffled, or the card is moved away from the top inside the deck, the code is free again for
   the next read. A code the deck does not hold stops a look and stays queued; a draw or a mill counts it as missing,
   drops it and takes the card already there. Allowed before OCG_StartDuel, so the opening hands are drawn this way. */
OCGAPI int OCG_DuelForceDeckTop(OCG_Duel duel, uint8_t player, const uint32_t* codes, uint32_t count);
/* With enabled nonzero, make each read of a deck's top that reaches past the queued codes draw each further card as a
   random pick from the cards at and below its place, recorded and forced like any other random pick (an OCG_DRAW_NAMED
   draw for a card the host names, an OCG_DRAW_KEPT one for any other): a host that rebuilds a logged duel forces the cards the log shows, or the results a run drew. A drawn card
   keeps its place until it leaves, the deck is shuffled, or it moves away from the top. Off by default, so a duel's
   draws are its shuffles' alone. */
OCGAPI int OCG_DuelDrawDeckTops(OCG_Duel duel, int enabled);
/* Write how many queued codes each player's deck has still to give to left[0] and left[1], and how many it did not
   hold when a draw or a mill took its top to missing[0] and missing[1]. */
OCGAPI int OCG_DuelGetDeckTopStatus(OCG_Duel duel, uint32_t* left, uint32_t* missing);
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
