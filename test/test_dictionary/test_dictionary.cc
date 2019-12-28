/*GRB*
  Gerbera - https://gerbera.io/

  dictionary_test.cc - this file is part of Gerbera.

  Copyright (C) 2016-2019 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/
#include "test_dictionary.h"
#include "zmm/dictionary.h"

using namespace zmm;

void DictionaryTest::SetUp()
{
    dictionary2.put(std::string("keyOne"), std::string("valueOne"));
    dictionary2.put(std::string("keyTwo"), std::string("valueTwo"));
    dictionary2.put(std::string("keyThree"), std::string("valueThree"));
}

TEST_F(DictionaryTest, IsEmptyInitially)
{
    EXPECT_EQ(0, dictionary1.size());
}

TEST_F(DictionaryTest, PutAndGet)
{
    dictionary1.put(std::string("put1"), std::string("value1"));
    EXPECT_EQ(1, dictionary1.size());
    EXPECT_EQ(dictionary1.get(std::string("put1")), std::string("value1"));
}

TEST_F(DictionaryTest, PutWithExistingKeyReplacesValue)
{
    EXPECT_EQ(3, dictionary2.size());
    EXPECT_EQ(dictionary2.get(std::string("keyTwo")), std::string("valueTwo"));

    dictionary2.put(std::string("keyTwo"), std::string("replacementValue"));
    EXPECT_EQ(3, dictionary2.size());
    EXPECT_EQ(dictionary2.get(std::string("keyTwo")), std::string("replacementValue"));
}
