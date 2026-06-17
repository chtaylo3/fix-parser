#include <catch2/catch_test_macros.hpp>

#include "PrettyPrinter.h"

using namespace fixparser;

namespace {
std::string soh(std::string s) {
    for (char& c : s) if (c == '|') c = '\x01';
    return s;
}
}

TEST_CASE("formatOne produces pipe-delimited single line", "[pretty]") {
    auto out = PrettyPrinter::formatOne(soh("8=FIX.4.4|9=5|35=0|10=000|"));
    REQUIRE(out == "8=FIX.4.4|9=5|35=0|10=000");
}

TEST_CASE("formatOne normalizes caret-A input", "[pretty]") {
    auto out = PrettyPrinter::formatOne("8=FIX.4.4^A9=5^A35=0^A10=000^A");
    REQUIRE(out == "8=FIX.4.4|9=5|35=0|10=000");
}

TEST_CASE("formatOne strips embedded newlines", "[pretty]") {
    auto out = PrettyPrinter::formatOne(soh("8=FIX.4.4|9=5|\n35=0|10=000|"));
    REQUIRE(out == "8=FIX.4.4|9=5|35=0|10=000");
}

TEST_CASE("prettyPrint joins messages one per line", "[pretty]") {
    std::string two = soh("8=FIX.4.4|9=5|35=0|10=000|") + soh("8=FIX.4.4|9=5|35=0|10=000|");
    auto out = PrettyPrinter::prettyPrint(two);
    REQUIRE(out == "8=FIX.4.4|9=5|35=0|10=000\n8=FIX.4.4|9=5|35=0|10=000");
}

TEST_CASE("prettyPrint emits SOH style when requested", "[pretty]") {
    auto out = PrettyPrinter::prettyPrint(soh("8=FIX.4.4|9=5|35=0|10=000|"),
                                          OutputStyle::SohPerLine);
    REQUIRE(out == soh("8=FIX.4.4|9=5|35=0|10=000"));
}
