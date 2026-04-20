#include <fstream>
#include <filesystem>
#include <vector>
#include <xxhash.h>
#include <string>
#include <algorithm>
#include <mutex>
#include "indexer.hpp"

uint64_t hashBlock(const std::string& block) {
    // использую для сравнения блоков тк сильно быстрее строк
    return XXH3_64bits(block.data(), block.size());
}

void Indexer::remove(const std::string& path) {
    auto it = state.find(path);
    if (it != state.end()) {
        state.erase(it);
    }
}

std::vector<std::string> makeBlocks(const std::vector<std::string>& lines, size_t block_size) {
    std::vector<std::string> blocks;
    // тут я разбиваю файл на фиксированные блоки по 5 строк (пока фикс)
    for (size_t i = 0; i < lines.size(); i += block_size) { // иду с шагом в размер блока
        std::string block;
        for (size_t j = i; j < i + block_size && j < lines.size(); j++) { // разбираю одну итерацию i
            block += lines[j];
            block += '\n'; // нормализую формат тк в ином случае будет отличаться хеш
        }
        blocks.push_back(block);
    }
    return blocks;
}

std::vector<Change> Indexer::process(const std::string& filename, size_t block_size) {
    // если файла нет, то удаляю его из state потому что могут быть несуществующие файлы
    if (!std::filesystem::exists(filename)) {
        std::lock_guard<std::mutex> lock(state_mutex);
        state.erase(filename);
        return {};
    }
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return {}; // позже будем логировать
    }
    // проверяю, является ли файл бинарником, если есть \0 то игнорим, тк файл бинарный
    char buf_check[512];
    infile.read(buf_check, sizeof(buf_check));
    for (std::streamsize i = 0; i < infile.gcount(); ++i) {
        if (buf_check[i] == '\0') {
            std::lock_guard<std::mutex> lock(state_mutex);
            state.erase(filename);
            return {};
        }
    }
    // в начало списка возвращаемся
    infile.clear();
    infile.seekg(0);

    std::vector<std::string> lines;
    std::string line;
    // построчно читаем файл
    while (std::getline(infile, line)) lines.push_back(line);
    std::vector<std::string> blocks = makeBlocks(lines, block_size);
    std::vector<uint64_t> new_hashes;
    new_hashes.reserve(blocks.size());
    // каждый блок хешируем, состояние файла вроде получаются компактнее строк
    for (const auto& b : blocks) {
        new_hashes.push_back(hashBlock(b));
    }
    std::lock_guard<std::mutex> lock(state_mutex);
    // сохраняем в переменной предыдущее состояние файла
    auto& old = state[filename];
    std::vector<Change> changes;
    // проходим по максимальной длине
    size_t max_len = std::max(old.hashes.size(), new_hashes.size());
    for (size_t i = 0; i < max_len; ++i) {
        // защита от выхода за границы, если блока нет, то считать как 0
        uint64_t old_h = (i < old.hashes.size()) ? old.hashes[i] : 0;
        uint64_t new_h = (i < new_hashes.size()) ? new_hashes[i] : 0;
        if (old_h != new_h) {
            // детект изменений
            Change c;
            c.file = filename;
            c.block_index = i;
            // если блока не было, то пустая строка
            c.old_content = (i < old.blocks.size()) ? old.blocks[i] : "";
            c.new_content = (i < blocks.size()) ? blocks[i] : "";
            changes.push_back(std::move(c));
        }
    }
    // обновляем состояние файла, те перезаписываем старое новым
    state[filename] = {new_hashes, blocks};
    return changes;
}
