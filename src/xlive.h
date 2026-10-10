/* SPDX-License-Identifier: Apache-2.0 */
/* Unified xlive.dll — shared state and declarations.
 * Base: xLiveLess 2.0a4 behavior (FUN_100012d0 / FUN_10002550 / XLiveInitialize).
 * Timeslip extras: ShowMessages sink, fakexlive.ini fallback, LoaderPatch.
 */
#ifndef UNIFIED_XLIVE_H
#define UNIFIED_XLIVE_H

/* NOTE: winsock2.h must come before windows.h (or with WIN32_LEAN_AND_MEAN),
 * or both MSVC and mingw fail: winsock.h v1 vs winsock2.h redefinitions. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <stdint.h>
#include "version.h"

#define MAX_PROFILES 4
#define PROFILE_NAME_LEN 32
#define SAVE_DIR_LEN 260
#define INI_PRIMARY "xlive.ini"
#define INI_FALLBACK "c:\\xlive.ini"
#define FAKE_INI_SUFFIX "\\Lionhead Studios\\Fable 3\\fakexlive.ini"
#define TRACE_LOG "xlive_trace.log"
/* Log header, version interpolated from src/version.h (%hs = narrow string). */
#define TRACE_BANNER_FMT L"Log started (unified v%hs)\n"

typedef struct {
    char  name[PROFILE_NAME_LEN + 1];
    DWORD signin;
    DWORD xuid;      /* low 32 bits; high bits zero offline */
} LiveProfile;

typedef struct {
    int         initialized;
    CRITICAL_SECTION lock;
    LiveProfile profiles[MAX_PROFILES];
    int         onlineProfile;
    char        saveDir[SAVE_DIR_LEN];
    int         debugLog;
    int         showMessages;  /* Timeslip: 0 ignore, 1 MessageBox, 2 in-game/trace */
    int         loaderPatch;   /* Timeslip: 0 off (recommended), 1 launcher-check patch */
    HMODULE     self;
    HMODULE     game;          /* Fable3.exe module */
} LiveConfig;

extern LiveConfig g_live;

/* config.c */
void EnsureInitialized(void);

/* trace.c */
void TraceOpen(void);
void TraceLine(const wchar_t *fmt, ...);
int  DebugEnabled(void);

/* overlay.c (ShowMessages sink) */
void XliveMessage(const char *msg);

/* loader_patch.c */
void ApplyLoaderPatchIfEnabled(void);

/* content.c */
void RefreshDlcList(int contentNum);

/* generic success stub for binding surface (see README re: arity) */
DWORD WINAPI StubSuccess(void);

#endif
