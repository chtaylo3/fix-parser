#include "DictionarySelector.h"

namespace fixparser {

std::string DictionarySelector::applVerIdToFile(const std::string& applVerID) {
    // ApplVerID enum (tag 1128) -> dictionary file. Values per FIX spec.
    if (applVerID == "0") return "FIX40.xml";
    if (applVerID == "1") return "FIX41.xml";
    if (applVerID == "2") return "FIX42.xml";
    if (applVerID == "3") return "FIX43.xml";
    if (applVerID == "4") return "FIX44.xml";
    if (applVerID == "5") return "FIX50.xml";
    if (applVerID == "6") return "FIX50SP1.xml";
    if (applVerID == "7") return "FIX50SP2.xml";
    if (applVerID == "8") return "FIX50SP2.xml"; // FIX50SP2 EP variants
    if (applVerID == "9") return "FIX50SP2.xml";
    return kDefaultFile;
}

std::string DictionarySelector::selectFile(const std::string& beginString,
                                           const std::string& applVerID,
                                           bool isAdminMsg) {
    if (beginString == "FIX.4.0") return "FIX40.xml";
    if (beginString == "FIX.4.1") return "FIX41.xml";
    if (beginString == "FIX.4.2") return "FIX42.xml";
    if (beginString == "FIX.4.3") return "FIX43.xml";
    if (beginString == "FIX.4.4") return "FIX44.xml";

    if (beginString == "FIXT.1.1") {
        // Session-level (admin) messages are defined in the transport dictionary.
        if (isAdminMsg) return "FIXT11.xml";
        if (!applVerID.empty()) return applVerIdToFile(applVerID);
        return "FIX50SP2.xml"; // sensible default application dictionary
    }

    return kDefaultFile;
}

} // namespace fixparser
