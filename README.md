# FIX Parser — Notepad++ Plugin

Turn unreadable FIX wire captures into scannable, one-message-per-line text — and
inspect any message's fields without leaving Notepad++.

> **Pretty-print is the headline feature.** A 50 MB single-line capture becomes a
> scrollable, line-addressable log in one command, regardless of which delimiter
> convention the log uses.

## Features

- **Pretty-print FIX log** — normalizes any common delimiter convention and puts
  one FIX message per line in a single undoable edit.
- **Multi-delimiter input** — accepts literal SOH (`0x01`), pipe (`|`), caret-A
  (`^A`), and the `\u0001` escape, with automatic detection and an alert when
  detection is ambiguous.
- **Hover quickview** — hover a FIX line to see a parsed summary in a calltip.
- **Dock + history** — double-click a FIX line to open a dockable panel with the
  full field breakdown and a lookback history of inspected messages.
- **QuickFIX dictionaries** — tag names, types, and enum descriptions resolved
  per message with automatic version detection (FIX.4.0–FIX.5.0 SP2 / FIXT.1.1).

## Pretty-print quick start

1. Open a FIX log (any delimiter convention).
2. **Plugins → FIX Parser → Pretty-print FIX log**.
3. Each message is reformatted to `tag=value|tag=value|…`, one per line. `Ctrl+Z`
   restores the original buffer.

## Repository layout

```
src/core/        Parser library — pure C++17, no Win32 / Notepad++ deps
src/plugin/      Plugin shell (menu, lifecycle, pretty-print command)
src/ui/          Dock panel, hover hooks, field renderer
test/unit/       Catch2 unit tests for the core library
cmake/           FetchDictionaries.cmake, FetchNppSdk.cmake (pinned downloads)
resources/dictionaries/   NOTICE for the dictionaries
third_party/quickfix-spec/  Vendored dictionary fallback (used when offline)
test/data/       Real-world FIX corpora by source (fixsim, quickfix, onixs, fxcm)
```

The `src/core/` library has no Windows dependencies and is fully unit-tested on
its own. The Notepad++ integration layers (`src/plugin/`, `src/ui/`) wrap it.

## Building

### Prerequisites

