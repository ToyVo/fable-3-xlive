/* SPDX-License-Identifier: Apache-2.0 */
/* LoaderPatch — Timeslip launcher-check removal, default OFF, size-guarded.
 * Provenance: FUN_10009350 lines 58-64 + synchronized disassembly:
 *     0x100094c8: ADD ESI,0x21567d     (ESI = Fable3.exe base; the decompiler
 *     0x100094dc: MOV byte ptr [ESI],0x0  renders the same constant as
 *                                       &DAT_10017f44[0x8559f].unused + 1
 *                                       since 0x8559F*4+1 = 0x21567D)
 * Mechanism: VirtualProtect(base+0x21567D, 1, PAGE_EXECUTE_READWRITE) →
 * write single zero byte → restore protection.
 * Guard: only fires when the host exe is exactly the 1.1.1.3 build
 * (29,067,024 bytes per readme). Anything else → skip + trace warning.
 * WARNING (readme, verbatim spirit): test-only; triggers SecuROM checks
 * incl. forced low-detail mode. Leave LoaderPatch = 0 for real play.
 */
#include "xlive.h"
#include <stdio.h>

#define LOADER_PATCH_RVA 0x21567Du
#define FABLE3_1113_SIZE 29067024u

void ApplyLoaderPatchIfEnabled(void)
{
    DWORD oldProt;
    BYTE *target;
    char exePath[MAX_PATH];
    HANDLE h;
    LARGE_INTEGER size;

    if (!g_live.loaderPatch)
        return;
    if (!g_live.game) {
        XliveMessage("LoaderPatch: Fable3.exe module not found, skipping");
        return;
    }
    /* Size guard: resolve exe path from module, check exact byte size. */
    if (!GetModuleFileNameA(g_live.game, exePath, sizeof(exePath)))
        return;
    h = CreateFileA(exePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    size.QuadPart = 0;
    GetFileSizeEx(h, &size);
    CloseHandle(h);
    if ((unsigned long long)size.QuadPart != FABLE3_1113_SIZE) {
        TraceLine(L"LoaderPatch: exe size %lld != 1.1.1.3, skipping",
                  size.QuadPart);
        return;
    }
    target = (BYTE *)g_live.game + LOADER_PATCH_RVA;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProt))
        return;
    *target = 0;
    VirtualProtect(target, 1, oldProt, &oldProt);
    TraceLine(L"LoaderPatch applied at +0x21567D");
}
