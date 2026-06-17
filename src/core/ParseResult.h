#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <memory>

namespace fixparser {

enum class Severity { Info, Warning, Error };

// A single resolved field after the semantic pass. When no dictionary match is
// available, name/typeName/enumDescription are left empty and the raw tag/value
// still carry the wire data.
struct ParsedField {
    int tag = 0;
    std::string value;
    std::string name;            // e.g. "MsgType"  (empty if unknown tag)
    std::string typeName;        // e.g. "CHAR", "PRICE" (empty if unknown)
    std::string enumDescription; // e.g. "NewOrderSingle" for 35=D (empty if none)
};

// One repeating-group instance (e.g. one NoPartyIDs entry). Holds its own
// fields and any nested groups, mirroring the QuickFIX group structure.
struct GroupInstance;

struct RepeatingGroup {
    int countTag = 0;            // the NUMINGROUP tag (e.g. 453 NoPartyIDs)
    std::string name;            // group name from dictionary
    std::vector<GroupInstance> instances;
};

struct GroupInstance {
    std::vector<ParsedField> fields;
    std::vector<RepeatingGroup> groups;
};

struct ParseDiagnostic {
    std::size_t pos = 0;
    Severity severity = Severity::Warning;
    std::string message;
};

// Result of parsing a single FIX message.
struct ParseResult {
    bool isFix = false;          // looked like a FIX message at all
    bool structurallyValid = false;
    std::string beginString;     // tag 8
    std::string msgType;         // tag 35
    std::string msgTypeName;     // resolved from dictionary
    std::string dictionary;      // dictionary id used for the semantic pass

    std::vector<ParsedField> header;
    std::vector<ParsedField> body;
    std::vector<ParsedField> trailer;
    std::vector<RepeatingGroup> groups;

    std::vector<ParseDiagnostic> diagnostics;

    bool hasErrors() const {
        for (const auto& d : diagnostics)
            if (d.severity == Severity::Error) return true;
        return false;
    }
};

} // namespace fixparser
