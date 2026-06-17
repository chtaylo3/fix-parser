#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

namespace fixparser {

enum class DelimiterStyle { Auto, Soh, Pipe, CaretA };
enum class DetectionConfidence { Confident, Ambiguous, None };

struct NormalizedFix {
    std::string canonical;                  // SOH (0x01) separated bytes
    DelimiterStyle detected = DelimiterStyle::Auto;
    DetectionConfidence confidence = DetectionConfidence::None;
    std::vector<std::size_t> fieldStarts;   // offsets into canonical for diagnostics
};

// Converts FIX text in any common log delimiter convention (literal SOH, pipe,
// caret-A, or \u0001 escape) into a canonical SOH-separated byte stream.
//
// Detection uses a three-pass structural probe (see plan): the \u0001 escape is
// pre-replaced with 0x01, then SOH > caret-A > pipe are tried in priority order,
// committing to the first style that yields at least one "<digits>=" field.
class DelimiterNormalizer {
public:
    static constexpr char SOH = '\x01';

    static NormalizedFix normalize(std::string_view raw);

    // Detect the dominant style without building the canonical stream.
    static DelimiterStyle detect(std::string_view raw, DetectionConfidence& confidence);
};

} // namespace fixparser
