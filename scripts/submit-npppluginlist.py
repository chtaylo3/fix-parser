#!/usr/bin/env python3
"""Insert FixParser's entries into the upstream nppPluginList files.

This is the *local* submit helper (run with your own `gh` auth). Its core job
is the one fiddly, brittle step: surgically inserting our finalized entry into
upstream's `src/pl.x64.json` / `src/pl.x86.json` so the diff is minimal and
reviewer-clean. Upstream specifics this respects:

  * The `npp-plugins` array is sorted case-insensitively by **display-name**
    (not folder-name), so we insert at the right sorted position.
  * Indentation is tabs; entries are objects of one-line string fields.
  * Line endings are MIXED across the file (some LF, some CRLF), so we match
    the neighbor entry's line ending instead of normalizing -- a full JSON
    re-serialize would rewrite every line and get the PR rejected.

By default it runs DRY: it edits local copies you pass in and prints the diff.
The actual fork / branch / push / PR stays a manual `gh` step (see the runbook
printed by --runbook and packaging/npppluginlist/README.md), because it needs
your cross-repo auth, a synced fork, and a local Plugins-Admin smoke test that
can't be automated.

Examples
  # Preview the edits against local copies of the upstream files:
  python scripts/submit-npppluginlist.py --dry-run \
      --x64-list /path/to/fork/src/pl.x64.json \
      --x86-list /path/to/fork/src/pl.x86.json

  # Edit the files in place (after reviewing the dry-run diff):
  python scripts/submit-npppluginlist.py --write \
      --x64-list /path/to/fork/src/pl.x64.json \
      --x86-list /path/to/fork/src/pl.x86.json

  # Print the surrounding fork/PR runbook:
  python scripts/submit-npppluginlist.py --runbook

Requires only the Python 3 standard library.
"""
import argparse
import difflib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PKG = ROOT / "packaging" / "npppluginlist"
PLACEHOLDER_ID = "0" * 64

# our entry file for each upstream list file
OUR_ENTRY = {
    "x64": PKG / "entry.x64.json",
    "x86": PKG / "entry.x86.json",
}

ENTRY_OPEN = re.compile(r"^\t\t\{[ \t]*$")
ENTRY_CLOSE = re.compile(r"^\t\t\},?[ \t]*$")


def _eol(line):
    return "\r\n" if line.endswith("\r\n") else "\n"


def _scan_entries(lines):
    """Yield (start, end, display_name, folder_name, has_comma) for each array
    element. start/end are 0-based inclusive line indices."""
    out = []
    in_array = False
    i, n = 0, len(lines)
    while i < n:
        body = lines[i].rstrip("\r\n")
        if not in_array:
            if '"npp-plugins"' in body:
                in_array = True
            i += 1
            continue
        if ENTRY_OPEN.match(body):
            start = i
            j = i + 1
            while j < n and not ENTRY_CLOSE.match(lines[j].rstrip("\r\n")):
                j += 1
            block = "".join(lines[start:j + 1])
            dn = re.search(r'"display-name":\s*"((?:[^"\\]|\\.)*)"', block)
            fn = re.search(r'"folder-name":\s*"((?:[^"\\]|\\.)*)"', block)
            has_comma = lines[j].rstrip("\r\n").endswith("},")
            out.append((start, j,
                        dn.group(1) if dn else "",
                        fn.group(1) if fn else "",
                        has_comma))
            i = j + 1
            continue
        i += 1
    return out


def _build_block(entry, eol, trailing_comma):
    """Serialize our entry as a tab-indented array element, fields in the order
    they appear in our entry file."""
    keys = list(entry.keys())
    parts = ["\t\t{" + eol]
    for idx, k in enumerate(keys):
        sep = "," if idx < len(keys) - 1 else ""
        parts.append("\t\t\t" + json.dumps(k) + ": "
                     + json.dumps(entry[k], ensure_ascii=True) + sep + eol)
    parts.append("\t\t}" + ("," if trailing_comma else "") + eol)
    return "".join(parts)


def insert_or_replace(text, entry):
    """Return (new_text, action). Replaces an existing FixParser entry, else
    inserts at the case-insensitive display-name sorted position."""
    lines = text.splitlines(keepends=True)
    entries = _scan_entries(lines)
    if not entries:
        raise ValueError("no npp-plugins entries found; is this a pl.*.json?")
    dn_new = entry["display-name"].lower()
    fn_new = entry["folder-name"]

    # Replace in place if we're already listed (idempotent re-runs).
    for (s, e, _dn, fn, comma) in entries:
        if fn == fn_new:
            block = _build_block(entry, _eol(lines[s]), comma)
            return "".join(lines[:s] + [block] + lines[e + 1:]), \
                f"replaced existing '{fn}' (lines {s + 1}-{e + 1})"

    # Insert before the first entry whose display-name sorts after ours.
    for (s, e, dn, fn, _comma) in entries:
        if dn.lower() > dn_new:
            block = _build_block(entry, _eol(lines[s]), True)
            return "".join(lines[:s] + [block] + lines[s:]), \
                f"inserted before '{fn}' (at line {s + 1})"

    # Otherwise append after the last entry, giving the old last one a comma.
    s, e, _dn, fn, comma = entries[-1]
    eol = _eol(lines[e])
    if not comma:
        lines[e] = lines[e].rstrip("\r\n").rstrip() + "," + eol
    block = _build_block(entry, eol, False)
    return "".join(lines[:e + 1] + [block] + lines[e + 1:]), \
        f"appended after '{fn}'"


