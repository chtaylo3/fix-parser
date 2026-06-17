#include "FixParser.h"

#include "Dictionary.h"
#include "DelimiterNormalizer.h"
#include "FixTokenizer.h"

#include <array>
#include <charconv>
#include <string>

namespace fixparser {

namespace {

// Standard header tags (minimal MVP set) and trailer tags. Anything else is body.
bool isHeaderTag(int tag) {
    switch (tag) {
        case 8:   // BeginString
        case 9:   // BodyLength
        case 35:  // MsgType
        case 34:  // MsgSeqNum
        case 49:  // SenderCompID
        case 56:  // TargetCompID
        case 52:  // SendingTime
        case 50:  // SenderSubID
        case 57:  // TargetSubID
        case 115: // OnBehalfOfCompID
        case 128: // DeliverToCompID
        case 43:  // PossDupFlag
        case 97:  // PossResend
        case 1128: // ApplVerID
            return true;
        default:
            return false;
    }
}

bool isTrailerTag(int tag) {
    return tag == 10 /*CheckSum*/ || tag == 89 /*Signature*/ || tag == 93 /*SignatureLength*/;
}

ParsedField makeField(const TagValue& tv, const Dictionary* dict) {
    ParsedField pf;
    pf.tag = tv.tag;
    pf.value.assign(tv.value.data(), tv.value.size());
    if (dict) {
        if (const FieldDef* fd = dict->field(tv.tag)) {
            pf.name = fd->name;
            pf.typeName = fd->type;
            auto it = fd->values.find(pf.value);
            if (it != fd->values.end()) pf.enumDescription = it->second;
        }
    }
    return pf;
}

// Compute the FIX checksum: sum of all bytes up to (not including) the "10="
// field, modulo 256. `canonical` is SOH-delimited.
bool computeChecksum(std::string_view canonical, int& outChecksum) {
    // The checksum covers everything up to and including the SOH before "10=".
    std::size_t pos = canonical.rfind("10=");
    // Ensure it's at a field boundary.
    while (pos != std::string_view::npos && pos != 0 && canonical[pos - 1] != FixTokenizer::SOH)
        pos = canonical.rfind("10=", pos == 0 ? 0 : pos - 1);
    if (pos == std::string_view::npos) return false;

    unsigned sum = 0;
    for (std::size_t i = 0; i < pos; ++i)
        sum += static_cast<unsigned char>(canonical[i]);
    outChecksum = static_cast<int>(sum % 256);
    return true;
}

} // namespace

ParseResult FixParser::parse(std::string_view rawMessage, const Dictionary* dict) {
    ParseResult result;

    NormalizedFix norm = DelimiterNormalizer::normalize(rawMessage);
    if (norm.confidence == DetectionConfidence::None) {
        result.diagnostics.push_back({0, Severity::Warning, "no FIX field delimiter detected"});
        return result;
    }

    TokenResult tokens = FixTokenizer::tokenize(norm.canonical);
    for (const auto& e : tokens.errors)
        result.diagnostics.push_back({e.pos, Severity::Warning, e.reason});

    if (tokens.fields.empty()) return result;

    // Structural checks.
    const TagValue& first = tokens.fields.front();
    result.isFix = (first.tag == 8);
    if (first.tag != 8)
        result.diagnostics.push_back({0, Severity::Error, "missing BeginString (tag 8) as first field"});

    bool sawBodyLength = false, sawMsgType = false, sawChecksum = false;
    std::string checksumValue;

    for (const auto& tv : tokens.fields) {
        ParsedField pf = makeField(tv, dict);

        switch (tv.tag) {
            case 8:  result.beginString.assign(tv.value.data(), tv.value.size()); break;
            case 9:  sawBodyLength = true; break;
            case 35:
                sawMsgType = true;
                result.msgType.assign(tv.value.data(), tv.value.size());
                if (dict) result.msgTypeName = dict->messageName(result.msgType);
                break;
            case 10:
                sawChecksum = true;
                checksumValue.assign(tv.value.data(), tv.value.size());
                break;
            default: break;
        }

        if (isHeaderTag(tv.tag)) result.header.push_back(std::move(pf));
        else if (isTrailerTag(tv.tag)) result.trailer.push_back(std::move(pf));
        else result.body.push_back(std::move(pf));
    }

    if (dict) result.dictionary = dict->id();

    if (!sawBodyLength)
        result.diagnostics.push_back({0, Severity::Warning, "missing BodyLength (tag 9)"});
    if (!sawMsgType)
        result.diagnostics.push_back({0, Severity::Error, "missing MsgType (tag 35)"});
    if (!sawChecksum)
        result.diagnostics.push_back({0, Severity::Warning, "missing CheckSum (tag 10)"});

    // Validate checksum (warning only — venue logs are often truncated).
    if (sawChecksum) {
        int computed = 0;
        int declared = -1;
        std::from_chars(checksumValue.data(), checksumValue.data() + checksumValue.size(), declared);
        if (computeChecksum(norm.canonical, computed) && computed != declared) {
            result.diagnostics.push_back(
                {0, Severity::Warning,
                 "checksum mismatch: computed " + std::to_string(computed) +
                     ", declared " + std::to_string(declared)});
        }
    }

    result.structurallyValid =
        result.isFix && sawMsgType && !result.hasErrors();
    return result;
}

} // namespace fixparser
