/* SPDX-License-Identifier: Apache-2.0 */
/* XLiveInitialize @ 0x10003fd0 (exact, 9 lines):
 *   FUN_100012d0(); TraceLine(L"XLiveInitialize  (a1 = %X)"); return 0;
 * XLiveInitializeEx @ 0x10005430: same init path. XLiveUninitialize: teardown.
 */
#include "xlive.h"

DWORD WINAPI XLiveInitialize(void)
{
    EnsureInitialized();
    TraceLine(L"XLiveInitialize  (a1 = %X)", 0);
    return 0; /* S_OK — always "signed in" locally */
}

DWORD WINAPI XLiveInitializeEx(void *pXii)
{
    (void)pXii;
    EnsureInitialized();
    TraceLine(L"XLiveInitializeEx");
    return 0;
}

DWORD WINAPI XLiveUninitialize(void)
{
    TraceLine(L"XLiveUninitialize");
    return 0;
}

/* Signin/out: offline emu — mark player 1 signed in, never touch network. */
DWORD WINAPI XLiveSignin(void *a1, void *a2, DWORD a3, DWORD a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    g_live.profiles[0].signin = 1;
    TraceLine(L"XLiveSignin -> player1");
    return 0;
}

DWORD WINAPI XLiveSignout(void)
{
    EnsureInitialized();
    TraceLine(L"XLiveSignout");
    return 0;
}
