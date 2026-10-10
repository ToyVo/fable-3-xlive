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
