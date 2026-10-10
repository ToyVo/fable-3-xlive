/* SPDX-License-Identifier: Apache-2.0 */
/* Single source of truth for the release version.
 * Consumers (do not duplicate the string anywhere else):
 *  - src/xlive.h TRACE_BANNER_FMT → compiled into the DLL log header
 *  - flake.nix packages.xlive-unified → parses this file for `version`
 *  - .github/workflows/release.yml → tags/releases v<version> on dispatch
 * Bump the string, commit, push, then run the release workflow. */
#ifndef UNIFIED_XLIVE_VERSION_H
#define UNIFIED_XLIVE_VERSION_H

#define XLIVE_UNIFIED_VERSION "0.1.0"

#endif
