/* SPDX-License-Identifier: Apache-2.0 */
/* User/profile shims backed by parsed xlive.ini profiles.
 * Evidence: "xLiveLess" referenced from XUserGetName @ 0x10004cb0 and
 * XUserGetSigninInfo @ 0x10004e30; profile name/xuid strings feed these.
 * Semantics: offline success — player 1 always available with the INI name/xuid
 * (guide step 11: set `profile xuid 1` to your save folder id so saves load).
 */
#include "xlive.h"
#include <stdio.h>

DWORD WINAPI XUserGetName(DWORD userIndex, char *name, DWORD nameLen)
{
    EnsureInitialized();
    if (userIndex >= MAX_PROFILES || !name || !nameLen)
        return 1;
    strcpy_s(name, nameLen, g_live.profiles[userIndex].name);
    return 0;
}

DWORD WINAPI XUserGetXUID(DWORD userIndex, DWORD *xuidLow, DWORD *xuidHigh)
{
    EnsureInitialized();
    if (userIndex >= MAX_PROFILES)
        return 1;
    if (xuidLow)  *xuidLow  = g_live.profiles[userIndex].xuid;
    if (xuidHigh) *xuidHigh = 0; /* offline: high bits zero */
    return 0;
}

DWORD WINAPI XUserGetSigninState(DWORD userIndex)
{
    EnsureInitialized();
    if (userIndex >= MAX_PROFILES)
        return 0; /* not signed in */
    return g_live.profiles[userIndex].signin ? 1 : 0;
}

DWORD WINAPI XUserGetSigninInfo(DWORD userIndex, DWORD flags, void *info)
{
    (void)flags;
    EnsureInitialized();
    if (userIndex >= MAX_PROFILES || !info)
        return 1;
    TraceLine(L"XUserGetSigninInfo(%u)", userIndex);
    return 0;
}

DWORD WINAPI XUserCheckPrivilege(DWORD userIndex, DWORD priv, DWORD *has)
{
    (void)priv;
    EnsureInitialized();
    if (has) *has = (userIndex < MAX_PROFILES) ? 1 : 0; /* allow offline */
    return 0;
}

DWORD WINAPI XUserReadProfileSettings(DWORD a1, DWORD a2, DWORD a3, void *a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    return 0;
}

DWORD WINAPI XUserWriteProfileSettings(DWORD a1, DWORD a2, DWORD a3, void *a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    return 0;
}
