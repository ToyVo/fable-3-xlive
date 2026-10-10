/* SPDX-License-Identifier: Apache-2.0 */
/* xlive_trace.log writer. Evidence: "xlive_trace.log" @ 0x1001ff28 region,
 * FUN_100011c0(wchar_t*) trace sink called from init/content/XLive* paths,
 * "debug log =" key in shipped xlive.ini (0 = off).
 */
#include "xlive.h"
#include <stdio.h>
#include <stdarg.h>

static FILE *g_trace;

int DebugEnabled(void) { return g_live.debugLog != 0; }

void TraceOpen(void)
{
    if (!DebugEnabled())
        return;
    _wfopen_s(&g_trace, L"xlive_trace.log", L"a,ccs=UTF-16LE");
    if (g_trace)
        fwprintf(g_trace, TRACE_BANNER_FMT, XLIVE_UNIFIED_VERSION);
}

void TraceLine(const wchar_t *fmt, ...)
{
    va_list ap;
    if (!DebugEnabled() || !g_trace)
        return;
    va_start(ap, fmt);
    vfwprintf(g_trace, fmt, ap);
    fputws(L"\n", g_trace);
    va_end(ap);
    fflush(g_trace);
}

void TraceLineA(const char *fmt, ...)
{
    va_list ap;
    if (!DebugEnabled() || !g_trace)
        return;
    va_start(ap, fmt);
    vfprintf(g_trace, fmt, ap);
    fputws(L"\n", g_trace);
    va_end(ap);
    fflush(g_trace);
}