def _check_entry(arch, entry):
    """Return a list of blocking problems for a finalized submission."""
    problems = []
    if entry.get("id", "").lower() == PLACEHOLDER_ID:
        problems.append(f"{arch}: id is still the placeholder -- run the prepare "
                        f"step against the published release first")
    if entry.get("folder-name") != "FixParser":
        problems.append(f"{arch}: folder-name is not 'FixParser'")
    return problems


RUNBOOK = """\
nppPluginList submission runbook (local, uses your own `gh` auth)

  0. Make sure the entries are finalized for the release you're submitting:
       run the "Prepare nppPluginList submission" workflow (or
       scripts/prepare-npppluginlist.py) so id/version/repository are real,
       then `python scripts/validate-npppluginlist.py` (expect 0 warnings).

  1. Fork + clone (first time) or sync (subsequent):
       gh repo fork notepad-plus-plus/nppPluginList --clone --remote
       # later releases, from inside the clone:
       gh repo sync <you>/nppPluginList --source notepad-plus-plus/nppPluginList

  2. Branch and apply the entries (review the dry-run first):
       cd nppPluginList
       git checkout -b add-fixparser
       python <fix-parser>/scripts/submit-npppluginlist.py --dry-run \\
           --x64-list src/pl.x64.json --x86-list src/pl.x86.json
       python <fix-parser>/scripts/submit-npppluginlist.py --write \\
           --x64-list src/pl.x64.json --x86-list src/pl.x86.json

  3. Validate with upstream's own tooling, then smoke-test the plugin in a
     debug Notepad++ (both architectures) -- upstream expects this.
       python validator/validator.py   # or per their current README

  4. Commit, push to your fork, open the PR:
       git commit -am "Add FixParser plugin"
       git push -u origin add-fixparser
       gh pr create --repo notepad-plus-plus/nppPluginList --web
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--x64-list", type=Path, help="path to upstream src/pl.x64.json")
    ap.add_argument("--x86-list", type=Path, help="path to upstream src/pl.x86.json")
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true",
                      help="print the diff without writing (default)")
    mode.add_argument("--write", action="store_true",
                      help="apply the edits to the list files in place")
    ap.add_argument("--runbook", action="store_true",
                    help="print the fork/PR runbook and exit")
    ap.add_argument("--allow-placeholder", action="store_true",
                    help="permit a placeholder id (preview only; never for a real PR)")
    args = ap.parse_args()

    # Preserve real line endings in the printed diff (Windows text-mode stdout
    # would otherwise translate every \n and show doubled CRs for CRLF lines).
    try:
        sys.stdout.reconfigure(newline="")
    except (AttributeError, ValueError):
        pass

    if args.runbook:
        print(RUNBOOK)
        return 0

    targets = [("x64", args.x64_list), ("x86", args.x86_list)]
    targets = [(a, p) for a, p in targets if p]
    if not targets:
        ap.error("pass --x64-list and/or --x86-list (or --runbook)")

    entries = {a: json.loads(OUR_ENTRY[a].read_text(encoding="utf-8"))
               for a, _ in targets}

    if not args.allow_placeholder:
        problems = [p for a, _ in targets for p in _check_entry(a, entries[a])]
        if problems:
            for p in problems:
                print(f"FAIL  {p}", file=sys.stderr)
            print("\nRefusing to proceed. Finalize the entries first, or pass "
                  "--allow-placeholder to preview the insertion only.",
                  file=sys.stderr)
            return 1

    for arch, list_path in targets:
        if not list_path.is_file():
            print(f"error: list file not found: {list_path}", file=sys.stderr)
            return 1
        with open(list_path, encoding="utf-8", newline="") as fh:
            original = fh.read()
        updated, action = insert_or_replace(original, entries[arch])
        print(f"== {list_path}  ->  {action} ==")
        if args.write:
            with open(list_path, "w", encoding="utf-8", newline="") as fh:
                fh.write(updated)
            print(f"   written ({len(updated) - len(original):+d} bytes)")
        else:
            diff = difflib.unified_diff(
                original.splitlines(keepends=True),
                updated.splitlines(keepends=True),
                fromfile=f"a/{list_path.name}", tofile=f"b/{list_path.name}", n=2)
            sys.stdout.writelines(diff)
            print()
    if not args.write:
        print("Dry run only. Re-run with --write to apply, then commit in the fork.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
