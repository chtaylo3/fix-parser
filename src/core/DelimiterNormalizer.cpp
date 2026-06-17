#include "DelimiterNormalizer.h"

#include <array>
#include <cctype>

namespace fixparser {

namespace {

// True if any token produced by splitting `s` on `delim` begins with one or
// more ASCII digits followed by '=' (i.e. looks like a FIX "tag=value" field).
bool yieldsTagValueField(std::string_view s, char delim) {
    std::size_t pos = 0;
    while (pos <= s.size()) {
        std::size_t next = s.find(delim, pos);
        std::string_view tok = (next == std::string_view::npos)
            ? s.substr(pos)
            : s.substr(pos, next - pos);

        std::size_t i = 0;
        while (i < tok.size() && std::isdigit(static_cast<unsigned char>(tok[i]))) ++i;
        if (i > 0 && i < tok.size() && tok[i] == '=')
            return true;

        if (next == std::string_view::npos) break;
        pos = next + 1;
    }
    return false;
}

// Two-character caret-A ("^A") is treated as a single logical delimiter. We
// scan for the literal pair rather than relying on std::find of one char.
bool yieldsTagValueFieldCaretA(std::string_view s) {
    std::size_t pos = 0;
    while (pos <= s.size()) {
        std::size_t next = s.find("^A", pos);
        std::string_view tok = (next == std::string_view::npos)
            ? s.substr(pos)
            : s.substr(pos, next - pos);

        std::size_t i = 0;
        while (i < tok.size() && std::isdigit(static_cast<unsigned char>(tok[i]))) ++i;
        if (i > 0 && i < tok.size() && tok[i] == '=')
            return true;

        if (next == std::string_view::npos) break;
        pos = next + 2; // skip both chars of "^A"
    }
    return false;
}

// Replace every literal six-character "\u0001" escape with a single SOH byte.
std::string preReplaceUnicodeEscape(std::string_view raw) {
    std::string out;
    out.reserve(raw.size());
    std::size_t pos = 0;
    while (pos < raw.size()) {
        if (raw.compare(pos, 6, "\\u0001") == 0) {
            out.push_back(DelimiterNormalizer::SOH);
            pos += 6;
        } else {
            out.push_back(raw[pos]);
            ++pos;
        }
    }
    return out;
}

} // namespace

DelimiterStyle DelimiterNormalizer::detect(std::string_view raw,
                                           DetectionConfidence& confidence) {
    // Pre-replace the \u0001 escape so it participates as SOH in pass 1.
    const std::string pre = preReplaceUnicodeEscape(raw);
    std::string_view s(pre);

    // A style is only plausible if its delimiter actually appears in the input
    // (otherwise the whole string is one token that may still start "<digits>=").
    const bool sohPlausible =
        (s.find(SOH) != std::string_view::npos) && yieldsTagValueField(s, SOH);
    const bool caretPlausible =
        (s.find("^A") != std::string_view::npos) && yieldsTagValueFieldCaretA(s);
    const bool pipePlausible =
        (s.find('|') != std::string_view::npos) && yieldsTagValueField(s, '|');

    const int candidates =
        (sohPlausible ? 1 : 0) + (caretPlausible ? 1 : 0) + (pipePlausible ? 1 : 0);

    if (candidates == 0) {
        confidence = DetectionConfidence::None;
        return DelimiterStyle::Auto;
    }
    confidence = (candidates == 1) ? DetectionConfidence::Confident
                                   : DetectionConfidence::Ambiguous;

    // Priority order: SOH > caret-A > pipe.
    if (sohPlausible) return DelimiterStyle::Soh;
    if (caretPlausible) return DelimiterStyle::CaretA;
    return DelimiterStyle::Pipe;
}

NormalizedFix DelimiterNormalizer::normalize(std::string_view raw) {
    NormalizedFix result;

    const std::string pre = preReplaceUnicodeEscape(raw);
    DetectionConfidence conf = DetectionConfidence::None;
    const DelimiterStyle style = detect(raw, conf);
    result.detected = style;
    result.confidence = conf;

    // Build the canonical SOH stream by mapping the committed delimiter to SOH.
    std::string& out = result.canonical;
    out.reserve(pre.size());

    switch (style) {
        case DelimiterStyle::Soh:
            out = pre; // \u0001 already mapped; literal SOH kept as-is
            break;
        case DelimiterStyle::Pipe:
            for (char c : pre) out.push_back(c == '|' ? SOH : c);
            break;
        case DelimiterStyle::CaretA: {
            std::size_t pos = 0;
            while (pos < pre.size()) {
                if (pre.compare(pos, 2, "^A") == 0) {
                    out.push_back(SOH);
                    pos += 2;
                } else {
                    out.push_back(pre[pos]);
                    ++pos;
                }
            }
            break;
        }
        case DelimiterStyle::Auto:
            out = pre; // no delimiter detected; forward bytes unchanged
            break;
    }

    // Record field start offsets for diagnostics.
    result.fieldStarts.push_back(0);
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (out[i] == SOH && i + 1 < out.size())
            result.fieldStarts.push_back(i + 1);
    }

    return result;
}

} // namespace fixparser
