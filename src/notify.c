/* SPDX-License-Identifier: Apache-2.0 */
/* Notification shims. Arity/return values replicate Timeslip v0.11
 * (sha256 7dd7b0a2…): f001 @0x10009340 (4 args, ret 0),
 * f002 @0x10009330 (1 arg, ret 0), f017 @0x100091d0 (2 args, ret 1).
 * The game binds these by ordinal, so stdcall stack cleanup must match. */
#include "xlive.h"

DWORD WINAPI XNotifyGetNext(DWORD a1, DWORD a2, DWORD a3, DWORD a4)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    EnsureInitialized();
    TraceLine(L"XNotifyGetNext");
    return 0;
}

DWORD WINAPI XNotifyPositionUI(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XNotifyPositionUI");
    return 0;
}

DWORD WINAPI XNotifyCreateListener(DWORD a1, DWORD a2)
{
    (void)a1; (void)a2;
    EnsureInitialized();
    TraceLine(L"XNotifyCreateListener");
    return 1;
}
