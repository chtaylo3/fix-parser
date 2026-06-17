# Dataset: nested repeating-group examples (OnixS-referenced)

- **Reference:** OnixS FIX Dictionary reference — https://ref.onixs.biz/
- **Description:** Messages exercising repeating groups and nested groups, which
  are the trickiest part of FIX parsing:
  - `35=W` MarketDataSnapshotFullRefresh with `NoMDEntries (268)`.
  - `35=X` MarketDataIncrementalRefresh with `NoMDEntries (268)`.
  - `35=8` ExecutionReport with `NoPartyIDs (453)` (two party entries).
- **Created:** 2026-06-17

## Note on provenance

The OnixS reference pages are proprietary commercial documentation, so their
content is **not** copied here. These messages are **synthetic** examples
constructed to match the standard FIX 4.4 group structures documented by OnixS
(and the FIX specification). They are intended for parser/group-handling tests.
BodyLength (9) and CheckSum (10) are placeholders (`0` / `000`); the parser
treats checksum mismatches as warnings, not errors.
