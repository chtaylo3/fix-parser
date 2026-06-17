#include "MessageFramer.h"

#include <cctype>
#include <charconv>

namespace fixparser {

namespace {

constexpr char SOH = MessageFramer::SOH;

bool isFieldBoundary(std::string_view s, std::size_t pos) {
    if (pos == 0) return true;
    char prev = s[pos - 1];
    return prev == SOH || prev == '\n' || prev == '\r';
}

// Find the next "8=" beginning a message at a field boundary, at or after `from`.
std::size_t findMessageStart(std::string_view s, std::size_t from) {
    std::size_t pos = from;
    while (true) {
        std::size_t cand = s.find("8=", pos);
        if (cand == std::string_view::npos) return std::string_view::npos;
        if (isFieldBoundary(s, cand)) return cand;
        pos = cand + 2;
    }
}

// Read the value of the field starting at `fieldStart` (expects "<tag>=value").
// Returns the value view and sets `afterField` to one past the field's SOH.
std::string_view readField(std::string_view s, std::size_t fieldStart,
                           std::size_t& afterField) {
    std::size_t eq = s.find('=', fieldStart);
    if (eq == std::string_view::npos) { afterField = s.size(); return {}; }
    std::size_t soh = s.find(SOH, eq + 1);
    std::size_t valEnd = (soh == std::string_view::npos) ? s.size() : soh;
    afterField = (soh == std::string_view::npos) ? s.size() : soh + 1;
    return s.substr(eq + 1, valEnd - eq - 1);
}

std::size_t fallbackEnd(std::string_view s, std::size_t start) {
    std::size_t next = findMessageStart(s, start + 1);
    return (next == std::string_view::npos) ? s.size() : next;
}

// Returns the end offset of the message starting at `start`, using BodyLength
// when possible and falling back to the next "8=" otherwise.
std::size_t frameOne(std::string_view s, std::size_t start) {
    // Field 1: BeginString (tag 8)
    if (s.compare(start, 2, "8=") != 0) return fallbackEnd(s, start);
    std::size_t after8 = 0;
    readField(s, start, after8);

    // Field 2: BodyLength (tag 9)
    if (after8 >= s.size() || s.compare(after8, 2, "9=") != 0)
        return fallbackEnd(s, start);
    std::size_t after9 = 0;
    std::string_view lenVal = readField(s, after8, after9);

    int bodyLength = 0;
    auto [ptr, ec] = std::from_chars(lenVal.data(), lenVal.data() + lenVal.size(), bodyLength);
    if (ec != std::errc() || bodyLength < 0) return fallbackEnd(s, start);

    // Body spans `bodyLength` bytes starting at the byte after the BodyLength SOH.
    std::size_t bodyStart = after9;
    std::size_t checksumStart = bodyStart + static_cast<std::size_t>(bodyLength);
    if (checksumStart > s.size()) return fallbackEnd(s, start);

    // Checksum field must be present at the computed position.
    if (s.compare(checksumStart, 3, "10=") != 0) return fallbackEnd(s, start);

    // End of the checksum field: terminated by SOH (consumed, real wire) or by a
    // newline / EOF (consumed exclusively, e.g. one-message-per-line input).
    std::size_t end = checksumStart;
    while (end < s.size() && s[end] != SOH && s[end] != '\n' && s[end] != '\r') ++end;
    if (end < s.size() && s[end] == SOH) ++end; // include trailing SOH only
    return end;
}

} // namespace

std::vector<MessageBoundary> MessageFramer::findBoundaries(std::string_view canonical) {
    std::vector<MessageBoundary> result;
    std::size_t pos = 0;
    while (pos < canonical.size()) {
        std::size_t start = findMessageStart(canonical, pos);
        if (start == std::string_view::npos) break;
        std::size_t end = frameOne(canonical, start);
        if (end <= start) { end = (start + 1 <= canonical.size()) ? start + 1 : canonical.size(); }
        result.push_back({start, end});
        pos = end;
    }
    return result;
}

std::vector<std::string_view> MessageFramer::split(std::string_view canonical) {
    std::vector<std::string_view> out;
    for (const auto& b : findBoundaries(canonical))
        out.push_back(canonical.substr(b.start, b.end - b.start));
    return out;
}

} // namespace fixparser
