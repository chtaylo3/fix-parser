# Dataset: FXCM broker-style messages

- **Reference:** FXCM FIX API — https://github.com/fxcm/FIXAPI
  (see `FXCM Custom Fields.txt` and `Sample_Codes.md` in that repo).
- **Description:** Broker-style messages using FXCM's documented custom fields:
  - `35=h` TradingSessionStatus with FXCM custom tags `FXCMSymPrecision (9001)`,
    `FXCMSymPointSize (9002)`, `FXCMSymInterestBuy (9003)`,
    `FXCMSymInterestSell (9004)`.
  - `35=W` MarketDataSnapshotFullRefresh (bid/ask) with `NoMDEntries (268)`.
  - `35=AP` PositionReport with `NoPositions (702)` and `NoPosAmt (753)` groups.
- **Created:** 2026-06-17

## Note on provenance

The FXCM repository ships its real messages inside binary archives (`.7z`,
`.docx`) and code samples rather than as plain-text message logs. These messages
are **synthetic**, constructed from the field semantics documented in the FXCM
FIX API repo (notably the custom 90xx tags), for parser test purposes.
BodyLength (9) and CheckSum (10) are placeholders.
