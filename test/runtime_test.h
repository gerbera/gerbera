/*GRB*
  Gerbera - https://gerbera.io/

  runtime_test.h - this file is part of Gerbera.

  Copyright (C) 2016-2018 Gerbera Contributors

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
#ifndef __RUNTIME_TEST_H__
#define __RUNTIME_TEST_H__

#include "dictionary.h"
#include "gtest/gtest.h"

class RuntimeTest : public ::testing::Test {
protected:

    // You can do set-up work for each test here.
    inline RuntimeTest(){};

    // You can do clean-up work that doesn't throw exceptions here.
    inline virtual ~RuntimeTest(){};

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    // Code here will be called immediately after the constructor (right
    // before each test).
    inline virtual void SetUp() {};

    // Code here will be called immediately after each test (right
    // before the destructor).
    inline virtual void TearDown() {};
};

#endif // __RUNTIME_TEST_H__
