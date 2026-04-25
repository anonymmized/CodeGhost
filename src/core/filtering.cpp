#include "filtering.hpp"

bool TypesFilter::allow(const std::string& filename) {
    return !filename.ends_with("~") &&
        !filename.ends_with(".swp") &&
        !filename.ends_with(".swo") &&
        !filename.ends_with(".tmp") &&
        !filename.starts_with(".#");
}
