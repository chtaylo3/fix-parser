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

## Submitting a release

The `version`, `repository`, and `id` change every release. Steps 1–3 below are
automated by the **Prepare nppPluginList submission** workflow.

1. **Fill the entries.** Run the *Prepare nppPluginList submission* workflow
   (Actions → Run workflow; leave `tag` blank for the latest release). It
   downloads the root-layout release zips, computes their SHA-256, writes
   `version` + `repository` + `id` into both entry files, validates them, and
   prints the paste-ready JSON in the run summary (also uploaded as an
   artifact). Tick `open_pr` to also get an in-repo PR refreshing these files.

   Or locally:

   ```sh
   python scripts/prepare-npppluginlist.py --version X.Y.Z \
       --x64-zip dist/FixParser_X.Y.Z_x64.zip \
       --x86-zip dist/FixParser_X.Y.Z_Win32.zip
   python scripts/validate-npppluginlist.py   # expect no warnings
   ```

2. **Smoke-test** the plugin via a debug Notepad++ (both architectures) — the
   upstream process expects this before the PR.

3. **Open the upstream PR.** Fork nppPluginList, add the `entry.x64.json` object
   into `src/pl.x64.json`'s `npp-plugins` array and `entry.x86.json` into
   `src/pl.x86.json`, and open the PR. For a first-time submission, confirm the
   `folder-name` is not already taken by another plugin.

## Refreshing the vendored schema

`pl.schema` is copied from nppPluginList `master`. When you refresh it, record
the upstream commit here and re-run the validator:

- Vendored from: `notepad-plus-plus/nppPluginList@master` (`pl.schema`).
- Last refreshed: 2026-06-19.
