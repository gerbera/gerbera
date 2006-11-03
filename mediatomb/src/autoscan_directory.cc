/*MT*/

#include "autoscan_directory.h"

using namespace zmm;

AutoscanDirectory::AutoscanDirectory(String location, scan_mode_t mode,
                      scan_level_t level, bool recursive,
                      unsigned int interval)
{
    this->location = location;
    this->mode = mode;
    this->level = level;
    this->recursive = recursive;
    this->interval = interval;
}

