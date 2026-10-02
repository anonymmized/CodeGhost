#include "Inspector/hasher.hpp"

#include <string>

std::string HashSession::hashFile(const std::filesystem::path& path) {
    HashSession session{path};
    return session.compute();
}

std::string HashSession::compute() {
    if (started) {
        throw std::logic_error("HashSession has already been used");
    }
    started = true;
    while(file_) {
        file_.read(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
        const std::streamsize bytesRead = file_.gcount();
        if (file_.bad() || (file_.fail() && !file_.eof())) {
            throw std::runtime_error("Failed to read file");
        }
        if (bytesRead > 0) {
            if (EVP_DigestUpdate(context_.get(), buffer_.data(), static_cast<std::size_t>(bytesRead)) != 1) {
                throw std::runtime_error("Failed to update SHA-256");
            }
        }
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0;
    if (EVP_DigestFinal_ex(context_.get(), digest.data(), &digestLength) != 1) {
        throw std::runtime_error("Unexpected SHA-256 digest length");
    }
    constexpr char hexDigits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digestLength * 2);
    for (unsigned int i = 0; i < digestLength; ++i) {
        const unsigned char byte = digest[i];
        result.push_back(hexDigits[byte >> 4]);
        result.push_back(hexDigits[byte & 0x0F]);
    }
    return result;
}
