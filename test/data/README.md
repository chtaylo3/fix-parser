# Test datasets

Real-world FIX message corpora used by `test/unit/DatasetTests.cpp`, organized by
source. Each subfolder has a `SOURCE.md` with the origin URL, license, retrieval
date, and any local modifications.

| Folder | Source | Notes |
|--------|--------|-------|
| `fixsim/` | https://www.fixsim.com/sample-fix-messages | Clean FIX 4.2/4.4 simulator messages (verbatim, pipe-normalized) |
| `quickfix/` | https://github.com/quickfix/quickfix (`test/definitions/`) | Reference engine acceptance-test `.def` files (verbatim, BSD-licensed) |
| `onixs/` | https://ref.onixs.biz/ | Synthetic nested repeating-group examples modeled on the OnixS reference |
| `fxcm/` | https://github.com/fxcm/FIXAPI | Synthetic broker-style messages using FXCM's documented custom fields |

Where a source ships its messages only as proprietary docs or binary archives
(OnixS, FXCM), the messages here are synthetic but constructed to match the
documented structures, and the original source is cited. Verbatim corpora
(fixsim, quickfix) are vendored as-is with attribution.
