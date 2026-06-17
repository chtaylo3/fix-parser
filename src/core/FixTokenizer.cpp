#include "FixTokenizer.h"

#include <cctype>
#include <charconv>

namespace fixparser {

TokenResult FixTokenizer::tokenize(std::string_view canonical) {
    TokenResult result;
    const std::size_t len = canonical.size();
    std::size_t pos = 0;

    auto skipToNextField = [&](std::size_t from) -> std::size_t {
        std::size_t soh = canonical.find(SOH, from);
        return (soh == std::string_view::npos) ? len : soh + 1;
    };

    while (pos < len) {
        // Skip stray SOH bytes (e.g. empty fields / leading delimiter).
        if (canonical[pos] == SOH) { ++pos; continue; }

        const std::size_t tagStart = pos;
        while (pos < len && std::isdigit(static_cast<unsigned char>(canonical[pos]))) ++pos;

        // Must have at least one digit followed by '='.
        if (pos == tagStart || pos >= len || canonical[pos] != '=') {
            result.errors.push_back({tagStart, "expected <digits>'='"});
            pos = skipToNextField(tagStart);
            continue;
        }

        int tag = 0;
        std::from_chars(canonical.data() + tagStart, canonical.data() + pos, tag);
        ++pos; // consume '='

        const std::size_t valStart = pos;
        const std::size_t soh = canonical.find(SOH, pos);
        const std::size_t valEnd = (soh == std::string_view::npos) ? len : soh;

        result.fields.push_back(TagValue{tag, canonical.substr(valStart, valEnd - valStart)});

        pos = (soh == std::string_view::npos) ? len : soh + 1;
    }

    return result;
}

} // namespace fixparser
