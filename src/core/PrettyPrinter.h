#pragma once

#include <string>
#include <string_view>

namespace fixparser {

enum class OutputStyle {
    PipePerLine, // tag=val|tag=val   (default; trader-readable)
    SohPerLine,  // raw SOH separators preserved
    CaretAPerLine
};

// Formats FIX messages for human reading: normalizes any input delimiter to the
// chosen output delimiter and places one message per line.
class PrettyPrinter {
public:
    // Format a single message (any delimiter convention) into one line with no
    // trailing newline. Embedded CR/LF inside the message are stripped.
    static std::string formatOne(std::string_view message,
                                 OutputStyle style = OutputStyle::PipePerLine);

    static std::string formatOne(const char* bytes, std::size_t len,
                                 OutputStyle style = OutputStyle::PipePerLine);

    // Frame `raw` into messages and join their formatted forms with '\n'.
    static std::string prettyPrint(std::string_view raw,
                                   OutputStyle style = OutputStyle::PipePerLine);
};

} // namespace fixparser
