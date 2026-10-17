/* SPDX-License-Identifier: Apache-2.0 */
/* Binding surface: one shared success callee + narrow passthroughs.
 * See README ("Stdcall-arity honesty"). Timeslip-only ordinal 5339 (f217,
 * a return-0 stub in the reference DLL) gets its own export name.
 */
#include "xlive.h"
#include <stdlib.h>
#include <string.h>
#include <intrin.h>

DWORD WINAPI StubSuccess(void)
{
    EnsureInitialized();
    TraceLine(L"StubSuccess (uncategorized ordinal) caller=%p", _ReturnAddress());
    return 0;
}

/* Timeslip f007 @0x100095a0: 0 args (plain RET). D3D message overlay;
 * without the D3D hooks this build stays silent (see overlay.c).
 * Deliberately untraced: the game calls it every frame. */
DWORD WINAPI XLiveRender(void)
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
    TraceLine(L"XLiveSecureFreeLibrary");
    return (DWORD)FreeLibrary(mod);
}

/* PBuffer API (Timeslip f205/f206 @0x10009140/0x10009110,
 * f215 @0x10008f80): malloc'd block with 0x4D2 magic + size header. */
#define PBUFFER_MAGIC 0x4D2u

DWORD WINAPI XLivePBufferAllocate(DWORD size, void **out)
{
    DWORD *p;
    EnsureInitialized();
    TraceLine(L"XLivePBufferAllocate(%u)", size);
    if (!out)
        return 1;
    p = (DWORD *)malloc((size_t)size + 0x10);
    if (!p)
        return 1;
    *out = p;
    p[1] = size;
    p[0] = PBUFFER_MAGIC;
    return 0;
}

DWORD WINAPI XLivePBufferFree(void *buf)
{
    DWORD *p = (DWORD *)buf;
    EnsureInitialized();
    TraceLine(L"XLivePBufferFree");
    if (p && p[0] == PBUFFER_MAGIC)
        free(p);
    return 0;
}

DWORD WINAPI XLivePBufferGetByteArray(void *buf, int offset, void *dst, DWORD len)
{
    DWORD *p = (DWORD *)buf;
    EnsureInitialized();
    TraceLine(L"XLivePBufferGetByteArray");
    if (p && p[0] == PBUFFER_MAGIC && dst && offset >= 0 &&
        (DWORD)offset + len <= p[1] && (DWORD)offset + len >= (DWORD)offset)
        memcpy(dst, (BYTE *)p + offset + 0x10, len);
    return 0;
}

/* Protected-data API (Timeslip f207/f208 @0x100090d0/0x10009080,
 * f209/f210/f211 @0x10009060/0x10009050/0x10009040). Plain-memory
 * emulation: ProtectData copies, UnprotectData unwraps one DWORD. */
DWORD WINAPI XLiveProtectData(void *src, DWORD srcLen, void *dst,
                               DWORD *dstLen, DWORD a5)
{
    (void)a5;
    EnsureInitialized();
    TraceLine(L"XLiveProtectData(%u)", srcLen);
    if (!dstLen)
        return 1;
    if (dst && src && srcLen <= *dstLen)
        memcpy(dst, src, srcLen);
    *dstLen = srcLen;
    return 0;
}

DWORD WINAPI XLiveUnprotectData(void *src, DWORD a2, void *dst,
                                 DWORD *sizeInOut, DWORD *a5)
{
    (void)a2;
    EnsureInitialized();
    TraceLine(L"XLiveUnprotectData");
    if (!sizeInOut || !a5)
        return 0x80004005u;
    *a5 = 1;
    if (dst && src && *sizeInOut > 3) {
        *sizeInOut = 4;
        *(DWORD *)dst = *(DWORD *)src;
        return 0;
    }
    *sizeInOut = 4;
    return 0x8007007Au;
}

DWORD WINAPI XLiveCreateProtectedDataContext(DWORD a1, DWORD *out)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XLiveCreateProtectedDataContext");
    if (out)
        *out = 1;
    return 0;
}

DWORD WINAPI XLiveQueryProtectedDataInformation(DWORD a1, DWORD a2)
{
    (void)a1; (void)a2;
    EnsureInitialized();
    TraceLine(L"XLiveQueryProtectedDataInformation");
    return 0;
}

DWORD WINAPI XLiveCloseProtectedDataContext(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XLiveCloseProtectedDataContext");
    return 0;
}

/* XHV engine object (Timeslip f5008 @0x10009a00, 3 args). The game treats
 * *out as a pointer to an engine struct and calls the method at +0x40.
 * Timeslip's struct holds 0x10009a20 (DebugBreak trap, swallowed
 * breakpoint) in every slot except +0x40, which holds 0x10009a30
 * (`or eax,-1; ret 4`). We mirror that shape with real addresses. */
