# nppPluginList submission entries

These files prepare FixParser's submission to the official Notepad++ plugin
registry, [notepad-plus-plus/nppPluginList](https://github.com/notepad-plus-plus/nppPluginList).
They are **not** consumed by the build or by Notepad++ — they exist so CI can
pre-flight the submission and so the upstream PR is copy-paste.

## Files

| File | Purpose |
|------|---------|
| `pl.schema` | Vendored copy of upstream's plugin-list JSON Schema. Source of truth for the validation rules. |
| `entry.x64.json` | The single plugin object to paste into upstream `pl.x64.json` (the `npp-plugins` array). |
| `entry.x86.json` | The single plugin object to paste into upstream `pl.x86.json`. |

There is no ARM64 entry because we do not ship an ARM64 build.

## What CI checks (offline)

`scripts/validate-npppluginlist.py` runs in the `nppPluginList entry lint` CI
job. It is stdlib-only (no pip, no network) and:

1. Validates each entry against `pl.schema` (`definitions/plugin`) — version
   format, required fields, `id` shape, `repository`/`homepage` URI format.
2. Asserts `folder-name == "FixParser"` (must equal the DLL base name).
3. Asserts `repository` is the correct release-asset URL for the entry's own
   `version` and architecture token (`x64` / `Win32`).
4. **Warns** (does not fail) when the entry `version` trails
   `.release-please-manifest.json`, or when `id` is still the placeholder.

The authoritative validator still runs on the upstream PR — this is a fast
pre-flight, not a replacement.

## Submitting a release (the manual step)

The `version`, `repository`, and `id` change every release. Before opening the
upstream PR for version `X.Y.Z`:

1. Bump `version` to `X.Y.Z` and update both `repository` URLs to the
   `vX.Y.Z` release assets in each entry file.
2. Compute the SHA-256 of each **root-layout** zip (`FixParser_X.Y.Z_x64.zip`
   and `FixParser_X.Y.Z_Win32.zip` — *not* the `_portable` ones). The release
   job already prints these; or locally: `Get-FileHash <zip> -Algorithm SHA256`.
   Paste each lowercase hash into the matching entry's `id`.
3. Re-run `python scripts/validate-npppluginlist.py` — it should report no
   warnings.
4. Fork nppPluginList, add `entry.x64.json` into `pl.x64.json`'s `npp-plugins`
   array and `entry.x86.json` into `pl.x86.json`, and open the PR. Confirm the
   `folder-name` is not already taken by another plugin first.

## Refreshing the vendored schema

`pl.schema` is copied from nppPluginList `master`. When you refresh it, record
the upstream commit here and re-run the validator:

- Vendored from: `notepad-plus-plus/nppPluginList@master` (`pl.schema`).
- Last refreshed: 2026-06-19.
