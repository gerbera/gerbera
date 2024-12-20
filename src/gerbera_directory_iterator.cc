#include "gerbera_directory_iterator.h"
#include <regex>

std::vector<std::filesystem::directory_entry>::iterator begin(gerbera_directory_iterator &it)
{ 
    return it.begin(); 
}

std::vector<std::filesystem::directory_entry>::iterator end(gerbera_directory_iterator &it)
{
    return it.end();    
}

gerbera_directory_iterator::gerbera_directory_iterator(const std::filesystem::directory_entry &de, std::error_code &ec)
{
    try
    {
        for(const auto &entry : std::filesystem::directory_iterator(de, ec))
            sortedContents.push_back(entry);
    }
    catch(...)
    {
    }

    process_sort();
}

gerbera_directory_iterator::gerbera_directory_iterator(const std::string &path, std::error_code &ec)
{
    try
    {
        for(const auto &entry : std::filesystem::directory_iterator(path, ec))
            sortedContents.push_back(entry);
    }
    catch(...)
    {
    }

    process_sort();
}

void gerbera_directory_iterator::process_sort()
{
    if(sortedContents.empty())
        return;

    std::regex stemRe("^([0-9]*)[^0-9]*([0-9]*)$");

    typedef std::vector<std::filesystem::directory_entry> DirVec;
    typedef std::filesystem::directory_entry DirEntry;

    DirVec sortedFolders;
    DirVec sortedFiles;

    for(const DirEntry &item : sortedContents)
    {
        if(item.is_directory())
            sortedFolders.push_back(item);
        else
            sortedFiles.push_back(item);
    }
    sortedContents.clear();

    for(DirVec *vec : std::vector<DirVec *>{ &sortedFolders, &sortedFiles })
    {
        if(vec->empty())
            continue;

        std::map<int, DirEntry> frontMap;
        std::map<int, DirEntry> backMap;

        for(size_t i = 0; i < vec->size();)
        {
            std::smatch stemMatch;
            std::filesystem::path filePath = vec->at(i).path();
            std::string fileStem = filePath.stem();

            if(std::regex_match(fileStem, stemMatch, stemRe))
            {
                std::string frontStr = stemMatch[1].str();
                std::string backStr = stemMatch[2].str();

                if(!frontStr.empty())
                    frontMap.insert({ std::stoi(frontStr), vec->at(i) });

                if(!backStr.empty())
                    backMap.insert({ std::stoi(backStr), vec->at(i) });
            }

            i++;

            if(frontMap.size() < i && backMap.size() < i)
                break;
        }

        if(frontMap.size() == vec->size())
        {
            for(auto it = frontMap.cbegin(); it != frontMap.cend(); ++it)
                sortedContents.push_back(it->second);
        }
        else
        {
            if(backMap.size() == vec->size())
            {
                for(auto it = backMap.cbegin(); it != backMap.cend(); ++it)
                    sortedContents.push_back(it->second);
            }
            else
            {
                std::sort(vec->begin(), vec->end(), [](const DirEntry &lhs, const DirEntry &rhs)
                    {
                        return lhs.path() < rhs.path();
                    });

                for(const DirEntry &item : *vec)
                    sortedContents.push_back(item);
            }
        }
    }
}
