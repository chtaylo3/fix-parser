#pragma once

#include "Dictionary.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace fixparser {

// Lazily loads and caches QuickFIX dictionaries from a directory, keyed by file
// name. Selection is driven by BeginString (+ ApplVerID for FIXT) via
// DictionarySelector.
class DictionaryCache {
public:
    explicit DictionaryCache(std::string dictDir) : dictDir_(std::move(dictDir)) {}

    // Returns a dictionary for the given message, or nullptr if none could be
    // loaded (caller should fall back to raw tag display).
    const Dictionary* forMessage(const std::string& beginString,
                                 const std::string& applVerID = "",
                                 bool isAdminMsg = false);

    // Load a specific dictionary file (cached). May return nullptr.
    const Dictionary* load(const std::string& fileName);

    const std::string& directory() const { return dictDir_; }

private:
    std::string dictDir_;
    std::unordered_map<std::string, std::unique_ptr<Dictionary>> cache_; // null = tried & failed
};

} // namespace fixparser
