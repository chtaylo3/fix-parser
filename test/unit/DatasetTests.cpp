#include <catch2/catch_test_macros.hpp>

#include "DelimiterNormalizer.h"
#include "Dictionary.h"
#include "FixParser.h"
#include "MessageFramer.h"
#include "PrettyPrinter.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace fixparser;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path, std::ios::binary);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

#ifdef FIXPARSER_DATA_DIR
const std::string kData = FIXPARSER_DATA_DIR;
#endif
#ifdef FIXPARSER_DICT_DIR
const std::string kDict = FIXPARSER_DICT_DIR;
#endif

} // namespace

#ifdef FIXPARSER_DATA_DIR

TEST_CASE("fixsim FIX.4.4 messages all parse", "[dataset][fixsim]") {
    auto lines = readLines(kData + "/fixsim/fix44.txt");
    REQUIRE(lines.size() == 8);
    for (const auto& line : lines) {
        auto r = FixParser::parse(line);
        INFO(line);
        REQUIRE(r.isFix);
        REQUIRE(r.structurallyValid);
        REQUIRE(r.beginString == "FIX.4.4");
        REQUIRE_FALSE(r.msgType.empty());
    }
}

TEST_CASE("fixsim FIX.4.2 messages all parse", "[dataset][fixsim]") {
    auto lines = readLines(kData + "/fixsim/fix42.txt");
    REQUIRE(lines.size() == 8);
    for (const auto& line : lines) {
        auto r = FixParser::parse(line);
        INFO(line);
        REQUIRE(r.isFix);
        REQUIRE(r.structurallyValid);
        REQUIRE(r.beginString == "FIX.4.2");
    }
}

TEST_CASE("fixsim messages round-trip through pretty-print framing", "[dataset][fixsim][pretty]") {
    std::string raw = readFile(kData + "/fixsim/fix44.txt");
    std::string out = PrettyPrinter::prettyPrint(raw);
    // 8 messages -> 8 lines (7 separators).
    std::size_t newlines = 0;
    for (char c : out) if (c == '\n') ++newlines;
    REQUIRE(newlines == 7);
}

TEST_CASE("QuickFIX .def initiator/expected messages parse", "[dataset][quickfix]") {
    namespace fs = std::filesystem;
    fs::path defs = fs::path(kData) / "quickfix" / "definitions";
    REQUIRE(fs::exists(defs));

    int parsed = 0;
    for (const auto& entry : fs::directory_iterator(defs)) {
        if (entry.path().extension() != ".def") continue;
        for (auto line : readLines(entry.path().string())) {
            // Only message lines: 'I'/'E' prefix immediately followed by "8=".
            if (line.size() < 3) continue;
            if ((line[0] != 'I' && line[0] != 'E') || line[1] != '8' || line[2] != '=')
                continue;
            std::string msg = line.substr(1);
            // Substitute the engine's <TIME> placeholder with a fixed timestamp.
            for (std::string::size_type p; (p = msg.find("<TIME>")) != std::string::npos;)
                msg.replace(p, 6, "20240101-00:00:00.000");
            auto r = FixParser::parse(msg);
            INFO(entry.path().filename().string() << ": " << line);
            REQUIRE(r.isFix);
            ++parsed;
        }
    }
    REQUIRE(parsed > 0);
}

TEST_CASE("OnixS nested-group examples parse", "[dataset][onixs]") {
    auto lines = readLines(kData + "/onixs/nested_groups.txt");
    REQUIRE(lines.size() == 3);
    for (const auto& line : lines) {
        auto r = FixParser::parse(line);
        INFO(line);
        REQUIRE(r.isFix);
        REQUIRE_FALSE(r.msgType.empty());
    }
}

TEST_CASE("messy mixed log frames only well-formed FIX messages", "[dataset][messy]") {
    std::string raw = readFile(kData + "/messy/messy_log.txt");

    NormalizedFix norm = DelimiterNormalizer::normalize(raw);
    REQUIRE(norm.detected == DelimiterStyle::Pipe);
    REQUIRE(norm.confidence == DetectionConfidence::Confident);

    auto bounds = MessageFramer::findBoundaries(norm.canonical);
    // Logon (A), Logout (5), NewOrderSingle (D) — the log-wrapped line and all
    // non-FIX lines are excluded.
    REQUIRE(bounds.size() == 3);

    std::vector<std::string> msgTypes;
    for (const auto& b : bounds) {
        std::string_view msg(norm.canonical.data() + b.start, b.end - b.start);
        INFO(std::string(msg));
        REQUIRE(msg.find("raw=") == std::string_view::npos);   // log-wrapped excluded
        REQUIRE(msg.find("INFO") == std::string_view::npos);   // log lines excluded
        auto r = FixParser::parse(msg);
        REQUIRE(r.isFix);
        REQUIRE(r.structurallyValid);
        REQUIRE(r.beginString == "FIX.4.4");
        msgTypes.push_back(r.msgType);
    }
    REQUIRE(msgTypes == std::vector<std::string>{"A", "5", "D"});
}

TEST_CASE("FXCM broker-style messages parse", "[dataset][fxcm]") {
    auto lines = readLines(kData + "/fxcm/messages.txt");
    REQUIRE(lines.size() == 3);
    for (const auto& line : lines) {
        auto r = FixParser::parse(line);
        INFO(line);
        REQUIRE(r.isFix);
        REQUIRE_FALSE(r.msgType.empty());
    }
}

#if defined(FIXPARSER_DICT_DIR)
TEST_CASE("fixsim NewOrderSingle resolves via FIX44 dictionary", "[dataset][fixsim][dictionary]") {
    auto dict = Dictionary::loadFromFile(kDict + "/FIX44.xml");
    REQUIRE(dict.has_value());
    auto lines = readLines(kData + "/fixsim/fix44.txt");
    auto r = FixParser::parse(lines.front(), &dict.value());
    REQUIRE(r.msgType == "D");
    REQUIRE(r.msgTypeName == "NewOrderSingle");

    bool sawSymbol = false;
    for (const auto& f : r.body) {
        if (f.tag == 55) { sawSymbol = true; REQUIRE(f.name == "Symbol"); }
        if (f.tag == 54) { REQUIRE(f.name == "Side"); REQUIRE(f.enumDescription == "BUY"); }
    }
    REQUIRE(sawSymbol);
}
#endif

#endif // FIXPARSER_DATA_DIR
