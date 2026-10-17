/* SPDX-License-Identifier: Apache-2.0 */
/* LoaderPatch — Timeslip launcher-check removal, default OFF, size-guarded.
 * Provenance: FUN_10009350 lines 58-64 + synchronized disassembly:
 *     0x100094c8: ADD ESI,0x21567d     (ESI = Fable3.exe base; the decompiler
 *     0x100094dc: MOV byte ptr [ESI],0x0  renders the same constant as
 *                                       &DAT_10017f44[0x8559f].unused + 1
 *                                       since 0x8559F*4+1 = 0x21567D)
 * The byte is the displacement of `je` at Fable3.exe+0x21567C: zeroing it
 * neuters the Wrong-Executable branch (fall-through to normal startup).
 * Mechanism: VirtualProtect(base+0x21567D, 1, PAGE_EXECUTE_READWRITE) →
 * write single zero byte → restore protection.
 * Guard: only fires when the host exe is exactly the 1.1.1.3 build
 * (29,067,024 bytes per readme). Anything else → skip + trace warning.
 * Modes: 1 = permanent patch (Timeslip behavior). SecuROM's later
 * integrity scans spot the modified byte and answer with the "pirated
 * copy" notice (forced lowest detail + disabled functionality).
 * 2 = self-healing (recommended for direct launch): patch at load, then
 * restore the original byte on the first XLiveInitialize[Ex] — which the
 * game provably calls only after the boot gate has consumed the patch.
 * Later SecuROM scans then see pristine bytes and stay quiet.
 * 3 = trampoline (experimental): hook the check to skip past the gate
 * without touching the patch byte at all (self-removing hook, so .text
 * is pristine again immediately). Falls back to mode 2 if the
 * check-site bytes don't match.
 * WARNING (readme, verbatim spirit): test-only; triggers SecuROM checks
 * incl. forced low-detail mode. Leave LoaderPatch = 0 for real play.
 */
#include "xlive.h"
#include <stdio.h>
#include <string.h>

#define LOADER_PATCH_RVA 0x21567Du
#define FABLE3_1113_SIZE 29067024u
#define LOADER_PATCH_EXPECT 0x68u

static BYTE *s_loaderTarget = NULL;
static BYTE s_loaderOrig = 0;
static int s_loaderArmed = 0;

/* Mode-3 trampoline state (all runtime absolute addresses). */
static BYTE *s_hookEntry = NULL;  /* Fable3.exe+0x215676 (cmp), 6 bytes */
static BYTE s_hookOrig[6];
static BYTE *s_hookFall = NULL;   /* entry+8 (startup fall-through) */
static int s_hookInstalled = 0;

static void PatchByteOffRemoveHook(void);
static int InstallCheckHook(void);

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
    if (*target != LOADER_PATCH_EXPECT) {
        TraceLine(L"LoaderPatch: byte at +0x21567D is %02X, not 68; skipping",
                  (unsigned)*target);
        return;
    }
    /* Mode 3: hook the check to skip the gate without touching any game
     * bytes long-term (self-removing). On install failure, fall through
     * to the mode-2 direct patch. */
    if (g_live.loaderPatch == 3 && InstallCheckHook())
        return;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProt))
        return;
    s_loaderOrig = *target;
    *target = 0;
    VirtualProtect(target, 1, oldProt, &oldProt);
    s_loaderTarget = target;
    /* Arm self-heal only for mode 2; mode 1 keeps Timeslip's permanent
     * patch. Armed restore fires on first XLiveInitialize[Ex]. */
    s_loaderArmed = (g_live.loaderPatch == 2);
    TraceLine(L"LoaderPatch applied at +0x21567D%s",
              s_loaderArmed ? L" (self-heal armed)" : L"");
}

/* Mode-3 entry stub (naked): remove the hook (restoring pristine bytes),
 * then resume at the startup fall-through. The patch byte is never
 * touched: control flow alone bypasses the gate, so no scan can catch
 * a modified byte afterwards. The dialog path stays reachable only if
 * the check re-runs later through unhooked code (fail-safe). */
__declspec(naked) static void CheckStub(void)
{
    __asm {
        pushad
        pushfd
        call PatchByteOffRemoveHook
        popfd
        popad
        mov eax, s_hookFall
        jmp eax
    }
}

/* Runs first inside the stub (flags/regs saved): remove the entry hook so
 * .text is pristine again, whatever happens next. Plain C so the naked
 * stub above stays simple. */
static void PatchByteOffRemoveHook(void)
{
    DWORD oldProt;
    if (s_hookInstalled && s_hookEntry) {
        s_hookInstalled = 0;
        if (VirtualProtect(s_hookEntry, 6, PAGE_EXECUTE_READWRITE, &oldProt)) {
            memcpy(s_hookEntry, s_hookOrig, 6);
            VirtualProtect(s_hookEntry, 6, oldProt, &oldProt);
            FlushInstructionCache(GetCurrentProcess(), s_hookEntry, 6);
        }
    }
}

/* Install the mode-3 trampoline. Returns nonzero on success. Verifies the
 * expected bytes first; on any mismatch the caller falls back to the
 * mode-2 direct patch. */
static int InstallCheckHook(void)
{
    BYTE *e;
    DWORD oldProt;
    BYTE jmp[5];
    int rel;
    if (!g_live.game)
        return 0;
    e = (BYTE *)g_live.game + 0x215676u;
    /* Match shape only (38 1D ?? ?? ?? ?? 74 68): the flag operand is
     * loader/SecuROM-rewritten at runtime, so only the opcode shape
     * is asserted here. */
    __try {
        if (e[0] != 0x38 || e[1] != 0x1D || e[6] != 0x74 || e[7] != 0x68) {
            TraceLine(L"LoaderPatch: check-site shape mismatch (have %02X%02X..%02X%02X), no hook",
                      e[0], e[1], e[6], e[7]);
            return 0;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        TraceLine(L"LoaderPatch: check-site unreadable, no hook");
        return 0;
    }
    s_hookEntry = e;
    s_hookFall = e + 8;
    memcpy(s_hookOrig, e, 6);
    jmp[0] = 0xE9;
    rel = (int)((BYTE *)(void *)CheckStub - (e + 5));
    memcpy(jmp + 1, &rel, 4);
    if (!VirtualProtect(e, 6, PAGE_EXECUTE_READWRITE, &oldProt))
        return 0;
    memcpy(e, jmp, 5);
    e[5] = 0x90;
    VirtualProtect(e, 6, oldProt, &oldProt);
    FlushInstructionCache(GetCurrentProcess(), e, 6);
    s_hookInstalled = 1;
    TraceLine(L"LoaderPatch mode 3 trampoline armed");
    return 1;
}

/* Restore the patched byte once the boot gate has consumed it. Idempotent:
 * only the first call (while armed) touches memory. Called from
 * XLiveInitialize[Ex], which the game provably reaches only after the
 * launcher check has run. */
void RestoreLoaderPatch(void)
{
    DWORD oldProt;
    if (!s_loaderArmed || !s_loaderTarget)
        return;
    s_loaderArmed = 0;
    if (!VirtualProtect(s_loaderTarget, 1, PAGE_EXECUTE_READWRITE, &oldProt))
        return;
    *s_loaderTarget = s_loaderOrig;
    VirtualProtect(s_loaderTarget, 1, oldProt, &oldProt);
    TraceLine(L"LoaderPatch reverted at +0x21567D");
}
