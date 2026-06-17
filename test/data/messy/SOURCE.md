# Dataset: messy mixed log (hand-authored)

- **Origin:** Hand-authored synthetic fixture (2026-06-17).
- **Purpose:** Exercise `MessageFramer` and the non-FIX handling decisions on a
  realistic mixed log: well-formed FIX messages interleaved with application log
  lines, blank lines, a `#` comment, and a **log-wrapped** FIX message (a FIX
  payload embedded after a non-FIX prefix).
- **Delimiter:** pipe (`|`) throughout — a single convention, since delimiter
  detection is per-buffer.

## What it validates

- **A1 (preserve non-FIX):** timestamp/log/blank/comment lines are *not* framed
  as FIX messages (they fall outside every `MessageBoundary`).
- **B1 (log-wrapped lines):** the line beginning `2024-... raw=8=FIX.4.4|...`
  is treated as non-FIX because its `8=` is not at a field boundary, so it is
  *not* framed.
- Exactly three well-formed messages (Logon `A`, Logout `5`, NewOrderSingle `D`)
  are framed and parse cleanly.

The three well-formed FIX message bodies are taken from the FIXSIM sample set
(`test/data/fixsim/`); see that folder's `SOURCE.md` for attribution.
