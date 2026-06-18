// libFuzzer entry point for the FIX parser core.
//
// Build with the "fuzz" preset (clang-cl, -fsanitize=fuzzer,address). Then run
// from a Developer Command Prompt with LLVM on PATH (first dir collects new
// finds; the rest are read-only seeds):
//
//   build/fuzz/fixparser_fuzzer -max_len=65536 build/fuzz-corpus test/fuzz/seeds test/data/fixsim
//
// Coverage-guided fuzzing on a delimiter-sniffing, length-prefixed parser is
// exactly the workload it was designed for: ASan flags any over-read when a
// BodyLength or NumInGroup count lies about how much data follows.
#include "DelimiterNormalizer.h"
#include "FixParser.h"
#include "MessageFramer.h"
#include "PrettyPrinter.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    const std::string_view input(reinterpret_cast<const char*>(data), size);

    // Headline path: detect delimiter -> frame -> format one message per line.
    fixparser::PrettyPrinter::prettyPrint(input);

    // Structural + semantic single-message parse (no dictionary).
    fixparser::FixParser::parse(input);

    // Framing in isolation: the BodyLength-driven extent logic is the classic
    // over-read surface when the length field disagrees with the byte stream.
    const fixparser::NormalizedFix norm = fixparser::DelimiterNormalizer::normalize(input);
    fixparser::MessageFramer::split(norm.canonical);

    // The parser is meant to be total (returns diagnostics, never throws), so any
    // escaping exception is itself a finding -- deliberately not caught here.
    return 0;
}
