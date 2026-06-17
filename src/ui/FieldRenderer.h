#pragma once

#include "ParseResult.h"

#include <string>

namespace fixparser {

// Produces human-readable text from a ParseResult. Output is UTF-8 and shared by
// both the hover calltip (compact) and the dock panel.
class FieldRenderer {
public:
    // Compact multi-line summary for the calltip. Limited to `maxFields` rows.
    static std::string toCalltip(const ParseResult& r, std::size_t maxFields = 8);

    // One "tag Name = value (enumDescription)" line for a single field.
    static std::string formatField(const ParsedField& f);
};

} // namespace fixparser
