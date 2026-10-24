/*GRB*

    Gerbera - https://gerbera.io/

    test_thread_runner.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera. If not, see <http://www.gnu.org/licenses/>.
*/

#include "util/thread_runner.h"

#include <future>
#include <gtest/gtest.h>

TEST(ThreadRunnerTest, ReadinessDoesNotWaitForWorkMutex)
{
    std::promise<StdThreadRunner*> publishedRunner;
    auto runner = publishedRunner.get_future();
    StdThreadRunner worker("ReadyThread", [&runner](void*) { runner.get()->setReady(); }, nullptr);

    // Timer readiness must not depend on the mutex used for queued work.
    auto workLock = worker.uniqueLock();
    publishedRunner.set_value(&worker);
    EXPECT_NO_THROW(worker.waitForReady());
    workLock.unlock();
    worker.join();
}
