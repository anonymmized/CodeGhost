#pragma once

class TypeFilter {
    private:
        std::vector<std::string> allowed_extensions;
    public:
        explicit TypeFilter(std::vector<std::string> ext) : allowed_extensions(std::move(ext)) {}
        bool allow(const std::string& filename);
};
