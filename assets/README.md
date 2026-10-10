# Bundled DLC support files

`90_DLC_xlive_removal/` is the community "DLC xlive removal" stub pack
(originally `DLC_xlive_removal.zip`, fan-distributed): an empty `DLC.bnk` bank,
its `.dat` manifest, `package_collection_info.xmb`, and a `content.xbx` so the
game's DLC enumerator finds the `DLC\` folder itself. ~148 KB total, vendored
here so `scripts/fetch-dlc.*` works without chasing dead file links.

The three real DLC packs are **not** vendored (hundreds of MB, Microsoft
content) — the scripts download them from Microsoft's still-live Xbox CDN:

| Pack | Folder | CAB | Size | sha256 |
|---|---|---|---|---|
| Understone (free DLC) | `01_Understone` | `4d53090a0ecf0000.cab` | 256,943,166 B | `869a0147…97caae` |
| Traitor's Keep | `02_TraitorsKeep` | `4d53090a0ecf000e.cab` | 1,025,720,613 B | `3728c92b…d9f79` |
| Inquisitor's Pack | `03_InquisitorsPack` | `4d53090a0ecf0010.cab` | 1,546,685 B | `836b9446…136865` |

Total download ≈ 1.24 GB (resumable — rerun the script to continue).

See [docs/DLC.md](../docs/DLC.md).
