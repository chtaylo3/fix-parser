#include "Dictionary.h"

#include <pugixml.hpp>

#include <fstream>
#include <sstream>

namespace fixparser {

std::optional<Dictionary> Dictionary::loadFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return loadFromString(ss.str());
}

std::optional<Dictionary> Dictionary::loadFromString(const std::string& xml) {
    Dictionary dict;
    if (!dict.parseBuffer(xml.data(), xml.size())) return std::nullopt;
    return dict;
}

bool Dictionary::parseBuffer(const char* data, std::size_t len) {
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_buffer(data, len);
    if (!res) return false;

    pugi::xml_node fix = doc.child("fix");
    if (!fix) fix = doc.child("fixt"); // some dictionaries use <fixt>
    if (!fix) return false;

    // Build the id, e.g. "FIX.4.4" or "FIXT.1.1".
    {
        std::string type = fix.attribute("type").as_string("FIX");
        std::string major = fix.attribute("major").as_string("");
        std::string minor = fix.attribute("minor").as_string("");
        id_ = type;
        if (!major.empty()) id_ += "." + major;
        if (!minor.empty()) id_ += "." + minor;
    }

    // Fields.
    for (pugi::xml_node f : fix.child("fields").children("field")) {
        FieldDef def;
        def.number = f.attribute("number").as_int(0);
        def.name = f.attribute("name").as_string("");
        def.type = f.attribute("type").as_string("");
        if (def.number == 0) continue;
        for (pugi::xml_node v : f.children("value")) {
            std::string e = v.attribute("enum").as_string("");
            std::string d = v.attribute("description").as_string("");
            if (!e.empty()) def.values.emplace(std::move(e), std::move(d));
        }
        nameToNumber_[def.name] = def.number;
        byNumber_.emplace(def.number, std::move(def));
    }

    // Messages.
    for (pugi::xml_node m : fix.child("messages").children("message")) {
        MessageDef def;
        def.name = m.attribute("name").as_string("");
        def.msgType = m.attribute("msgtype").as_string("");
        def.category = m.attribute("msgcat").as_string("");
        if (def.msgType.empty()) continue;
        messages_[def.msgType] = std::move(def);
    }

    return !byNumber_.empty();
}

const FieldDef* Dictionary::field(int number) const {
    auto it = byNumber_.find(number);
    return it == byNumber_.end() ? nullptr : &it->second;
}

const FieldDef* Dictionary::field(const std::string& name) const {
    auto it = nameToNumber_.find(name);
    if (it == nameToNumber_.end()) return nullptr;
    return field(it->second);
}

std::string Dictionary::enumDescription(int fieldNumber, const std::string& value) const {
    const FieldDef* f = field(fieldNumber);
    if (!f) return {};
    auto it = f->values.find(value);
    return it == f->values.end() ? std::string{} : it->second;
}

std::string Dictionary::messageName(const std::string& msgType) const {
    auto it = messages_.find(msgType);
    return it == messages_.end() ? std::string{} : it->second.name;
}

} // namespace fixparser
