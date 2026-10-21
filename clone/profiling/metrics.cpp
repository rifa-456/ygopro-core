// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../../ocgapi.h"
#include "metrics.h"

#if defined(OCG_CLONE_PROFILING)
namespace {
thread_local OCG_CloneProfileRecord record{};
thread_local bool collecting = false;
}

namespace ocg::clone_profile {
Scope::Scope(OCG_CloneProfilePhase phase) noexcept : phase_(phase), active_(collecting) {
    if(active_) started_ = Clock::now();
}
Scope::~Scope() {
    if(active_ && collecting) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - started_);
        record.nanoseconds[phase_] += static_cast<uint64_t>(elapsed.count());
    }
}
void count(OCG_CloneProfileCounter counter, uint64_t amount) noexcept {
    if(collecting) record.counters[counter] += amount;
}
}
#endif

int OCG_CloneProfileEnabled(void) {
#if defined(OCG_CLONE_PROFILING)
    return 1;
#else
    return 0;
#endif
}

int OCG_CloneProfileBegin(void) {
#if defined(OCG_CLONE_PROFILING)
    if(collecting) return 0;
    record = {};
    collecting = true;
    return 1;
#else
    return 0;
#endif
}

int OCG_CloneProfileEnd(OCG_CloneProfileRecord* output) {
    if(!output) return 0;
    *output = {};
#if defined(OCG_CLONE_PROFILING)
    if(!collecting) return 0;
    *output = record;
    collecting = false;
    return 1;
#else
    return 0;
#endif
}
