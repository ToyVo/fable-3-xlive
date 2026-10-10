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
    return (DWORD)WSAStartup(v, &wd);
}

DWORD WINAPI XWSACleanup(void) { return (DWORD)WSACleanup(); }

DWORD WINAPI XCreateSocket(int af, int type, int proto)
{
    EnsureInitialized();
    return (DWORD)socket(af, type, proto);
}

DWORD WINAPI XSocketClose(DWORD s) { return (DWORD)closesocket((SOCKET)s); }
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

DWORD WINAPI XNetStartup(void *a) { (void)a; EnsureInitialized(); return 0; }
DWORD WINAPI XNetCleanup(void) { return 0; }
DWORD WINAPI XOnlineStartup(void) { EnsureInitialized(); return 0; }
DWORD WINAPI XOnlineCleanup(void) { return 0; }
