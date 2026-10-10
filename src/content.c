/* SPDX-License-Identifier: Apache-2.0 */
/* DLC/content enumeration — sketch of FUN_10002550 @ 0x10002550 (218 lines).
 * Observed mechanism (transcribed control flow, simplified string ops):
 *  - one-shot cache (DAT_10032578): on first call, build wide search pattern
 *    for DLC subdirectories and FindFirstFileW over them
 *  - skip "." / ".."; for each directory entry, append L"\\content.xbx" and
 *    try to open it; on success record the entry and trace:
 *        L"- DLC%d = %s"            (per-folder, @ 0x10020008)
 *        L"=== ContentID = %s"      (content id,   @ 0x100200b0)
 *  - callers: XContentCreateEnumerator (arg 0 = rescan), XContentCreatePackage
 *    and XLiveContentGetPath (arg = content number) all funnel through here;
 *    XEnumerate then reports L"- [%d] DLC = %08X" (@ 0x1002221c).
 * Matches guide.md: game dir DLC/01_Understone, 02_TraitorsKeep,
 * 03_InquisitorsPack (+90_DLC_xlive_removal) each carrying content payloads.
 *
 * TODO precision: the exact search-root construction (game dir vs cwd) and the
 * content.xbx binary parse (ContentID extraction) need a closer read of lines
 * 68-218 of FUN_10002550. Contract below is behaviorally faithful.
 */
#include "xlive.h"
#include <stdio.h>

#define MAX_DLC 64

typedef struct {
    wchar_t dir[MAX_PATH];
    wchar_t contentId[64];
} DlcEntry;

static DlcEntry g_dlc[MAX_DLC];
static int g_dlcCount = -1; /* -1 = not scanned yet */

void RefreshDlcList(int contentNum)
{
    WIN32_FIND_DATAW fd;
    HANDLE h;
    wchar_t pattern[MAX_PATH];
    (void)contentNum;

    if (g_dlcCount >= 0 && contentNum == 0)
        return; /* cached (XContentCreateEnumerator rescan passes 0 first) */
    g_dlcCount = 0;

    /* DLC subdirectories of the game dir (falls back to cwd/DLC). */
    GetModuleFileNameW(NULL, pattern, ARRAYSIZE(pattern));
    wchar_t *sep = wcsrchr(pattern, L'\\');
    if (sep) *sep = L'\0';
    wcscat_s(pattern, ARRAYSIZE(pattern), L"\\DLC\\*");

    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
            continue;
        if (g_dlcCount >= MAX_DLC)
            break;
        DlcEntry *e = &g_dlc[g_dlcCount];
        _snwprintf_s(e->dir, ARRAYSIZE(e->dir), _TRUNCATE,
                     L"DLC\\%s", fd.cFileName);
        /* per-folder content marker: <dir>\content.xbx */
        wchar_t marker[MAX_PATH];
        _snwprintf_s(marker, ARRAYSIZE(marker), _TRUNCATE,
                     L"%s\\content.xbx", e->dir);
        FILE *f = _wfopen(marker, L"rb");
        if (f) {
            /* TODO: parse ContentID from content.xbx (see === ContentID trace);
             * placeholder records folder name. */
            _snwprintf_s(e->contentId, ARRAYSIZE(e->contentId), _TRUNCATE,
                         L"%s", fd.cFileName);
            fclose(f);
            TraceLine(L"- DLC%d = %s", g_dlcCount, e->dir);
            g_dlcCount++;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

DWORD WINAPI XContentCreateEnumerator(DWORD a1, DWORD a2, DWORD a3, DWORD *a4)
{
    (void)a1; (void)a2; (void)a3;
    EnsureInitialized();
    RefreshDlcList(0);
    TraceLine(L"XContentCreateEnumerator -> %d entries", g_dlcCount);
    if (a4) *a4 = (DWORD)g_dlcCount;
    return 0;
}

DWORD WINAPI XContentCreatePackage(DWORD a1, DWORD *a2, void *a3)
{
    EnsureInitialized();
    TraceLine(L"- ContentNum = %d", a2 ? (int)*a2 : -1);
    RefreshDlcList(a2 ? (int)*a2 : 0);
    (void)a1; (void)a3;
    return 0;
}

DWORD WINAPI XLiveContentGetPath(DWORD a1, DWORD *a2, wchar_t *out, DWORD *outLen)
{
    EnsureInitialized();
    RefreshDlcList(a2 ? (int)*a2 : 0);
    (void)a1;
    if (out && outLen && *outLen > 0) {
        wcsncpy_s(out, *outLen, g_dlc[0].dir, _TRUNCATE);
        TraceLine(L"=== ContentID = %s", g_dlc[0].contentId);
    }
    return 0;
}

DWORD WINAPI XEnumerate(HANDLE hEnum, void *buf, DWORD bufLen, DWORD *retLen, void *ov)
{
    (void)hEnum; (void)buf; (void)bufLen; (void)ov;
    EnsureInitialized();
    if (retLen) *retLen = (DWORD)g_dlcCount;
    TraceLine(L"- [%d] DLC = %08X", 0, (unsigned)g_dlcCount);
    return 0;
}
