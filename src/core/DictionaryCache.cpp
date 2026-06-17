#include "DictionaryCache.h"

#include "DictionarySelector.h"

namespace fixparser {

const Dictionary* DictionaryCache::load(const std::string& fileName) {
    auto it = cache_.find(fileName);
    if (it != cache_.end()) return it->second.get();

    std::string path = dictDir_;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += fileName;

    auto loaded = Dictionary::loadFromFile(path);
    std::unique_ptr<Dictionary> ptr =
        loaded ? std::make_unique<Dictionary>(std::move(*loaded)) : nullptr;
    const Dictionary* raw = ptr.get();
    cache_.emplace(fileName, std::move(ptr));
    return raw;
}

const Dictionary* DictionaryCache::forMessage(const std::string& beginString,
                                              const std::string& applVerID,
                                              bool isAdminMsg) {
    std::string file = DictionarySelector::selectFile(beginString, applVerID, isAdminMsg);
    const Dictionary* d = load(file);
    if (d) return d;
    if (file != DictionarySelector::kDefaultFile)
        return load(DictionarySelector::kDefaultFile);
    return nullptr;
}

} // namespace fixparser
