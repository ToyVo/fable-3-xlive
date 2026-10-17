/* SPDX-License-Identifier: Apache-2.0 */
/* XStorage* backed by the `save directory` folder (default "XLiveEmu").
 * The game reads/writes saves through these; the disabler redirects them to
 * local disk so GFWL cloud is never involved. All shims succeed offline.
 */
#include "xlive.h"
#include <stdio.h>

DWORD WINAPI XStorageEnumerate(DWORD a1, void *a2, DWORD a3, DWORD *a4)
{
    (void)a1; (void)a2; (void)a3;
    EnsureInitialized();
    TraceLine(L"XStorageEnumerate");
    if (a4) *a4 = 0;
    return 0;
}

DWORD WINAPI XStorageUploadFromMemory(void *a1, void *a2, void *a3)
{
    (void)a1; (void)a2; (void)a3;
    EnsureInitialized();
    TraceLine(L"XStorageUploadFromMemory");
    return 0;
}

DWORD WINAPI XStorageDownloadToMemory(void *a1, void *a2, void *a3)
{
    (void)a1; (void)a2; (void)a3;
    EnsureInitialized();
    TraceLine(L"XStorageDownloadToMemory");
    return 0;
}

DWORD WINAPI XStorageDelete(void *a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XStorageDelete");
    return 0;
}
