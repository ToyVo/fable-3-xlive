#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Fail CI unless the built DLL exports the full pinned surface.

Usage: python scripts/check_exports.py <xlive.dll>
Expects every ordinal from src/xlive.def to be present under the right name.
"""
import re
import sys

import pefile

DEF = "src/xlive.def"
NEED = {
    "XWSAStartup": 1,
    "XLiveInitialize": 5000,
    "XLiveInitializeEx": 5297,
    "XUserGetName": 5263,
    "XLiveContentGetPath": 5355,
}

dll = sys.argv[1]
exps = {e.ordinal: (e.name.decode() if e.name else None)
        for e in pefile.PE(dll).DIRECTORY_ENTRY_EXPORT.symbols}

wanted = {}
for line in open(DEF):
    line = line.strip()
    if not line or line.startswith(";") or line.startswith("LIBRARY") \
            or line.startswith("EXPORTS"):
        continue
    m = re.match(r"(\S+?)(?:=\S+)?\s+@(\d+)", line)
    if m:
        wanted[int(m.group(2))] = m.group(1)

missing = {o: n for o, n in wanted.items() if exps.get(o) != n}
print(f"exports: {len(exps)} present, {len(wanted)} pinned in xlive.def")
if missing:
    for o, n in sorted(missing.items()):
        print(f"  MISMATCH ord {o}: want {n!r}, got {exps.get(o)!r}")
    sys.exit(1)
for n, o in NEED.items():
    assert exps.get(o) == n, f"spot check failed: {n}"
print("OK: full pinned surface present")
