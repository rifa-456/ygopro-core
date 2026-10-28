// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../../duel.h"
#include <memory>
#include <vector>
namespace ocg::persistence {
/** Returns an image of an awaiting duel, which load turns back into a duel. */
std::vector<unsigned char> save(duel &source);
/** Rebuilds a duel from an image save wrote; the image must come from the same engine build and trusted storage. */
std::unique_ptr<duel> load(std::vector<unsigned char> data, const OCG_CloneCallbacks &callbacks);
} // namespace ocg::persistence
