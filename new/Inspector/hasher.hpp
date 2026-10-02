#pragma once

#include <string>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include <openssl/evp.h>

class HashSession {
    private:
        std::ifstream file_;
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context_;
        std::array<char, 32768> buffer_{};
        bool started = false;
        std::string compute();
    public:
        explicit HashSession(const std::filesystem::path& path) : file_(path, std::ios::binary), context_(EVP_MD_CTX_new(), &EVP_MD_CTX_free) {
            if (!file_.is_open()) {
                throw std::runtime_error("Cannot open file: " + path.string());
            }
            if (!context_) {
                throw std::runtime_error("Cannot allocate hash context");
            }
            if (EVP_DigestInit_ex(context_.get(), EVP_sha256(), nullptr) != 1) {
                throw std::runtime_error("Cannot initialize SHA-256");
            }
        }
        ~HashSession() = default;
        HashSession(const HashSession&) = delete;
        static std::string hashFile(const std::filesystem::path& path);
};
