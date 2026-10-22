// SPDX-License-Identifier: AGPL-3.0-or-later
#include "vm.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace ocg::persistence {
std::uintptr_t image_base(const void *pointer) {
#if defined(_WIN32)
    HMODULE module{};
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(pointer), &module))
        throw std::invalid_argument("snapshot pointer is outside a loaded module");
    return reinterpret_cast<std::uintptr_t>(module);
#else
    Dl_info info{};
    if (!dladdr(pointer, &info) || !info.dli_fbase)
        throw std::invalid_argument("snapshot pointer is outside a loaded module");
    return reinterpret_cast<std::uintptr_t>(info.dli_fbase);
#endif
}
} // namespace ocg::persistence