static DWORD WINAPI HvTrap(void)
{
    __debugbreak();
    return 0xFFFFFFFFu;
}

static DWORD WINAPI HvMethod16(DWORD a1)
{
    (void)a1;
    return 0xFFFFFFFFu;
}

#define HV_SLOTS 24
static DWORD (WINAPI *s_hvMethods[HV_SLOTS])(void);
static void *s_hvInner = NULL;

DWORD WINAPI XHVCreateEngine(DWORD a1, DWORD a2, void **out)
{
    int i;
    (void)a1; (void)a2;
    EnsureInitialized();
    for (i = 0; i < HV_SLOTS; i++)
        s_hvMethods[i] = HvTrap;
    s_hvMethods[16] = (DWORD (WINAPI *)(void))HvMethod16;
    s_hvInner = s_hvMethods;
    if (out)
        *out = &s_hvInner;
    TraceLine(L"XHVCreateEngine -> %p (hvInner=%p)",
              out ? *out : 0, (void *)&s_hvInner);
    return 0;
}

/* Timeslip f003 @0x10009320: 1 arg, return 0. */
DWORD WINAPI XGetOverlappedExtendedError(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XGetOverlappedExtendedError");
    return 0;
}

/* Timeslip f004 @0x10009300: 3 args, zero the result slot. */
DWORD WINAPI XGetOverlappedResult(DWORD a1, DWORD *a2, DWORD a3)
{
    (void)a1; (void)a3;
    EnsureInitialized();
    TraceLine(L"XGetOverlappedResult");
    if (a2)
        *a2 = 0;
    return 0;
}

DWORD WINAPI XCloseHandle(DWORD h)
{
    (void)h;
    EnsureInitialized();
    TraceLine(L"XCloseHandle");
    return 0;
}

DWORD WINAPI XCancelOverlapped(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XCancelOverlapped");
    return 0;
}

/* Timeslip f010 @0x100092c0: 1 arg, return 0. */
DWORD WINAPI XLivePreTranslateMessage(const void *msg)
{
    (void)msg;
    EnsureInitialized();
    TraceLine(L"XLivePreTranslateMessage");
    return 0;
}

/* Timeslip f5303 @0x10009910: 7 args, zero param 6, return -1. */
DWORD WINAPI XStringVerify(DWORD a1, DWORD a2, DWORD a3, DWORD a4,
                            DWORD a5, DWORD *a6, DWORD a7)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a7;
    EnsureInitialized();
    TraceLine(L"XStringVerify");
    if (a6)
        *a6 = 0;
    return 0xFFFFFFFFu;
}

/* Timeslip f219 @0x10009d10: 7 args, param 6 = -1. */
DWORD WINAPI XLiveContentCreateAccessHandle(DWORD a1, DWORD a2, DWORD a3,
                                             DWORD a4, DWORD a5, DWORD *a6,
                                             DWORD a7)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a7;
    EnsureInitialized();
    TraceLine(L"XLiveContentCreateAccessHandle");
    if (a6)
        *a6 = 0xFFFFFFFFu;
    return 0;
}

/* Timeslip f220 @0x10008f50: 6 args, return -1. */
DWORD WINAPI XMarketplaceCreateOfferEnumerator(DWORD a1, DWORD a2, DWORD a3,
                                                DWORD a4, DWORD a5, DWORD a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    EnsureInitialized();
    TraceLine(L"XMarketplaceCreateOfferEnumerator");
    return 0xFFFFFFFFu;
}

/* Timeslip f5376 @0x10009d00: 6 args, return 0xFFFF. */
DWORD WINAPI XMarketplaceCreateOfferEnumeratorByOffering(DWORD a1, DWORD a2,
                                                          DWORD a3, DWORD a4,
                                                          DWORD a5, DWORD a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    EnsureInitialized();
    TraceLine(L"XMarketplaceCreateOfferEnumeratorByOffering");
    return 0xFFFFu;
}

/* Timeslip f216 @0x10008f70: 2 args, return 1. */
DWORD WINAPI XInviteGetAcceptedInfo(DWORD a1, DWORD a2)
{
    (void)a1; (void)a2;
    EnsureInitialized();
    TraceLine(L"XInviteGetAcceptedInfo");
    return 1;
}

/* Timeslip f5313 @0x100098b0: 1 arg, return -1. */
DWORD WINAPI XPresenceInitialize(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XPresenceInitialize");
    return 0xFFFFFFFFu;
}
