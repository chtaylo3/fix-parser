#include <catch2/catch_test_macros.hpp>

#include "FixParser.h"
#include "Dictionary.h"

using namespace fixparser;

namespace {
std::string soh(std::string s) {
    for (char& c : s) if (c == '|') c = '\x01';
    return s;
}

bool hasDiagnosticContaining(const ParseResult& r, const std::string& needle) {
    for (const auto& d : r.diagnostics)
        if (d.message.find(needle) != std::string::npos) return true;
    return false;
}

// Checksum 163 is the hand-computed mod-256 sum for this exact heartbeat.
const std::string VALID_HB = "8=FIX.4.4|9=5|35=0|10=163|";

const char* kMiniXml = R"(<fix major="4" minor="4" type="FIX">
  <messages>
    <message name="NewOrderSingle" msgtype="D" msgcat="app"/>
  </messages>
  <fields>
    <field number="35" name="MsgType" type="STRING">
      <value enum="D" description="ORDER_SINGLE"/>
    </field>
    <field number="54" name="Side" type="CHAR">
      <value enum="1" description="BUY"/>
    </field>
  </fields>
</fix>)";
}

TEST_CASE("parses a structurally valid message", "[parser]") {
    auto r = FixParser::parse(soh(VALID_HB));
    REQUIRE(r.isFix);
    REQUIRE(r.structurallyValid);
    REQUIRE(r.beginString == "FIX.4.4");
    REQUIRE(r.msgType == "0");
    REQUIRE_FALSE(hasDiagnosticContaining(r, "checksum mismatch"));
}

TEST_CASE("flags missing BeginString", "[parser]") {
    auto r = FixParser::parse(soh("9=5|35=0|10=000|"));
    REQUIRE_FALSE(r.isFix);
    REQUIRE(r.hasErrors());
}

TEST_CASE("flags missing MsgType", "[parser]") {
    auto r = FixParser::parse(soh("8=FIX.4.4|9=5|10=000|"));
    REQUIRE(r.hasErrors());
    REQUIRE_FALSE(r.structurallyValid);
}

TEST_CASE("warns on checksum mismatch", "[parser]") {
    auto r = FixParser::parse(soh("8=FIX.4.4|9=5|35=0|10=000|"));
    REQUIRE(hasDiagnosticContaining(r, "checksum mismatch"));
}

TEST_CASE("returns None warning for non-FIX input", "[parser]") {
    auto r = FixParser::parse("just some log text without delimiters");
    REQUIRE_FALSE(r.isFix);
    REQUIRE(hasDiagnosticContaining(r, "no FIX field delimiter"));
}

TEST_CASE("resolves names and enums with a dictionary", "[parser]") {
    auto dict = Dictionary::loadFromString(kMiniXml);
    REQUIRE(dict.has_value());
    auto r = FixParser::parse(soh("8=FIX.4.4|9=12|35=D|54=1|10=000|"), &dict.value());
    REQUIRE(r.msgTypeName == "NewOrderSingle");
    REQUIRE(r.dictionary == "FIX.4.4");

    bool foundSide = false;
    for (const auto& f : r.body) {
        if (f.tag == 54) {
            foundSide = true;
            REQUIRE(f.name == "Side");
            REQUIRE(f.enumDescription == "BUY");
        }
    }
    REQUIRE(foundSide);
}
