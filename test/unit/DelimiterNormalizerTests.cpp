#include <catch2/catch_test_macros.hpp>

#include "DelimiterNormalizer.h"

using namespace fixparser;

namespace {
std::string soh(std::string s) {
    for (char& c : s) if (c == '|') c = '\x01';
    return s;
}
}

TEST_CASE("detects literal SOH delimiters", "[normalizer]") {
    auto n = DelimiterNormalizer::normalize(soh("8=FIX.4.4|9=5|35=0|10=000|"));
    REQUIRE(n.detected == DelimiterStyle::Soh);
    REQUIRE(n.confidence == DetectionConfidence::Confident);
    REQUIRE(n.canonical.find('\x01') != std::string::npos);
}

TEST_CASE("detects and normalizes pipe delimiters", "[normalizer]") {
    auto n = DelimiterNormalizer::normalize("8=FIX.4.4|9=5|35=0|10=000|");
    REQUIRE(n.detected == DelimiterStyle::Pipe);
    REQUIRE(n.confidence == DetectionConfidence::Confident);
    // canonical must use SOH, not pipe
    REQUIRE(n.canonical.find('|') == std::string::npos);
    REQUIRE(n.canonical.rfind("8=FIX.4.4", 0) == 0);
    REQUIRE(n.canonical[9] == '\x01');
}

TEST_CASE("detects and normalizes caret-A delimiters", "[normalizer]") {
    auto n = DelimiterNormalizer::normalize("8=FIX.4.4^A9=5^A35=0^A10=000^A");
    REQUIRE(n.detected == DelimiterStyle::CaretA);
    REQUIRE(n.canonical.find("^A") == std::string::npos);
    REQUIRE(n.canonical.find('\x01') != std::string::npos);
}

TEST_CASE("handles the \\u0001 escape form", "[normalizer]") {
    auto n = DelimiterNormalizer::normalize("8=FIX.4.4\\u00019=5\\u000135=0\\u000110=000\\u0001");
    REQUIRE(n.detected == DelimiterStyle::Soh); // escape pre-mapped to 0x01
    REQUIRE(n.canonical.find("\\u0001") == std::string::npos);
    REQUIRE(n.canonical.find('\x01') != std::string::npos);
}

TEST_CASE("reports None when no delimiter is present", "[normalizer]") {
    auto n = DelimiterNormalizer::normalize("this is not a fix message at all");
    REQUIRE(n.confidence == DetectionConfidence::None);
}

TEST_CASE("flags ambiguous delimiters", "[normalizer]") {
    // Contains both SOH (after \\u0001 mapping) and pipe-delimited fields.
    auto n = DelimiterNormalizer::normalize(soh("8=FIX.4.4|9=5|35=0") + "|55=ABC|10=000");
    // First segment uses SOH; the appended part uses pipe — both plausible.
    REQUIRE(n.confidence == DetectionConfidence::Ambiguous);
    REQUIRE(n.detected == DelimiterStyle::Soh); // SOH wins on priority
}
