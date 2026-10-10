/* SPDX-License-Identifier: Apache-2.0 */
/* DllMain: DisableThreadLibraryCalls + init + optional LoaderPatch.
 * (Timeslip FUN_1000a390 shape, init upgraded to unified EnsureInitialized.)
 */
#include "xlive.h"

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    (void)hInst;
    if (reason == DLL_PROCESS_ATTACH) {
        g_live.self = GetModuleHandleA("xlive.dll");
        DisableThreadLibraryCalls(g_live.self);
        EnsureInitialized();
        ApplyLoaderPatchIfEnabled();
    }
    return TRUE;
}
