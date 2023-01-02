/*GRB*
    Gerbera - https://gerbera.io/

    runtime_test.cc - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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
#include "content/scripting/scripting_runtime.h"

TEST_F(RuntimeTest, CheckTestCodeLinksAgainstDependencies)
{
    auto runtime = std::make_unique<ScriptingRuntime>();
    auto ctx = runtime->createContext("testCtx");
    EXPECT_NE(ctx, nullptr);
}
