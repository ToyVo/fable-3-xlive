/* SPDX-License-Identifier: Apache-2.0 */
/* ShowMessages sink (Timeslip contract, D3DX-free).
 * 0 = ignore; 1 = MessageBox (never fullscreen); 2 = in-game overlay in the
 * original — without the D3D device hooks we log to xlive_trace.log instead
 * (needs debug log = 1) and otherwise stay silent. Rationale: adding a
 * D3DX9_42 dependency would reintroduce a redist requirement modern systems
 * lack; the trace log covers the troubleshooting need.
 */
#include "xlive.h"

void XliveMessage(const char *msg)
{
    if (!msg || g_live.showMessages == 0)
        return;
    if (g_live.showMessages == 1) {
        MessageBoxA(NULL, msg, "xlive message", MB_OK | MB_TOPMOST);
        return;
    }
    TraceLine(L"xlive message: %S", msg);
}

/* Timeslip f011 @0x100092b0: 1 arg, return 1. */
DWORD WINAPI XShowGuideUI(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XShowGuideUI");
    return 1;
}

/* Timeslip f5216 @0x100099e0: 8 args, return 0. */
DWORD WINAPI XShowKeyboardUI(DWORD a1, DWORD a2, DWORD a3, DWORD a4,
                              DWORD a5, DWORD a6, DWORD a7, DWORD a8)
{
    (void)a1; (void)a2; (void)a3; (void)a4;
    (void)a5; (void)a6; (void)a7; (void)a8;
    EnsureInitialized();
    TraceLine(L"XShowKeyboardUI");
    return 0;
}

/* Timeslip f013 @0x100092a0: 2 args, return 0. */
DWORD WINAPI XShowSigninUI(DWORD a1, DWORD a2)
{
    (void)a1; (void)a2;
    EnsureInitialized();
    TraceLine(L"XShowSigninUI");
    return 0;
}

/* Timeslip f213 @0x10008fe0: 9 args; text is param 3.
 * ShowMessages==1 pops a MessageBox, ==2 traces (no D3D hooks here). */
DWORD WINAPI XShowMessageBoxUI(DWORD a1, DWORD a2, LPCWSTR text,
                               DWORD a4, DWORD a5, DWORD a6,
                               DWORD a7, DWORD a8, DWORD a9)
{
    (void)a1; (void)a2; (void)a4; (void)a5; (void)a6;
    (void)a7; (void)a8; (void)a9;
    EnsureInitialized();
    if (!text)
        return 0;
    if (g_live.showMessages == 1) {
        MessageBoxW(NULL, text, L"xlive message", MB_OK | MB_TOPMOST);
        return 0;
    }
    if (g_live.showMessages == 2)
        TraceLine(L"xlive message: %s", text);
    TraceLine(L"XShowMessageBoxUI");
    return 0;
}

/* Timeslip f108 @0x10009180: 5 args, return 1. */
DWORD WINAPI XShowMarketplaceUI(DWORD a1, DWORD a2, DWORD a3, DWORD a4, DWORD a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    EnsureInitialized();
    TraceLine(L"XShowMarketplaceUI");
    return 1;
}

/* Timeslip f202 @0x100094f0: 2 args, return 0. The overlay-rect math feeds
 * the D3D message overlay, which this build omits (see XliveMessage). */
DWORD WINAPI XLiveOnCreateDevice(DWORD a1, DWORD a2)
{
    (void)a1; (void)a2;
    EnsureInitialized();
    TraceLine(L"XLiveOnCreateDevice");
    return 0;
}

/* Timeslip f204 @0x10009170: 1 arg, return 0. */
DWORD WINAPI XLiveOnResetDevice(DWORD a1)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XLiveOnResetDevice");
    return 0;
}
