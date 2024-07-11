#define  THREAD_BACK       ThPoolBack
#define  THREAD_BACK_TEST  ThPoolBackTest
#include "ThreadBackTest.hpp"
#undef   THREAD_BACK
#undef   THREAD_BACK_TEST

namespace RLib
{
// ***********************************************************************************************
TEST_F(ThPoolBackTest, invalid_maxThread)
{
    ThPoolBack myPool(0);  // invalid maxThread=0
    EXPECT_TRUE(threadBack_.newTaskOK(
        [] { return true; },  // entryFn
        [](bool) {}  // backFn
    )) << "REQ: can create new task";
    while (threadBack_.hdlFinishedTasks() == 0);  // REQ: wait new task done

    myPool.fakeNotify();  // simulate wrong notify; inc cov
}

// ***********************************************************************************************
TEST_F(ThPoolBackTest, performance)
{
    const size_t maxThread = 10;  // github ci can afford 100 (1000+ slower than belinb03)

    AsyncBack asyncBack;
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < maxThread; i++)
        EXPECT_TRUE(asyncBack.newTaskOK(
            [] { return true; },  // entryFn
            [](bool) {}  // backFn
        ));
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += asyncBack.hdlFinishedTasks());
    auto dur = duration_cast<chrono::microseconds>(high_resolution_clock::now() - start);
    HID("AsyncBack cost=" << dur.count() << "us");

    ThPoolBack thPoolBack(maxThread);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < maxThread; i++)
        EXPECT_TRUE(thPoolBack.newTaskOK(
            [] { return true; },  // entryFn
            [](bool) {}  // backFn
        ));
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += thPoolBack.hdlFinishedTasks());
    dur = duration_cast<chrono::microseconds>(high_resolution_clock::now() - start);
    HID("ThPoolBack cost=" << dur.count() << "us");
    // - belinb03 : ~ 10 faster than AsyncBack
    // - github ci: ~100 faster than AsyncBack
}

}  // namespace
