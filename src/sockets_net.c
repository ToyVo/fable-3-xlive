/* SPDX-License-Identifier: Apache-2.0 */
/* Network/socket shims. Evidence: imports WS2_32.DLL:12; exports full XSocket*
 * family (CreateSocket/Close/Shutdown/IOCTLSocket/SetSockOpt/GetSockOpt +
 * WSA event/select/recv/send set) and XNet* family. xLiveLess does not emulate
 * LIVE servers — these are thin wrappers over real WinSock so system-link-ish
 * calls don't crash, plus deterministic stubs (HTONL/HTONS/NTOHL/NTOHS,
 * INET_ADDR, GetLastError) implemented locally. XWSACleanup/XWSAStartup pair
 * with the real ones.
 */
#include "xlive.h"
#include <winsock2.h>

DWORD WINAPI XWSAStartup(WORD v, void *data)
{
    WSADATA wd;
    (void)data;
    EnsureInitialized();
    TraceLine(L"XWSAStartup");
    return (DWORD)WSAStartup(v, &wd);
}

DWORD WINAPI XWSACleanup(void)
{
    TraceLine(L"XWSACleanup");
    return (DWORD)WSACleanup();
}

DWORD WINAPI XCreateSocket(int af, int type, int proto)
{
    EnsureInitialized();
    TraceLine(L"XCreateSocket");
    return (DWORD)socket(af, type, proto);
}

DWORD WINAPI XSocketClose(DWORD s)
{
    TraceLine(L"XSocketClose");
    return (DWORD)closesocket((SOCKET)s);
}
DWORD WINAPI XSocketShutdown(DWORD s, int how)
{
    return (DWORD)shutdown((SOCKET)s, how);
}
DWORD WINAPI XSocketIOCTLSocket(DWORD s, long cmd, u_long *arg)
{
    return (DWORD)ioctlsocket((SOCKET)s, cmd, arg);
}
DWORD WINAPI XSocketSetSockOpt(DWORD s, int lvl, int opt, const char *val, int len)
{
    return (DWORD)setsockopt((SOCKET)s, lvl, opt, val, len);
}
DWORD WINAPI XSocketGetSockOpt(DWORD s, int lvl, int opt, char *val, int *len)
{
    return (DWORD)getsockopt((SOCKET)s, lvl, opt, val, len);
}
DWORD WINAPI XSocketHTONL(DWORD v) { return (DWORD)htonl((u_long)v); }
DWORD WINAPI XSocketHTONS(DWORD v) { return (DWORD)htons((u_short)v); }
DWORD WINAPI XSocketNTOHL(DWORD v) { return (DWORD)ntohl((u_long)v); }
DWORD WINAPI XSocketNTOHS(DWORD v) { return (DWORD)ntohs((u_short)v); }

/* Timeslip f51 @0x10009b20: 1 arg, always -1 (offline, no system link). */
DWORD WINAPI XNetStartup(void *a) { (void)a; EnsureInitialized(); TraceLine(L"XNetStartup"); return 0xFFFFFFFFu; }
DWORD WINAPI XNetCleanup(void)
{
    TraceLine(L"XNetCleanup");
    return 0;
}
/* Timeslip f021 @0x100091c0: XOR EAX,EAX; RET — 0 args. */
DWORD WINAPI XOnlineStartup(void) { EnsureInitialized(); TraceLine(L"XOnlineStartup"); return 0; }
DWORD WINAPI XOnlineCleanup(void)
{
    TraceLine(L"XOnlineCleanup");
    return 0;
}

/* Timeslip f64 @0x10009a90: zero 2 dwords, return 10022. */
DWORD WINAPI XNetXnAddrToMachineId(DWORD a1, DWORD *out)
{
    (void)a1;
    EnsureInitialized();
    TraceLine(L"XNetXnAddrToMachineId");
    if (out) { out[0] = 0; out[1] = 0; }
    return 0x2726u;
}

/* Timeslip f73 @0x10009a40: *out = loopback-ish, return 4. */
DWORD WINAPI XNetGetTitleXnAddr(DWORD *out)
{
    EnsureInitialized();
    TraceLine(L"XNetGetTitleXnAddr");
    if (out) *out = 0x100007Fu;
    return 4;
}
