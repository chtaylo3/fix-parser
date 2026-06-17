# Dataset: QuickFIX acceptance-test definitions

- **Source:** https://github.com/quickfix/quickfix (`test/definitions/`)
- **Pinned commit:** `386ce46e917ae494ab6e90b1be90fd421cdbe3f9`
- **Description:** A curated subset of the QuickFIX engine's own acceptance-test
  definition files (`.def`). These are the reference corpus the QuickFIX engine
  validates against, covering Logon/Heartbeat/Logout session flows across
  FIX 4.0 through FIX 5.0, plus the client `Normal` end-to-end flow.
- **Retrieved:** 2026-06-17
- **License:** QuickFIX Software License, Version 1.0 (BSD-style). See
  https://github.com/quickfix/quickfix/blob/master/LICENSE

## File format

`.def` files are session scripts. Lines are prefixed with a control character:

- `I` — message the test **initiator sends** (BodyLength/CheckSum are added by
  the engine, so these lines often omit tags 9 and 10).
- `E` — message the test **expects to receive**.
- `i` / `e` — connection actions (e.g. `iCONNECT`, `eDISCONNECT`).
- `#` — comment.
- `<TIME>` — a placeholder the engine substitutes with the current timestamp.

Fields within a message are separated by literal SOH (0x01) bytes. The test
harness (`DatasetTests.cpp`) strips the `I`/`E` prefix and substitutes `<TIME>`
before parsing.
