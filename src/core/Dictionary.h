#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fixparser {

struct FieldDef {
    int number = 0;
    std::string name;
    std::string type;
    // enum value -> human description (e.g. "D" -> "ORDER_SINGLE")
    std::unordered_map<std::string, std::string> values;
};

struct MessageDef {
    std::string msgType;  // e.g. "D"
    std::string name;     // e.g. "NewOrderSingle"
    std::string category; // "admin" | "app"
};

// Immutable in-memory view of a QuickFIX data dictionary (one FIX version).
// Loaded once via pugixml; provides fast tag -> name/type/enum resolution.
class Dictionary {
public:
    static std::optional<Dictionary> loadFromFile(const std::string& path);
    static std::optional<Dictionary> loadFromString(const std::string& xml);

    const std::string& id() const { return id_; }   // e.g. "FIX.4.4"

    const FieldDef* field(int number) const;
    const FieldDef* field(const std::string& name) const;

    // Resolve an enum value's description for a given field, if defined.
    std::string enumDescription(int fieldNumber, const std::string& value) const;

    // Message type name (e.g. "D" -> "NewOrderSingle"); empty if unknown.
    std::string messageName(const std::string& msgType) const;

    std::size_t fieldCount() const { return byNumber_.size(); }
    std::size_t messageCount() const { return messages_.size(); }

private:
    // Parse a QuickFIX XML buffer; returns false on malformed input.
    bool parseBuffer(const char* data, std::size_t len);

    std::string id_;
    std::unordered_map<int, FieldDef> byNumber_;
    std::unordered_map<std::string, int> nameToNumber_;
    std::unordered_map<std::string, MessageDef> messages_;
};

} // namespace fixparser
