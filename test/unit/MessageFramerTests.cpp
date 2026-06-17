#include <catch2/catch_test_macros.hpp>

#include "MessageFramer.h"

using namespace fixparser;

namespace {
// Build a canonical (SOH) message with a correct BodyLength for "35=0".
// Body = everything from tag 35 up to and including the SOH before tag 10.
std::string canon(std::string s) {
    for (char& c : s) if (c == '|') c = '\x01';
    return s;
}
// A minimal heartbeat: 8,9,35,10. BodyLength counts "35=0<SOH>" = 5 bytes.
const std::string HB = canon("8=FIX.4.4|9=5|35=0|10=000|");
}

TEST_CASE("frames a single message via BodyLength", "[framer]") {
    auto b = MessageFramer::findBoundaries(HB);
    REQUIRE(b.size() == 1);
    REQUIRE(b[0].start == 0);
    REQUIRE(b[0].end == HB.size());
}

TEST_CASE("frames two concatenated messages", "[framer]") {
    std::string two = HB + HB;
    auto b = MessageFramer::findBoundaries(two);
    REQUIRE(b.size() == 2);
    REQUIRE(b[0].end == HB.size());
    REQUIRE(b[1].start == HB.size());
    REQUIRE(b[1].end == two.size());
}

TEST_CASE("splits return message views", "[framer]") {
    const std::string two = HB + HB; // keep buffer alive: split returns views
    auto parts = MessageFramer::split(two);
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == HB);
}

TEST_CASE("preserves non-FIX content between messages (not framed)", "[framer]") {
    std::string mixed = "2024-01-01 12:00:00 INFO start\n" + HB +
                        "\n2024-01-01 12:00:01 INFO done\n" + HB;
    auto b = MessageFramer::findBoundaries(mixed);
    REQUIRE(b.size() == 2);
    // The framed ranges are exactly the two FIX messages; log lines are excluded.
    REQUIRE(mixed.substr(b[0].start, b[0].end - b[0].start) == HB);
    REQUIRE(mixed.substr(b[1].start, b[1].end - b[1].start) == HB);
}

TEST_CASE("falls back to next 8= when BodyLength is invalid", "[framer]") {
    std::string bad = canon("8=FIX.4.4|9=BAD|35=0|10=000|") + HB;
    auto b = MessageFramer::findBoundaries(bad);
    REQUIRE(b.size() == 2);
}
