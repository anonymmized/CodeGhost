#pragma once 

#include "Integrity/integrity.hpp"
#include "Inspector/types.hpp"

void printChanges(const std::vector<FileChange>& changes);
void printScanErrors(const std::vector<ScanError>& errors);
