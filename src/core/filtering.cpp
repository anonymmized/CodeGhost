#include "filtering.hpp"

bool TypeFilter::allow(const std::string& filename) {
    if (filename.starts_with('.')) return false;
    return filename.ends_with(".cpp") ||
        filename.ends_with(".hpp") ||
        filename.ends_with(".c") ||
        filename.ends_with(".h") ||
        filename.ends_with(".txt");
}
