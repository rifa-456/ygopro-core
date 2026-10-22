// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../../duel.h"
#include <memory>
#include <vector>
namespace ocg::persistence {
// Input images must come from the same linked engine build and trusted storage.
std::vector<unsigned char> save(duel &source);
std::unique_ptr<duel> load(std::vector<unsigned char> data, const OCG_CloneCallbacks &callbacks);
} // namespace ocg::persistence
