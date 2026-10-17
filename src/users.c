/* SPDX-License-Identifier: Apache-2.0 */
/* User/profile shims backed by parsed xlive.ini profiles.
 * Evidence: "xLiveLess" referenced from XUserGetName @ 0x10004cb0 and
 * XUserGetSigninInfo @ 0x10004e30; profile name/xuid strings feed these.
 * Semantics: offline success — player 1 always available with the INI name/xuid
 * (guide step 11: set `profile xuid 1` to your save folder id so saves load).
 */
#include "xlive.h"
#include <stdio.h>
#include <string.h>
#include <intrin.h>
#include <shlobj.h>

DWORD WINAPI XUserGetName(DWORD userIndex, char *name, DWORD nameLen)
{
    size_t need;
    EnsureInitialized();
    if (userIndex >= MAX_PROFILES || !name || !nameLen)
        return 1;
    /* Timeslip f015 @0x10009250: -1 when the buffer cannot hold name+null. */
    need = strlen(g_live.profiles[userIndex].name) + 1;
    if (need > nameLen)
        return 0xFFFFFFFFu;
    strcpy_s(name, nameLen, g_live.profiles[userIndex].name);
    TraceLine(L"XUserGetName(%u) -> %S", userIndex, g_live.profiles[userIndex].name);
    return 0;
}

DWORD WINAPI XUserGetXUID(DWORD userIndex, DWORD *xuidLow, DWORD *xuidHigh)
{
    EnsureInitialized();
    TraceLine(L"XUserGetXUID(%u)", userIndex);
    if (userIndex >= MAX_PROFILES)
        return 1;
    if (xuidLow)  *xuidLow  = g_live.profiles[userIndex].xuid;
    if (xuidHigh) *xuidHigh = 0; /* offline: high bits zero */
    return 0;
}

DWORD WINAPI XUserGetSigninState(DWORD userIndex)
{
    (void)userIndex;
    EnsureInitialized();
    TraceLine(L"XUserGetSigninState(%u) -> 1", userIndex);
    /* Timeslip f014 @0x10009290: unconditional 1 (always signed in). */
    return 1;
}

/* XUSER_SIGNIN_INFO layout per Timeslip f016 @0x100091e0 (assembly):
 * lo@0, hi@4, signinState@8, guest@12, name@+0x18. flags==1 yields
 * zero xuids and no state/name fill. Note the name is at +24, not +16. */
DWORD WINAPI XUserGetSigninInfo(DWORD userIndex, DWORD flags, void *info)
{
    DWORD *d = (DWORD *)info;
    char *dstName;
    void *caller;
    EnsureInitialized();
    caller = _ReturnAddress();
    TraceLine(L"XUserGetSigninInfo(user=%u flags=%u info=%p caller=%p)",
              userIndex, flags, info, caller);
    if (userIndex >= MAX_PROFILES || !info)
        return 1;
    if (flags == 1) {
        d[0] = 0;
        d[1] = 0;
    } else {
        /* Timeslip f016: (index+1) * xuidl/xuidh, 32-bit wrap. */
        d[0] = (DWORD)((userIndex + 1) * (unsigned long long)g_live.profiles[userIndex].xuid);
        d[1] = (DWORD)((userIndex + 1) * (unsigned long long)g_live.profiles[userIndex].xuidHigh);
        d[2] = 1;
        d[3] = 1;
        /* Byte copy stopping at null (Timeslip f016 loop). NOTE: do NOT
         * use strncpy here: it pads to n bytes, but the game's buffer at
         * this site holds only ~40 bytes before the caller's /GS cookie. */
        dstName = (char *)info + 0x18;
        strcpy(dstName, g_live.profiles[userIndex].name);
    }
    TraceLine(L"XUserGetSigninInfo(%u)", userIndex);
    return 0;
}

DWORD WINAPI XUserCheckPrivilege(DWORD userIndex, DWORD priv, DWORD *has)
{
    (void)priv;
    EnsureInitialized();
    TraceLine(L"XUserCheckPrivilege(%u)", userIndex);
    if (has) *has = (userIndex < MAX_PROFILES) ? 1 : 0; /* allow offline */
    return 0;
}

