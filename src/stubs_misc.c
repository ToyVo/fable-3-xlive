/* SPDX-License-Identifier: Apache-2.0 */
/* Binding surface: one shared success callee + narrow passthroughs.
 * See README ("Stdcall-arity honesty"). Timeslip-only ordinal 5339 (f217,
 * a return-0 stub in the reference DLL) gets its own export name.
 */
#include "xlive.h"

DWORD WINAPI StubSuccess(void)
{
    EnsureInitialized();
    return 0;
}

/* Load-library shims: pass through to the real loader so game modules resolve.
 * Single-arg shape is the common case; anything fancier stays a StubSuccess
 * alias in the .def until traced. */
DWORD WINAPI XLiveSecureLoadLibraryW(const wchar_t *path)
{
    EnsureInitialized();
    TraceLine(L"XLiveSecureLoadLibraryW(%s)", path ? path : L"(null)");
    return (DWORD)(uintptr_t)LoadLibraryW(path);
}

DWORD WINAPI XLiveSecureFreeLibrary(HMODULE mod)
{
    EnsureInitialized();
    return (DWORD)FreeLibrary(mod);
}
