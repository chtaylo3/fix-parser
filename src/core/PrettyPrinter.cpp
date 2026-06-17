#include "PrettyPrinter.h"

#include "DelimiterNormalizer.h"
#include "MessageFramer.h"

namespace fixparser {

namespace {

char outputDelimiter(OutputStyle style) {
    switch (style) {
        case OutputStyle::PipePerLine:   return '|';
        case OutputStyle::SohPerLine:    return '\x01';
        case OutputStyle::CaretAPerLine: return '\0'; // handled specially
    }
    return '|';
}

} // namespace

std::string PrettyPrinter::formatOne(std::string_view message, OutputStyle style) {
    NormalizedFix norm = DelimiterNormalizer::normalize(message);
    const std::string& canon = norm.canonical;

    std::string out;
    out.reserve(canon.size());

    const bool caretA = (style == OutputStyle::CaretAPerLine);
    const char delim = outputDelimiter(style);

    for (char c : canon) {
        if (c == '\r' || c == '\n') continue; // never carry embedded EOLs into a line
        if (c == DelimiterNormalizer::SOH) {
            if (caretA) { out += "^A"; }
            else { out.push_back(delim); }
        } else {
            out.push_back(c);
        }
    }

    // Trim a single trailing delimiter so lines don't end in a dangling separator.
    if (caretA) {
        if (out.size() >= 2 && out.compare(out.size() - 2, 2, "^A") == 0)
            out.erase(out.size() - 2);
    } else if (!out.empty() && out.back() == delim) {
        out.pop_back();
    }

    return out;
}

std::string PrettyPrinter::formatOne(const char* bytes, std::size_t len, OutputStyle style) {
    return formatOne(std::string_view(bytes, len), style);
}

std::string PrettyPrinter::prettyPrint(std::string_view raw, OutputStyle style) {
    NormalizedFix norm = DelimiterNormalizer::normalize(raw);
    const std::string& canon = norm.canonical;

    std::string out;
    auto bounds = MessageFramer::findBoundaries(canon);
    for (std::size_t i = 0; i < bounds.size(); ++i) {
        if (i > 0) out.push_back('\n');
        std::string_view msg(canon.data() + bounds[i].start, bounds[i].end - bounds[i].start);
        out += formatOne(msg, style);
    }
    return out;
}

} // namespace fixparser
