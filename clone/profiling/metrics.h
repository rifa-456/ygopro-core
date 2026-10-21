// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stdint.h>

enum OCG_CloneProfilePhase {
    OCG_PROFILE_HOST_CLONE,
    OCG_PROFILE_HOST_STATE,
    OCG_PROFILE_HOST_CLOSE,
    OCG_PROFILE_CORE_CLONE,
    OCG_PROFILE_CORE_SETUP,
    OCG_PROFILE_CORE_ALLOCATE,
    OCG_PROFILE_CORE_OBJECTS,
    OCG_PROFILE_CORE_FIELD,
    OCG_PROFILE_CORE_METADATA,
    OCG_PROFILE_CORE_RESEED,
    OCG_PROFILE_CORE_DESTROY,
    OCG_PROFILE_LUA_CLONE,
    OCG_PROFILE_LUA_COLLECT,
    OCG_PROFILE_LUA_COPY,
    OCG_PROFILE_LUA_FIX,
    OCG_PROFILE_PHASE_COUNT
};

enum OCG_CloneProfileCounter {
    OCG_PROFILE_CLONE_ATTEMPTS,
    OCG_PROFILE_CLONE_SUCCESSES,
    OCG_PROFILE_CORE_CARDS,
    OCG_PROFILE_CORE_EFFECTS,
    OCG_PROFILE_CORE_GROUPS,
    OCG_PROFILE_CORE_MAPPED_LOOKUPS,
    OCG_PROFILE_LUA_BLOCKS,
    OCG_PROFILE_LUA_BYTES,
    OCG_PROFILE_LUA_OBJECTS,
    OCG_PROFILE_LUA_TRANSLATIONS,
    OCG_PROFILE_LUA_EXTERNAL_LOOKUPS,
    OCG_PROFILE_COUNTER_COUNT
};

typedef struct OCG_CloneProfileRecord {
    uint64_t nanoseconds[OCG_PROFILE_PHASE_COUNT];
    uint64_t counters[OCG_PROFILE_COUNTER_COUNT];
} OCG_CloneProfileRecord;

#ifdef __cplusplus
#if defined(OCG_CLONE_PROFILING)
#include <chrono>
namespace ocg::clone_profile {
void count(OCG_CloneProfileCounter counter, uint64_t amount) noexcept;
class Scope {
    using Clock = std::chrono::steady_clock;
    OCG_CloneProfilePhase phase_;
    Clock::time_point started_;
    bool active_;
public:
    explicit Scope(OCG_CloneProfilePhase phase) noexcept;
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
}
#define OCG_PROFILE_SCOPE(phase) ocg::clone_profile::Scope profile_scope_##phase(phase)
#define OCG_PROFILE_COUNT(counter, value) ocg::clone_profile::count(counter, value)
#else
#define OCG_PROFILE_SCOPE(phase) do {} while(false)
#define OCG_PROFILE_COUNT(counter, value) do {} while(false)
#endif
#endif
