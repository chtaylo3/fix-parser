#pragma once

#include "ParseResult.h"

#include <string_view>

namespace fixparser {

class Dictionary;

// Parses a single FIX message (in any delimiter convention) into a ParseResult:
// normalize delimiters -> tokenize -> structural validation -> optional semantic
// pass against a QuickFIX dictionary.
class FixParser {
public:
    // `dict` may be null, in which case fields carry only raw tag/value data.
    static ParseResult parse(std::string_view rawMessage, const Dictionary* dict = nullptr);
};

} // namespace fixparser
