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
#include "dictionary.h"
#include "strings.h"

using namespace zmm;

void DictionaryTest::SetUp()
{
    dictionary2.put(String("keyOne"), String("valueOne"));
    dictionary2.put(String("keyTwo"), String("valueTwo"));
    dictionary2.put(String("keyThree"), String("valueThree"));
}

TEST_F(DictionaryTest, IsEmptyInitially)
{
    EXPECT_EQ(0, dictionary1.size());
}

TEST_F(DictionaryTest, PutAndGet)
{
    dictionary1.put(String("put1"), String("value1"));
    EXPECT_EQ(1, dictionary1.size());
    EXPECT_EQ(dictionary1.get(String("put1")), String("value1"));
}

TEST_F(DictionaryTest, PutWithExistingKeyReplacesValue)
{
    EXPECT_EQ(3, dictionary2.size());
    EXPECT_EQ(dictionary2.get(String("keyTwo")), String("valueTwo"));

    dictionary2.put(String("keyTwo"), String("replacementValue"));
    EXPECT_EQ(3, dictionary2.size());
    EXPECT_EQ(dictionary2.get(String("keyTwo")), String("replacementValue"));
}
