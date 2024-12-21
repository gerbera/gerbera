#ifndef GERBERA_DIRECTORY_ITERATOR_H
#define GERBERA_DIRECTORY_ITERATOR_H

#include <filesystem>
#include <string>
#include <vector>

/// Custom iterator for human-oriented sorting (right order for numbers without leading zeros)
///  if option HAVE_HUMAN_SORTING is disabled, using dictionary sort
class gerbera_directory_iterator {
public:
    gerbera_directory_iterator(const std::filesystem::directory_entry& de, std::error_code& ec);
    gerbera_directory_iterator(const std::string& path, std::error_code& ec);

    std::vector<std::filesystem::directory_entry>::iterator begin() { return sortedContents.begin(); }
    std::vector<std::filesystem::directory_entry>::iterator end() { return sortedContents.end(); }

private:
    void process_sort();

    std::vector<std::filesystem::directory_entry> sortedContents;
};

std::vector<std::filesystem::directory_entry>::iterator begin(gerbera_directory_iterator& it);
std::vector<std::filesystem::directory_entry>::iterator end(gerbera_directory_iterator& it);

#endif // GERBERA_DIRECTORY_ITERATOR_H
