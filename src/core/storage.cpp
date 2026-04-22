#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <ctime>
#include "indexer.hpp"
#include "stdio.h"
const std::string storage_path = ".confighistory/history.bin";

std::vector<Change> storage_read(){
        std::fstream s{storage_path, s.binary | s.in };
        if (!s.is_open())
            throw std::runtime_error("[exception (runtime_error)]: storage ("+storage_path+") is not available for reading");
        std::vector<Change> changes;
        while (s.peek()!=-1){
            char c_time[8];        s.read(c_time,8);
            int64_t timestamp = *(int64_t*)(&c_time[0]);
            char c_hashblock[8];       s.read(c_hashblock,8);
            uint64_t hashblock = *(uint64_t*)(&c_hashblock[0]);
            char c_index[8];          s.read(c_index,8);
            size_t index = *(size_t*)(&c_index[0]);

            char c;
            std::string filename;    while (s.get(c) && c!='\0' && c!='EOF') filename.push_back(c);
            std::string old_content; while (s.get(c) && c!='\0' && c!='EOF') old_content.push_back(c);
            std::string new_content; while (s.get(c) && c!='\0' && c!='EOF') new_content.push_back(c);
            //todo: proper error-handling for invalid content 
            
            Change change{timestamp,hashblock,index,filename,old_content,new_content};
            changes.push_back(change); 
        }
        s.close();
        return changes;
}

//mt-unsafe corrupt
void* storage_append(const std::vector<Change>& changes){ 
    for ( Change change : changes ) {
        std::fstream s{storage_path, s.binary | s.out | s.app};
        if (!s.is_open())
            throw std::runtime_error("[exception (runtime_error)]: storage ("+storage_path+") could not be openned for writing");

        //timestamp, filepath, hashblock, old content, new content
        s.write(const_cast<const char*> ((char*)&change.timestamp), sizeof(change.timestamp));
        s.write(const_cast<const char*> ((char*)&change.hash), sizeof(change.hash));
        s.write(const_cast<const char*> ((char*)&change.block_index), sizeof(change.block_index));
        s.write(&change.file[0], change.file.size()); s.put('\0');
        s.write(&change.old_content[0], change.old_content.size()); s.put('\0');
        s.write(&change.new_content[0], change.new_content.size()); s.put('\0');
        s.close();
    }
    return nullptr;
}

/*
int main(void){
    Change c1{4, 4, 4, "file1","old_cont1", "new_cont1"};
    Change c2{1, 1, 1, "file2","old_cont2", "new_cont2"};
    std::vector<Change> vec;
    vec.push_back(c1);
    vec.push_back(c2);
    storage_append(vec);
    std::vector<Change> vec2 = storage_read();
    for (Change change : vec2){
        std::cout<<change.file<<"\n";
    }
}
*/
