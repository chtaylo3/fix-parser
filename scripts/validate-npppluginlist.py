#!/usr/bin/env python3
"""Offline lint for our nppPluginList submission entries.

Validates packaging/npppluginlist/entry.{x64,x86}.json against the vendored
pl.schema (definitions/plugin) with a tiny stdlib-only JSON-Schema subset
checker, then applies project-specific consistency checks.

This mirrors the upstream nppPluginList validation closely enough to catch the
mistakes that would otherwise only surface in the upstream PR (bad version
format, missing field, malformed id/repository). The authoritative validator
still runs on that PR -- this is a fast pre-flight, not a replacement.

  - HARD failures (exit 1): schema violations, wrong folder-name, repository URL
    that is not the correct release-asset URL for the entry's own version/arch.
  - WARNINGs (exit 0): the entry version trails the release manifest, or the id
    is still the placeholder. Both are expected between a release and the moment
    you refresh the entry for submission -- they should not block CI.

Run locally:  python scripts/validate-npppluginlist.py
Requires only the Python 3 standard library.
"""
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
PKG = ROOT / "packaging" / "npppluginlist"
SCHEMA = PKG / "pl.schema"
MANIFEST = ROOT / ".release-please-manifest.json"

EXPECTED_FOLDER = "FixParser"
REPO_SLUG = "chtaylo3/fix-parser"
PLACEHOLDER_ID = "0" * 64

# (label, Notepad++ arch zip token, entry file)
ARCHES = [
    ("x64", "x64", PKG / "entry.x64.json"),
    ("x86", "Win32", PKG / "entry.x86.json"),
]


def load_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


# --- tiny JSON-Schema subset validator (only the keywords pl.schema uses) -----
def validate(node, schema, root, path, errs):
    if "$ref" in schema:
        target = root
        for part in schema["$ref"].lstrip("#/").split("/"):
            target = target[part]
        validate(node, target, root, path, errs)
        return

    t = schema.get("type")
    if t == "object":
        if not isinstance(node, dict):
            errs.append(f"{path}: expected object")
            return
        for req in schema.get("required", []):
            if req not in node:
                errs.append(f"{path}: missing required field '{req}'")
        for key, sub in schema.get("properties", {}).items():
            if key in node:
                validate(node[key], sub, root, f"{path}.{key}", errs)
        return
    if t == "array":
        if not isinstance(node, list):
            errs.append(f"{path}: expected array")
            return
        item = schema.get("items")
        if item:
            for i, elem in enumerate(node):
                validate(elem, item, root, f"{path}[{i}]", errs)
        return
    if t == "string" and not isinstance(node, str):
        errs.append(f"{path}: expected string")
        return

    # value-level keywords (apply regardless of declared type)
    if "enum" in schema and node not in schema["enum"]:
        errs.append(f"{path}: '{node}' is not one of {schema['enum']}")
    if isinstance(node, str):
        if "minLength" in schema and len(node) < schema["minLength"]:
            errs.append(f"{path}: shorter than minLength {schema['minLength']}")
        if "maxLength" in schema and len(node) > schema["maxLength"]:
            errs.append(f"{path}: longer than maxLength {schema['maxLength']}")
        if "pattern" in schema and not re.search(schema["pattern"], node):
            errs.append(f"{path}: '{node}' does not match /{schema['pattern']}/")
        if schema.get("format") == "uri":
            parsed = urlparse(node)
            if not (parsed.scheme and parsed.netloc):
                errs.append(f"{path}: '{node}' is not a valid URI")
    if "oneOf" in schema:
        matched = 0
        for sub in schema["oneOf"]:
            sub_errs = []
            validate(node, sub, root, path, sub_errs)
            if not sub_errs:
                matched += 1
        if matched != 1:
            errs.append(f"{path}: matched {matched} of oneOf branches (expected exactly 1)")


def main():
    schema_doc = load_json(SCHEMA)
    plugin_schema = schema_doc["definitions"]["plugin"]
    manifest_version = load_json(MANIFEST)["."]

    hard, warn = [], []
    for label, token, entry_path in ARCHES:
        if not entry_path.exists():
            hard.append(f"{entry_path.name}: file not found")
            continue
        entry = load_json(entry_path)

        # 1) schema conformance (the same rules the upstream validator enforces)
        validate(entry, plugin_schema, schema_doc, entry_path.name, hard)

        # 2) project-specific consistency
        if entry.get("folder-name") != EXPECTED_FOLDER:
            hard.append(
                f"{entry_path.name}: folder-name '{entry.get('folder-name')}' != "
                f"'{EXPECTED_FOLDER}' (must equal the plugin DLL base name)"
            )

        ver = entry.get("version", "")
        expected_repo = (
            f"https://github.com/{REPO_SLUG}/releases/download/"
            f"v{ver}/FixParser_{ver}_{token}.zip"
        )
        if entry.get("repository") != expected_repo:
            hard.append(
                f"{entry_path.name}: repository must be the {label} release-asset URL "
                f"for this entry's version:\n"
                f"    expected: {expected_repo}\n"
                f"    actual:   {entry.get('repository')}"
            )

        if ver != manifest_version:
            warn.append(
                f"{entry_path.name}: version '{ver}' trails release manifest "
                f"'{manifest_version}' -- refresh version + repository + id before submitting"
            )
        if str(entry.get("id", "")).lower() == PLACEHOLDER_ID:
            warn.append(
                f"{entry_path.name}: id is the placeholder -- replace with the SHA-256 "
                f"of the published {token} zip before opening the nppPluginList PR"
            )

    for w in warn:
        print(f"WARN  {w}")
    for e in hard:
        print(f"FAIL  {e}")

    if hard:
        print(f"\n{len(hard)} error(s); nppPluginList entries are NOT submission-shaped.")
        sys.exit(1)
    print(f"\nnppPluginList entries OK ({len(warn)} warning(s)).")


if __name__ == "__main__":
    main()
