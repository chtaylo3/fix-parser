#!/usr/bin/env python3
"""Fill the nppPluginList submission entries for a released version.

Given the two published *root-layout* release zips, this computes their
SHA-256 and writes version + repository URL + id into
packaging/npppluginlist/entry.{x64,x86}.json, then leaves them for
scripts/validate-npppluginlist.py to confirm.

It does NOT push anything upstream -- it produces the finalized, paste-ready
entries. The actual PR to notepad-plus-plus/nppPluginList stays a manual,
human-reviewed step (the project also expects a local Plugins Admin smoke
test first). See packaging/npppluginlist/README.md.

Usage:
  python scripts/prepare-npppluginlist.py --version 0.1.3 \
      --x64-zip dist/FixParser_0.1.3_x64.zip \
      --x86-zip dist/FixParser_0.1.3_Win32.zip

Requires only the Python 3 standard library.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PKG = ROOT / "packaging" / "npppluginlist"
REPO_SLUG = "chtaylo3/fix-parser"

# (Notepad++ arch zip token, entry file). The x86 build ships as the "Win32"
# token; entry.x86.json is its nppPluginList object (which lands in pl.x86.json).
TARGETS = [
    ("x64", PKG / "entry.x64.json"),
    ("Win32", PKG / "entry.x86.json"),
]


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--version", required=True,
                    help="release version, e.g. 0.1.3 (a leading 'v' is stripped)")
    ap.add_argument("--x64-zip", required=True, type=Path,
                    help="path to FixParser_<ver>_x64.zip (root-layout)")
    ap.add_argument("--x86-zip", required=True, type=Path,
                    help="path to FixParser_<ver>_Win32.zip (root-layout)")
    args = ap.parse_args()

    version = args.version.lstrip("v")
    zips = {"x64": args.x64_zip, "Win32": args.x86_zip}

    for token, entry_path in TARGETS:
        zip_path = zips[token]
        if not zip_path.is_file():
            print(f"error: missing {token} zip: {zip_path}", file=sys.stderr)
            return 1
        digest = sha256(zip_path)
        repository = (
            f"https://github.com/{REPO_SLUG}/releases/download/"
            f"v{version}/FixParser_{version}_{token}.zip"
        )
        entry = json.loads(entry_path.read_text(encoding="utf-8"))
        # Mutate in place so the field order (and any extra fields) is preserved.
        entry["version"] = version
        entry["id"] = digest
        entry["repository"] = repository
        entry_path.write_text(json.dumps(entry, indent=2) + "\n", encoding="utf-8")
        print(f"{entry_path.name}: version={version} id={digest}")
        print(f"    repository={repository}")

    print("\nEntries filled. Run scripts/validate-npppluginlist.py to confirm "
          "(expect no warnings).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
