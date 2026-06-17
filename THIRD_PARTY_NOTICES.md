# Third-Party Notices

This product incorporates components from the projects listed below. The
original copyright notices and the licenses under which they were received are
set forth for informational purposes.

---

## pugixml

- **Used for:** Parsing QuickFIX data dictionary XML.
- **License:** MIT License
- **Homepage:** https://pugixml.org/
- **Source:** https://github.com/zeux/pugixml
- **Linkage:** Linked into the plugin (acquired via vcpkg).

```
Copyright (c) 2006-2025 Arseny Kapoulkine

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
```

---

## Catch2

- **Used for:** Unit testing only. **Not** distributed in the plugin binary.
- **License:** Boost Software License 1.0 (BSL-1.0)
- **Source:** https://github.com/catchorg/Catch2

---

## QuickFIX Data Dictionaries

- **Used for:** Runtime semantic resolution of FIX tags, types, and enums.
- **License:** QuickFIX Software License, Version 1.0 (BSD-style)
- **Source:** https://github.com/quickfix/quickfix (`spec/`, pinned commit
  `386ce46e917ae494ab6e90b1be90fd421cdbe3f9`)
- **Linkage:** Downloaded at build time (SHA-512 verified) by
  `cmake/FetchDictionaries.cmake`. A vendored fallback copy is committed under
  `third_party/quickfix-spec/` for offline builds. See
  `resources/dictionaries/NOTICE.txt`.

---

## QuickFIX acceptance-test definitions (test data)

- **Used for:** Parser test corpus only. Not shipped in the plugin.
- **License:** QuickFIX Software License, Version 1.0 (BSD-style)
- **Source:** https://github.com/quickfix/quickfix (`test/definitions/`)
- **Linkage:** Vendored verbatim under `test/data/quickfix/`. See its `SOURCE.md`.

---

## FIXSIM sample messages (test data)

- **Used for:** Parser test corpus only. Not shipped in the plugin.
- **Source:** https://www.fixsim.com/sample-fix-messages
- **Linkage:** Sample messages vendored under `test/data/fixsim/` with
  attribution. See its `SOURCE.md`.

---

## OnixS / FXCM referenced test data

- **Used for:** Parser test corpus only. Not shipped in the plugin.
- **References:** https://ref.onixs.biz/ and https://github.com/fxcm/FIXAPI
- **Linkage:** The messages under `test/data/onixs/` and `test/data/fxcm/` are
  **synthetic** examples modeled on these references (no proprietary content is
  redistributed). See each folder's `SOURCE.md`.

---

## Notepad++ Plugin Template

- **Used for:** Plugin shell / bootstrap (`PluginInterface`, docking).
- **License:** GPL-2.0
- **Source:** https://github.com/npp-plugins/plugintemplate
