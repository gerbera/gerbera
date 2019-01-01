/*GRB*
    Gerbera - https://gerbera.io/

    runtime_test.cc - this file is part of Gerbera.

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
#include "test_runtime.h"
#include "scripting/runtime.h"

TEST_F(RuntimeTest, CheckTestCodeLinksAgainstDependencies)
{
    Runtime* runtime = new Runtime();
    duk_context* ctx = runtime->createContext("testCtx");
    EXPECT_NE(ctx, nullptr);
}
