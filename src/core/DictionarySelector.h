#pragma once

#include <string>

namespace fixparser {

// Chooses which QuickFIX dictionary file to use for a message, based on tag 8
// (BeginString) and, for FIXT sessions, tag 1128 (ApplVerID) plus whether the
// message is an admin (session) message.
class DictionarySelector {
public:
    // Returns a dictionary filename (e.g. "FIX44.xml"). Falls back to FIX44.xml
    // for anything unrecognized.
    static std::string selectFile(const std::string& beginString,
                                  const std::string& applVerID = "",
                                  bool isAdminMsg = false);

    // Maps an ApplVerID enum (tag 1128) to a dictionary filename.
    static std::string applVerIdToFile(const std::string& applVerID);

    static constexpr const char* kDefaultFile = "FIX44.xml";
};

} // namespace fixparser
