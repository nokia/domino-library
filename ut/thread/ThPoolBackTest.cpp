#define  THREAD_BACK_TYPE  ThPoolBack
#define  THREAD_BACK_TEST  ThPoolBackTest
#include "ThreadBackTest.hpp"
#undef   THREAD_BACK_TYPE
#undef   THREAD_BACK_TEST

namespace rlib
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
}

// ***********************************************************************************************
TEST_F(ThPoolBackTest, performance)
{
    const size_t maxThread = 10;  // github ci can afford 100 (1000+ slower than belinb03)

    ThPoolBack thPoolBack(maxThread);
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < maxThread; i++)
        EXPECT_TRUE(thPoolBack.newTaskOK(
            []  // entryFn
            {
                this_thread::yield();  // hung like real time-cost task
                return true;
            },
            [](bool) {}  // backFn
        ));
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += thPoolBack.hdlFinishedTasks());
    auto dur = duration_cast<chrono::microseconds>(high_resolution_clock::now() - start);
    HID("ThPoolBack cost=" << dur.count() << "us");
    // - belinb03 : ~ 20 faster than AsyncBack
    // - github ci: ~100 slower than AsyncBack

    AsyncBack asyncBack;
    start = high_resolution_clock::now();
    for (size_t i = 0; i < maxThread; i++)
        EXPECT_TRUE(asyncBack.newTaskOK(
            []  // entryFn
            {
                this_thread::yield();  // hung like real time-cost task
                return true;
            },
            [](bool) {}  // backFn
        ));
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += asyncBack.hdlFinishedTasks());
    dur = duration_cast<chrono::microseconds>(high_resolution_clock::now() - start);
    HID("AsyncBack cost=" << dur.count() << "us");
}

}  // namespace
