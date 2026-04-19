#include <vector>
#include "indexer.hpp"
#include "searcher.hpp"

std::vector<Change> Searcher::find(const std::vector<Change>& changes, const std::string& query) {
    std::vector<Change> result;
    // линейный поиск по истории изменений пока o(n), надо будет оптимизировать под больший объем данных
    for (const auto& c : changes) {
        // тут важно: ищем не только в новом содержимом, но и в старом, потому что в дальнейшем это поможет понять, где код появился и антоним к слову появился
        if (c.new_content.find(query) != std::string::npos || c.old_content.find(query) != std::string::npos) {
            result.push_back(c);
        }
    }
    return result;
}
