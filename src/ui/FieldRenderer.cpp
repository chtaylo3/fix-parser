#include "FieldRenderer.h"

#include <string>

namespace fixparser {

std::string FieldRenderer::formatField(const ParsedField& f) {
    std::string line = std::to_string(f.tag);
    if (!f.name.empty()) { line += "  "; line += f.name; }
    line += " = ";
    line += f.value;
    if (!f.enumDescription.empty()) {
        line += " (";
        line += f.enumDescription;
        line += ")";
    }
    return line;
}

std::string FieldRenderer::toCalltip(const ParseResult& r, std::size_t maxFields) {
    std::string out;

    if (!r.isFix) {
        return "Not a FIX message";
    }

    // Header line: MsgType (name).
    out += "MsgType: ";
    out += r.msgType.empty() ? "?" : r.msgType;
    if (!r.msgTypeName.empty()) { out += " ("; out += r.msgTypeName; out += ")"; }
    if (!r.beginString.empty()) { out += "  ["; out += r.beginString; out += "]"; }
    out += "\n";

    std::size_t shown = 0;
    auto appendFields = [&](const std::vector<ParsedField>& fields) {
        for (const auto& f : fields) {
            if (f.tag == 8 || f.tag == 35) continue; // already in the header line
            if (shown >= maxFields) return;
            out += FieldRenderer::formatField(f);
            out += "\n";
            ++shown;
        }
    };
    appendFields(r.header);
    appendFields(r.body);

    std::size_t total = r.header.size() + r.body.size() + r.trailer.size();
    if (shown < total - shown && total > shown) {
        out += "... ";
        out += std::to_string(total - shown);
        out += " more fields\n";
    }

    for (const auto& d : r.diagnostics) {
        if (d.severity == Severity::Error) {
            out += "[error] ";
            out += d.message;
            out += "\n";
        }
    }

    if (!out.empty() && out.back() == '\n') out.pop_back();
    return out;
}

} // namespace fixparser
