#include <catch2/catch_test_macros.hpp>

#include "FixTokenizer.h"

using namespace fixparser;

namespace {
std::string canon(std::string s) {
    for (char& c : s) if (c == '|') c = '\x01';
    return s;
}
}

// NB: TagValue::value is a string_view into the tokenized buffer, so the buffer
// must outlive the result — hence the named locals below.

TEST_CASE("tokenizes simple tag=value pairs", "[tokenizer]") {
    const std::string s = canon("8=FIX.4.4|9=12|35=D|");
    auto r = FixTokenizer::tokenize(s);
    REQUIRE(r.errors.empty());
    REQUIRE(r.fields.size() == 3);
    REQUIRE(r.fields[0].tag == 8);
    REQUIRE(r.fields[0].value == "FIX.4.4");
    REQUIRE(r.fields[1].tag == 9);
    REQUIRE(r.fields[1].value == "12");
    REQUIRE(r.fields[2].tag == 35);
    REQUIRE(r.fields[2].value == "D");
}

TEST_CASE("value may contain '=' characters", "[tokenizer]") {
    const std::string s = canon("58=key=value pair|");
    auto r = FixTokenizer::tokenize(s);
    REQUIRE(r.fields.size() == 1);
    REQUIRE(r.fields[0].tag == 58);
    REQUIRE(r.fields[0].value == "key=value pair");
}

TEST_CASE("handles missing trailing SOH", "[tokenizer]") {
    const std::string s = canon("8=FIX.4.4|10=000");
    auto r = FixTokenizer::tokenize(s);
    REQUIRE(r.fields.size() == 2);
    REQUIRE(r.fields[1].tag == 10);
    REQUIRE(r.fields[1].value == "000");
}

TEST_CASE("emits error for a field without '='", "[tokenizer]") {
    const std::string s = canon("8=FIX.4.4|garbage|35=D|");
    auto r = FixTokenizer::tokenize(s);
    REQUIRE(r.errors.size() == 1);
    REQUIRE(r.fields.size() == 2); // 8 and 35 still recovered
}

TEST_CASE("supports empty field values", "[tokenizer]") {
    const std::string s = canon("58=|35=D|");
    auto r = FixTokenizer::tokenize(s);
    REQUIRE(r.fields.size() == 2);
    REQUIRE(r.fields[0].tag == 58);
    REQUIRE(r.fields[0].value.empty());
}
