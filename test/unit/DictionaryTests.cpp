#include <catch2/catch_test_macros.hpp>

#include "Dictionary.h"
#include "DictionarySelector.h"

using namespace fixparser;

namespace {
const char* kMiniXml = R"(<fix major="4" minor="4" type="FIX">
  <messages>
    <message name="NewOrderSingle" msgtype="D" msgcat="app"/>
    <message name="Heartbeat" msgtype="0" msgcat="admin"/>
  </messages>
  <fields>
    <field number="35" name="MsgType" type="STRING">
      <value enum="D" description="ORDER_SINGLE"/>
      <value enum="0" description="HEARTBEAT"/>
    </field>
    <field number="11" name="ClOrdID" type="STRING"/>
    <field number="54" name="Side" type="CHAR">
      <value enum="1" description="BUY"/>
      <value enum="2" description="SELL"/>
    </field>
  </fields>
</fix>)";
}

TEST_CASE("loads a QuickFIX dictionary from string", "[dictionary]") {
    auto dict = Dictionary::loadFromString(kMiniXml);
    REQUIRE(dict.has_value());
    REQUIRE(dict->id() == "FIX.4.4");
    REQUIRE(dict->fieldCount() == 3);
    REQUIRE(dict->messageCount() == 2);
}

TEST_CASE("resolves fields by number and name", "[dictionary]") {
    auto dict = Dictionary::loadFromString(kMiniXml);
    REQUIRE(dict->field(11) != nullptr);
    REQUIRE(dict->field(11)->name == "ClOrdID");
    REQUIRE(dict->field("Side") != nullptr);
    REQUIRE(dict->field("Side")->number == 54);
}

TEST_CASE("resolves enum descriptions", "[dictionary]") {
    auto dict = Dictionary::loadFromString(kMiniXml);
    REQUIRE(dict->enumDescription(54, "1") == "BUY");
    REQUIRE(dict->enumDescription(35, "D") == "ORDER_SINGLE");
    REQUIRE(dict->enumDescription(54, "9").empty());
}

TEST_CASE("resolves message names", "[dictionary]") {
    auto dict = Dictionary::loadFromString(kMiniXml);
    REQUIRE(dict->messageName("D") == "NewOrderSingle");
    REQUIRE(dict->messageName("0") == "Heartbeat");
    REQUIRE(dict->messageName("ZZ").empty());
}

TEST_CASE("rejects malformed XML", "[dictionary]") {
    auto dict = Dictionary::loadFromString("<not-fix><garbage");
    REQUIRE_FALSE(dict.has_value());
}

#ifdef FIXPARSER_DICT_DIR
TEST_CASE("loads the real FIX44 dictionary", "[dictionary][resources]") {
    auto dict = Dictionary::loadFromFile(std::string(FIXPARSER_DICT_DIR) + "/FIX44.xml");
    REQUIRE(dict.has_value());
    REQUIRE(dict->id() == "FIX.4.4");
    REQUIRE(dict->fieldCount() > 900);
    REQUIRE(dict->field(11) != nullptr);
    REQUIRE(dict->field(11)->name == "ClOrdID");
    REQUIRE(dict->messageName("D") == "NewOrderSingle");
    REQUIRE(dict->enumDescription(54, "1") == "BUY");
}
#endif

TEST_CASE("selects dictionary file from BeginString", "[selector]") {
    REQUIRE(DictionarySelector::selectFile("FIX.4.2") == "FIX42.xml");
    REQUIRE(DictionarySelector::selectFile("FIX.4.4") == "FIX44.xml");
    REQUIRE(DictionarySelector::selectFile("UNKNOWN") == "FIX44.xml");
}

TEST_CASE("selects dictionary for FIXT sessions", "[selector]") {
    REQUIRE(DictionarySelector::selectFile("FIXT.1.1", "", true) == "FIXT11.xml");
    REQUIRE(DictionarySelector::selectFile("FIXT.1.1", "7", false) == "FIX50SP2.xml");
    REQUIRE(DictionarySelector::selectFile("FIXT.1.1", "4", false) == "FIX44.xml");
}
