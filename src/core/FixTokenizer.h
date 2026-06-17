#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

namespace fixparser {

struct TagValue {
    int tag = 0;
    std::string_view value; // view into the canonical buffer; zero-copy
};

struct TokenError {
    std::size_t pos = 0;
    std::string reason;
};

struct TokenResult {
    std::vector<TagValue> fields;
    std::vector<TokenError> errors;
};

// Hand-rolled scanner for canonical (SOH-separated) FIX. FIX at the token level
// is a flat sequence of `<digits> '=' <chars> SOH`, so a single while-loop with
// no allocations per field is sufficient.
class FixTokenizer {
public:
    static constexpr char SOH = '\x01';

    // `canonical` must already be SOH-delimited (see DelimiterNormalizer).
    static TokenResult tokenize(std::string_view canonical);
};

} // namespace fixparser
