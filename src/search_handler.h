#ifndef __SEARCH_HANDLER_H__
#define __SEARCH_HANDLER_H__

#include "cds_objects.h"

#include <memory>
#include <string>
#include <vector>

class SearchParam
{
protected:
    const std::string& containerID;
    std::string searchCriteria;
    int startingIndex;
    int requestedCount;

public:
    inline SearchParam(const std::string& containerID, const std::string& searchCriteria, int startingIndex, int requestedCount) :
        containerID(containerID),
        searchCriteria(searchCriteria),
        startingIndex(startingIndex),
        requestedCount(requestedCount)
         {}

};

class SearchHandler
{
public:
    SearchHandler();
    virtual ~SearchHandler();

    std::unique_ptr<std::vector<CdsObject>> executeSearch(SearchParam& searchParam);

protected:

private:
};

#endif // __SEARCH_HANDLER_H__
