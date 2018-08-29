#include <common.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;

TEST(StringsTest, ConcatonatesValueUsingOperatorWhenStringHasValue) {

  zmm::String input = "string";
  std::ostringstream buf;

  buf << input;
  buf << "-exists";

  ASSERT_STREQ("string-exists", buf.str().c_str());
}

TEST(StringsTest, ConcatonatesBlankUsingOperatorWhenStringIsNULL) {

  zmm::String input = nullptr;
  std::ostringstream buf;

  buf << input;
  buf << "-isNULL";

  ASSERT_STREQ("-isNULL", buf.str().c_str());
}
