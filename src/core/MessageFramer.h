#pragma once

#include <string_view>
#include <vector>
#include <cstddef>

namespace fixparser {

struct MessageBoundary {
    std::size_t start = 0; // offset of '8' in "8=..."
    std::size_t end = 0;   // one past the last byte of the message
};

// Splits a canonical (SOH-separated) byte stream into individual FIX messages.
// Primary strategy uses tag 9 (BodyLength) to find exact message extents; when
// BodyLength is missing or inconsistent it falls back to scanning for the next
// "8=" at a field boundary. Only confirmed FIX messages are returned; arbitrary
// non-FIX content between messages is left out of every boundary.
class MessageFramer {
public:
    static constexpr char SOH = '\x01';

    static std::vector<MessageBoundary> findBoundaries(std::string_view canonical);

    static std::vector<std::string_view> split(std::string_view canonical);
};

} // namespace fixparser
