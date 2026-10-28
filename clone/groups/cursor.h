// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../../group.h"

namespace ocg::groups {

/**
 * Returns the card a group's iterator points at, or null at the end or outside the container. Core processors such as
 * SwapControl, GetControl and SpSummonRule step a group's iterator across player decisions without clearing
 * is_iterator_dirty, so a copy must carry the iterator whatever that flag says. The iterator is located by comparison,
 * never dereferenced.
 */
inline card* cursor(const group& value) {
    for (auto it = value.container.begin(); it != value.container.end(); ++it)
        if (it == value.it)
            return *it;
    return nullptr;
}

}