/* Timeslip f217 @0x10008f60 (ordinal 5339): 9 args, return -1. */
DWORD WINAPI XUserReadProfileSettingsByXuid(DWORD a1, DWORD a2, DWORD a3,
                                            DWORD a4, DWORD a5, DWORD a6,
                                            DWORD a7, DWORD a8, DWORD a9)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    (void)a6; (void)a7; (void)a8; (void)a9;
    EnsureInitialized();
    TraceLine(L"XUserReadProfileSettingsByXuid");
    return 0xFFFFFFFFu;
}
/* Timeslip f5331 @0x100097d0: 7 args, always 0x7A. */
DWORD WINAPI XUserReadProfileSettings(DWORD a1, DWORD a2, DWORD a3, DWORD a4,
                                      DWORD a5, DWORD a6, DWORD a7)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6; (void)a7;
    EnsureInitialized();
    TraceLine(L"XUserReadProfileSettings -> 0x7A");
    return 0x7Au;
}

DWORD WINAPI XUserWriteProfileSettings(DWORD a1, DWORD a2, DWORD a3, void *a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    TraceLine(L"XUserWriteProfileSettings");
    return 0;
}

/* Timeslip f5276 @0x10009960: 4 args, return 0. */
DWORD WINAPI XUserSetProperty(DWORD a1, DWORD a2, DWORD a3, DWORD a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    TraceLine(L"XUserSetProperty");
    return 0;
}

/* Timeslip f214 @0x10008fd0: 3 args, return 0. */
DWORD WINAPI XUserSetContext(DWORD a1, DWORD a2, DWORD a3)
{
    (void)a1; (void)a2; (void)a3;
    EnsureInitialized();
    TraceLine(L"XUserSetContext");
    return 0;
}

/* Achievement persistence (Timeslip f018 @0x1000a1f0, 3 args).
 * Per entry (8 bytes, id first, array starts 4 bytes in): read the 8-byte
 * unlocked mask from
 * %APPDATA%\Lionhead Studios\Fable 3\<xuidh-as-%8x><xuidl-as-%8x>.achieve
 * (a missing file reads as zero), OR in the earned bit, write it back.
 * TSL matches ids against a 50-entry table for the bit position; that
 * table isn't recoverable statically, so bit = id (guarded < 64). The
 * game never reads the mask back through xlive, so only read-modify-
 * write consistency matters — which this preserves exactly. */
DWORD WINAPI XUserWriteAchievements(DWORD count, void *achievements, DWORD a3)
{
    DWORD *e;
    DWORD i;
    char dir[MAX_PATH];
    char path[MAX_PATH * 2];
    (void)a3;
    EnsureInitialized();
    TraceLine(L"XUserWriteAchievements(%u)", count);
    if (!achievements || count == 0)
        return 0;
    if (count > 1024)
        count = 1024;
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, dir) != 0)
        return 0;
    _snprintf_s(path, sizeof(path), _TRUNCATE,
                "%s\\Lionhead Studios\\Fable 3\\%8x%8x.achieve", dir,
                g_live.profiles[0].xuidHigh, g_live.profiles[0].xuid);
    e = (DWORD *)achievements + 1; /* first DWORD skipped, stride 2 */
    for (i = 0; i < count; i++, e += 2) {
        DWORD id = e[0];
        HANDLE h;
        DWORD got = 0;
        unsigned long long mask = 0;
        if (id >= 64)
            continue;
        h = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            ReadFile(h, &mask, sizeof(mask), &got, NULL);
            CloseHandle(h);
        }
        mask |= (1ULL << id);
        h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            WriteFile(h, &mask, sizeof(mask), &got, NULL);
            CloseHandle(h);
        }
    }
    return 0;
}

/* Achievement-enumerator preamble (Timeslip f5280 @0x10009d90, 9 args). */
DWORD WINAPI XUserCreateAchievementEnumerator(DWORD a1, DWORD a2, DWORD a3,
                                              DWORD a4, DWORD a5, DWORD a6,
                                              DWORD a7, DWORD *a8, DWORD *a9)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    EnsureInitialized();
    if (a8) *a8 = a7 * 0x24u;
    if (a9) *a9 = 0xFFFFFFFFu;
    TraceLine(L"XUserCreateAchievementEnumerator");
    return 0;
}

/* Timeslip f5282 @0x10009940: 6 args, return -1. */
DWORD WINAPI XUserReadGamerPicture(DWORD a1, DWORD a2, DWORD a3,
                                    DWORD a4, DWORD a5, DWORD a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    EnsureInitialized();
    TraceLine(L"XUserReadGamerPicture");
    return 0xFFFFFFFFu;
}
