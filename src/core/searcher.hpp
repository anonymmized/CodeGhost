#pragma once
#include <vector>
#include "indexer.hpp"

class Searcher {
    public:
        std::vector<Change> find(const std::vector<Change>& changes, const std::string& query);
};
