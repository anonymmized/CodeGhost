#include "utils.hpp"

bool shouldIgnoreFile(const std::string& name) {
    if (name.empty()) return true;
    if (name.starts_with(".#")) return true;
    if (name.ends_with("~") || name.ends_with(".swp") || name.ends_with(".swo") || name.ends_with(".tmp")) return true;
    if (name[0] == '.') return true;
    return false;
}