- Windows 10/11
- Visual Studio 2022 (or Build Tools) with the **C++ workload** (MSVC + Windows SDK)
- CMake ≥ 3.21
- [vcpkg](https://github.com/microsoft/vcpkg) — dependencies are declared in
  `vcpkg.json` (manifest mode) and installed automatically at configure time.

> **`VCPKG_ROOT` must be set, and how you set it matters.** The vcpkg toolchain
> in `CMakePresets.json` resolves `$env{VCPKG_ROOT}`. If that variable is empty
> at configure time it silently collapses to an invalid toolchain path
> (`/scripts/buildsystems/vcpkg.cmake`), vcpkg never runs, `find_package` fails,
> and you get a broken/empty build tree. A session-only `$env:VCPKG_ROOT = "…"`
> (below) is fine for command-line builds **in that same terminal**, but VS Code
> and Visual Studio launched from Explorer only inherit *persistent* User/System
> variables. For the IDE flows, set it persistently once and restart the IDE:
>
> ```powershell
> setx VCPKG_ROOT "C:\path\to\vcpkg"   # persistent; takes effect in new processes
> ```

### Build the core library + tests

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"   # this terminal only — see note above
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
```

Dependencies (`pugixml`, `catch2`) are resolved by vcpkg manifest mode. The
manifest deliberately **does not pin a `builtin-baseline`**, so it resolves
against whatever commit your active vcpkg checkout (`$VCPKG_ROOT`) is at. This is
what lets CI "just work" on GitHub's preinstalled vcpkg (whose commit GitHub
controls and changes over time) without a baseline the runner might not contain.
If you ever need reproducibility, re-pin by adding `"builtin-baseline": "<commit>"`
to `vcpkg.json` (that becomes the override for both local and CI).

### Keeping local vcpkg in sync with CI

Because neither side pins a baseline, your local vcpkg and the CI runner's vcpkg
can drift to different commits. For `pugixml` + `catch2` (very stable ports) this
rarely matters, but if you want your local builds to match CI exactly:

1. Find the commit CI used — every CI run prints **"Runner vcpkg commit: `<sha>`"**
   in its job summary (and build log).
2. Check your local vcpkg out to that commit:

   ```powershell
   git -C $env:VCPKG_ROOT fetch origin
   git -C $env:VCPKG_ROOT checkout <sha>
   ```

   (Re-run `bootstrap-vcpkg.bat` if vcpkg asks you to.) Delete `build/<preset>/` and
   re-configure so the new ports take effect.

To go the other way — make CI match a commit *you* choose — pin `builtin-baseline`
as described above; both sides then use exactly that commit.

### FIX dictionaries

`cmake/FetchDictionaries.cmake` provides the QuickFIX dictionaries at configure
time, writing them to `build/<preset>/dictionaries/`:

- **Default:** download from a pinned `quickfix/quickfix` commit, verified by
  SHA-512.
- **Fallback:** if the download fails (offline / air-gapped) the build copies the
  vendored copies committed under `third_party/quickfix-spec/`.
- Pass `-DFIXPARSER_FETCH_DICTIONARIES=OFF` to skip the network entirely and
  always use the vendored copies.

### Test datasets

Real-world FIX corpora live under `test/data/`, organized by source with a
`SOURCE.md` (origin, license, retrieval date) in each folder. See
`test/data/README.md`.

## Local debugging in Notepad++ (VS Code, F5)

A one-command loop sets up a real portable Notepad++ with the plugin deployed and
a sample log open, so you can build → F5 → step into plugin code.

**Prerequisites:** the VS Code C/C++ extension (`ms-vscode.cpptools`, provides the
`cppvsdbg` debugger) and a **persistent** `VCPKG_ROOT` (see the `setx` note under
[Building](#building) — a terminal-only `$env:VCPKG_ROOT` is *not* seen by VS Code,
including the CMake Tools extension and the F5 build task). When you open this repo
in VS Code it will prompt to install the recommended extensions (from
`.vscode/extensions.json`) — accept it, or install them from the Extensions view's
"Recommended" section.

> **If configure already failed once with an unset `VCPKG_ROOT`,** the
> `build/<preset>/` tree holds a poisoned cache with the bad toolchain path baked
> in; re-configuring won't repair it. Delete the build directory
> (`Remove-Item -Recurse -Force build\vs2022`) and configure again after setting
> `VCPKG_ROOT` persistently.

**Use it:** pick **Debug FixParser in Notepad++ (moderate log)** (or the 50 MB
single-line variant) from the Run and Debug panel and press F5. The
`preLaunchTask` builds the plugin, downloads portable Notepad++, generates the
sample logs, and deploys — then launches N++ on the sample so your breakpoints in
`src/plugin` / `src/ui` hit.

What the local-dev CMake targets do (all excluded from `ALL`/CI):

| Target | Action |
|--------|--------|
| `npp_fetch` | Download + extract pinned portable Notepad++ to `tools/npp/` |
| `gen_sample_log` | Generate `tools/sample/sample-multiline.fix` (~25k msgs) and `sample-singleline.fix` (~50 MB) |
| `deploy_local` | Copy `FixParser.dll` + `.pdb` + dictionaries into `tools/npp/plugins/FixParser/` |
| `local_env` | All of the above (the VS Code preLaunchTask) |

The plugin is built with the static triplet (`x64-windows-static`), so the
deployed DLL is self-contained — it loads in a stock portable Notepad++ with no
extra runtime DLLs, including Debug builds. Everything under `tools/` is generated
and git-ignored.

## Encoding

Files must be open in **UTF-8 or ANSI** encoding in Notepad++. FIX's structural
characters are all ASCII, so parsing is encoding-agnostic; non-ASCII bytes in
free-text fields are passed through and rendered using the buffer's code page.
UTF-16 files should be converted via the Encoding menu first.

## License

GPL-2.0. See [`LICENSE`](LICENSE). Third-party components and their licenses are
listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md); the bundled FIX
dictionaries are covered by the QuickFIX Software License (see
`resources/dictionaries/NOTICE.txt`).
