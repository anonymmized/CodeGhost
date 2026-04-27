#include "filtering.hpp"

bool TypeFilter::allow(const std::string& filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string::npos) return false;
    std::string ext = filename.substr(pos);
    return allowed_extensions.contains(ext);
}
