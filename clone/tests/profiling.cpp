// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ocgapi.h"
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition) {
    if(!condition) throw std::runtime_error("clone profiling accounting failed");
}
}

int main() {
    OCG_CloneProfileRecord result{};
    require(!OCG_CloneProfileEnd(nullptr));
    require(!OCG_CloneProfileEnd(&result));
    if(!OCG_CloneProfileEnabled()) {
        require(!OCG_CloneProfileBegin());
        OCG_PROFILE_COUNT(OCG_PROFILE_LUA_BYTES, 8);
        require(!OCG_CloneProfileEnd(&result));
        require(result.counters[OCG_PROFILE_LUA_BYTES] == 0);
        return 0;
    }
    require(OCG_CloneProfileBegin());
    require(!OCG_CloneProfileBegin());
    OCG_PROFILE_COUNT(OCG_PROFILE_LUA_BYTES, 11);
    std::thread independent([] {
        require(OCG_CloneProfileBegin());
        OCG_PROFILE_COUNT(OCG_PROFILE_LUA_BYTES, 7);
        OCG_CloneProfileRecord child{};
        require(OCG_CloneProfileEnd(&child));
        require(child.counters[OCG_PROFILE_LUA_BYTES] == 7);
    });
    independent.join();
    {
        OCG_PROFILE_SCOPE(OCG_PROFILE_CORE_METADATA);
        OCG_PROFILE_COUNT(OCG_PROFILE_CORE_CARDS, 3);
    }
    require(OCG_CloneProfileEnd(&result));
    require(result.counters[OCG_PROFILE_LUA_BYTES] == 11);
    require(result.counters[OCG_PROFILE_CORE_CARDS] == 3);
    require(OCG_CloneProfileBegin());
    require(OCG_CloneProfileEnd(&result));
    require(result.counters[OCG_PROFILE_LUA_BYTES] == 0);
    require(!OCG_CloneProfileEnd(&result));
}
