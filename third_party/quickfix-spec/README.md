# Vendored QuickFIX dictionaries (offline fallback)

These XML data dictionaries are the **offline fallback** for the build. By
default the build downloads them at configure time (pinned + SHA-512 verified);
these committed copies are used only when the download is unavailable or when
`-DFIXPARSER_FETCH_DICTIONARIES=OFF` is set. See `cmake/FetchDictionaries.cmake`.

- **Source:** https://github.com/quickfix/quickfix (`spec/`)
- **Pinned commit:** `386ce46e917ae494ab6e90b1be90fd421cdbe3f9`
- **License:** QuickFIX Software License, Version 1.0 (BSD-style) —
  https://github.com/quickfix/quickfix/blob/master/LICENSE
- **Files:** FIX40–FIX44, FIX50, FIX50SP1, FIX50SP2, FIXT11.
