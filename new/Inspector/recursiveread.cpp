#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    const std::filesystem::path sandbox{"sandbox"};
    std::filesystem::create_directories(sandbox/"dir1"/"dir2");
    //std::ofstream{sandbox/"file1.txt"};
    //std::ofstream{sandbox/"file2.txt"};

    std::cout << "\nrecursive_directory_iterator:\n";
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{sandbox}) {
        if (dir_entry.is_regular_file()) {
            
            std::cout << "The file: " << dir_entry.path() << " size: " << dir_entry.file_size() << " bytes\n";
            std::cout << "Directory: " << dir_entry.path().parent_path() << '\n';
        }
    }

    // delete the sandbox dir and all contents within it, including subdirs
    //std::filesystem::remove_all(sandbox);
}
