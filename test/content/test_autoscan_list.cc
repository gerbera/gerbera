#include <gtest/gtest.h>

#include "content/autoscan_list.h"
#include "content/autoscan.h"

TEST(AutoscanListTest, create)
{
    auto list = AutoscanList();

    auto adir = std::make_shared<AutoscanDirectory>();
    adir->setLocation({"/path/to/thing"});

    list.add(adir);

    EXPECT_EQ(list.size(), 1);

    EXPECT_EQ(list.get({"/path/to/thing"}), adir);

    EXPECT_EQ(list.get({"/path/to/other"}), nullptr);
}
