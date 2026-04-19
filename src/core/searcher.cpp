#include <vector>
#include "indexer.hpp"
#include "searcher.hpp"

std::vector<Change> Searcher::find(const std::vector<Change>& changes, const std::string& query) {
    std::vector<Change> result;
    for (const auto& c : changes) {
        if (c.new_content.find(query) != std::string::npos || c.old_content.find(query) != std::string::npos) {
            result.push_back(c);
        }
    }
    return result;
}
