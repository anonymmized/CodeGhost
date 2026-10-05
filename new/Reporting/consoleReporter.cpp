#include "Reporting/consoleReporter.hpp"

#include <iostream>

void printChanges(const std::vector<FileChange>& changes) {
    if (changes.empty()) {
        std::cout << "No changes detected\n";
        return;
    }
    for (const auto& change : changes) {
        switch (change.type) {
            case ChangeType::Added: 
                std::cout << "ADDED\t" << change.path.string() << '\n';
                break;
            case ChangeType::Removed:
                std::cout << "REMOVED\t" << change.path.string() << '\n';
                break;
            case ChangeType::Modified: 
                std::cout << "MODIFIED\t" << change.path.string() << " [";
                if (change.contentChanged) {
                    std::cout << " content changed ";
                }
                if (change.sizeChanged) {
                    std::cout << " size changed ";
                }
                if (change.permissionsChanged) {
                    std::cout << " permissions changed "; 
                }
                if (change.modificationTimeChanged) {
                    std::cout << " modification time changed ";
                }
                std::cout << "]\n";
                break;
        }
    }
}

void printScanErrors(const std::vector<ScanError>& errors) {
    for (const auto& error : errors) {
        std::cerr << "ERROR\t" << error.path.string() << ": " << error.message << '\n';
    }
}
