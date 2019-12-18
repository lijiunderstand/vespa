// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/vespalib/testkit/testapp.h>
#include <vespa/vespalib/util/clock.h>
#include <vespa/fastos/thread.h>

using vespalib::Clock;
using vespalib::duration;
using vespalib::steady_time;


TEST("Test that clock is ticking forward") {

    Clock clock(0.050);
    FastOS_ThreadPool pool(0x10000);
    ASSERT_TRUE(pool.NewThread(clock.getRunnable(), nullptr) != nullptr);
    steady_time start = clock.getTimeNS();
    std::this_thread::sleep_for(5s);
    steady_time stop = clock.getTimeNS();
    EXPECT_TRUE(stop > start);
    std::this_thread::sleep_for(6s);
    clock.stop();
    steady_time stop2 = clock.getTimeNS();
    EXPECT_TRUE(stop2 > stop);
    EXPECT_TRUE(vespalib::count_ms(stop2 - stop) > 1000);
}

TEST_MAIN() { TEST_RUN_ALL(); }