/* SPDX-License-Identifier: Apache-2.0 */
/* Unified config loader.
 * Primary: xlive.ini next to exe, fallback c:\xlive.ini (xLiveLess behavior,
 * FUN_100012d0: fopen/fgets/feof line loop, strstr-prefix keys, sscanf %d/%s).
 * Migration aid: if neither exists, import [xlive] xuidl/xuidh/profile from
 * %APPDATA%\Lionhead Studios\Fable 3\fakexlive.ini (Timeslip layout,
 * FUN_10009350 semantics: hex-aware via strtoul base 0, 32-char profile cap).
 * Defaults: Player1..4 / XLiveEmu / online=0? (shipped INI uses online=1;
 * code default 0, shipped xlive.ini sets 1) / debug=0 / ShowMessages=2 /
 * LoaderPatch=0.
 */
#include "xlive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>

LiveConfig g_live;

static void ParseLine(char *line)
{
    static const char *signinKeys[MAX_PROFILES] =
        { "signin player 1 =", "signin player 2 =",
          "signin player 3 =", "signin player 4 =" };
    static const char *nameKeys[MAX_PROFILES] =
        { "profile name 1 =", "profile name 2 =",
          "profile name 3 =", "profile name 4 =" };
    static const char *xuidKeys[MAX_PROFILES] =
        { "profile xuid 1 =", "profile xuid 2 =",
          "profile xuid 3 =", "profile xuid 4 =" };
    char *eq;
    int i;

    for (i = 0; i < MAX_PROFILES; i++) {
        if (strstr(line, signinKeys[i]) == line) {
            eq = strchr(line, '=');
            if (eq) sscanf(eq + 1, "%lu", &g_live.profiles[i].signin); /* DWORD == unsigned long on MSVC and mingw */
            return;
        }
        if (strstr(line, nameKeys[i]) == line) {
            eq = strchr(line, '=');
            if (eq) sscanf(eq + 1, "%32s", g_live.profiles[i].name);
            return;
        }
        if (strstr(line, xuidKeys[i]) == line) {
            eq = strchr(line, '=');
            if (eq) {
                /* Save-folder ids are 16 HEX digits (a 64-bit XUID, e.g.
                 * 1000100010001000 = hi 10001000 / lo 10001000, matching
                 * Timeslip's xuidh/xuidl pair). Parse base 16 so the
                 * folder name pastes in verbatim (optional 0x ok).
                 * SigninInfo scales lo/hi by (index+1); saves key off
                 * both halves, so the split must be exact. */
                while (*++eq == ' ' || *eq == '\t')
                    ;
                {
                    unsigned long long v = strtoull(eq, NULL, 16);
                    g_live.profiles[i].xuid = (DWORD)v;
                    g_live.profiles[i].xuidHigh = (DWORD)(v >> 32);
                }
            }
            return;
        }
    }
    eq = strchr(line, '=');
    if (!eq) return;
    if (strstr(line, "online profile =") == line)
        sscanf(eq + 1, "%d", &g_live.onlineProfile);
    else if (strstr(line, "save directory =") == line)
        sscanf(eq + 1, "%259s", g_live.saveDir);
    else if (strstr(line, "debug log =") == line)
        sscanf(eq + 1, "%d", &g_live.debugLog);
    else if (strstr(line, "ShowMessages =") == line) {
        sscanf(eq + 1, "%d", &g_live.showMessages);
        if (g_live.showMessages > 2) g_live.showMessages = 2;
        if (g_live.showMessages < 0) g_live.showMessages = 0;
    } else if (strstr(line, "LoaderPatch =") == line)
        sscanf(eq + 1, "%d", &g_live.loaderPatch);
}

/* Static-buffered INI load (no CRT heap): reads the whole file with plain
 * Win32 calls and parses lines in place. Keeps boot allocation-free. */
static void ParseBuffer(char *buf)
{
    char *line = buf;
    for (;;) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        ParseLine(line);
        if (!nl) break;
        line = nl + 1;
    }
}

static int LoadIniFile(const char *path)
{
    HANDLE h;
    static char buf[4096];
    DWORD got = 0, total = 0;
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    while (total < sizeof(buf) - 1) {
        if (!ReadFile(h, buf + total, (DWORD)(sizeof(buf) - 1 - total),
                      &got, NULL) || got == 0)
            break;
        total += got;
    }
    CloseHandle(h);
    buf[total] = '\0';
    ParseBuffer(buf);
    return 1;
}

/* Timeslip fallback: hex-aware xuidl/xuidh + profile (FUN_10009350). */
static void ImportFakeXliveIni(void)
{
    char path[MAX_PATH * 2];
    char profile[PROFILE_NAME_LEN + 1];
    unsigned long lo, hi;

    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path);
    strcat_s(path, sizeof(path), FAKE_INI_SUFFIX);

    {
        /* Direct text parse to stay dependency-light (no shell32 INI APIs). */
        FILE *f = fopen(path, "r");
        char line[256];
        if (!f) return;
        lo = 0x10001000; hi = 0x10001000;
        profile[0] = '\0';
        while (!feof(f)) {
            if (!fgets(line, sizeof(line), f)) break;
            if (strstr(line, "xuidl") == line || strstr(line, "xuidl=")) {
                char *eq = strchr(line, '=');
                if (eq) lo = strtoul(eq + 1, NULL, 0);
            } else if (strstr(line, "xuidh") == line || strstr(line, "xuidh=")) {
                char *eq = strchr(line, '=');
                if (eq) hi = strtoul(eq + 1, NULL, 0);
            } else if (strstr(line, "profile") == line || strstr(line, "profile=")) {
                char *eq = strchr(line, '=');
                if (eq) sscanf(eq + 1, "%32s", profile);
            }
        }
        fclose(f);
    }
    g_live.profiles[0].xuid = (DWORD)lo;
    g_live.profiles[0].xuidHigh = (DWORD)hi;
    if (profile[0])
        strcpy_s(g_live.profiles[0].name, sizeof(g_live.profiles[0].name), profile);
}

void EnsureInitialized(void)
{
    if (g_live.initialized)
        return;
    g_live.initialized = 1;
    /* NOTE: no InitializeCriticalSection — g_live.lock is unused, and
     * skipping it keeps boot allocation-free (see LoadIniFile). */

    strcpy_s(g_live.profiles[0].name, sizeof(g_live.profiles[0].name), "Player1");
    strcpy_s(g_live.profiles[1].name, sizeof(g_live.profiles[1].name), "Player2");
    strcpy_s(g_live.profiles[2].name, sizeof(g_live.profiles[2].name), "Player3");
    strcpy_s(g_live.profiles[3].name, sizeof(g_live.profiles[3].name), "Player4");
    strcpy_s(g_live.saveDir, sizeof(g_live.saveDir), "XLiveEmu");
    g_live.showMessages = 2;
    g_live.loaderPatch = 0;

    g_live.game = GetModuleHandleA("Fable3.exe");

    if (!LoadIniFile(INI_PRIMARY) && !LoadIniFile(INI_FALLBACK))
        ImportFakeXliveIni();
    TraceOpen();
}
