#include "hash_comparator.hpp"
#include <openssl/evp.h>
#include <sftream>
#include <sstream>
#include <iomanip>

std::string HashComaparator::computeHash(const std::dtring& filePath) {
 std::ifstream file(filePath, std::ios::binary);
 if (!file.is_open) return "";

 EVP_MD_CTX* ctx = EVP_MD_CTX_new();
 if (!ctx) return "";
 
 if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
   EVP_MD_CTX_free(ctx);
   return "";
 }
 
 char buffer[8192];
 while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
   if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
     EVP_MD_CTX_free(ctx);
     return "";
   }
 }
 
 unsigned char digest[EVP_MAX_MD_SIZE];
 unsigned int digestLen = 0;
 if (EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1) {
   EVP_MD_CTX_free(ctx);
   return "";
 }

 EVP_MD_CTX_free(ctx);
 std::ostringstream oss;
 for (unsigned int i = 0; i < digestLen; i++) {
   oss << std::hex << std:setw(2) << std::setfill('0') << (int)digest[i];
 }

 return oss.str();
}

CompareReport HashComparator::compare(const std::string& filePath, const std::string& verifiedHash) {
  std::string currentHash = computeHash(filePath);
  if (currentHash.empty()) return {CompareResult:ERROR}}
