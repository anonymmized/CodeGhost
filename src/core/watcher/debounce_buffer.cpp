#include "debounce_buffer.hpp"
#include <chrono>

void DebounceBuffer::touch(const std::string& path) {
    files[path].ts = std::chrono::steady_clock::now();
}
