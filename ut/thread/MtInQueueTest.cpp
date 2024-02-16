/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <future>
#include <gtest/gtest.h>
#include <queue>
#include <thread>
#include <unistd.h>

#include "MT_Semaphore.hpp"
#include "UniLog.hpp"

#define MT_IN_Q_TEST
#include "MT_PingMainTH.hpp"
#include "MtInQueue.hpp"
#undef MT_IN_Q_TEST

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct MtInQueueTest : public Test, public UniLog
{
    MtInQueueTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {}
    void SetUp() override
    {
        mt_getQ().mt_clear();  // avoid other case interfere
    }
    ~MtInQueueTest() { GTEST_LOG_FAIL }
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_simple_fifo)
{
    mt_getQ().mt_push<int>(MAKE_PTR<int>(1));
    mt_getQ().mt_push<int>(MAKE_PTR<int>(2));
    EXPECT_EQ(1, *(mt_getQ().pop<int>().get())) << "REQ: fifo";
    EXPECT_EQ(2, *(mt_getQ().pop<int>().get())) << "REQ: fifo";
}
TEST_F(MtInQueueTest, GOLD_sparsePush_fifo)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [&nMsg]()
    {
        for (int i = 0; i < nMsg; i++)
        {
            mt_getQ().mt_push(MAKE_PTR<int>(i));
            this_thread::sleep_for(1us);  // simulate real world (sparse msg)
        }
    });

    int nHdl = 0;
    while (nHdl < nMsg)
    {
        auto msg = mt_getQ().pop<int>();  // directly get() will destruct shared_ptr afterward
        if (msg.get())
        {
            HID("nHdl=" << nHdl << ", msg=" << msg.get() << ", nRef=" << msg.use_count());
            SCOPED_TRACE(nHdl);
            EXPECT_EQ(nHdl, *(msg.get())) << "REQ: fifo";
            ++nHdl;
        }
        else
        {
            HID("nHdl=" << nHdl << ", nRef=" << msg.use_count());
            timedwait();  // REQ: less CPU than repeat pop() or this_thread::yield()
        }
    }
    DBG("REQ(sleep 1us/push): e2e user=0.354s->0.123s, sys=0.412s->0.159s")
}
TEST_F(MtInQueueTest, GOLD_surgePush_fifo)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [&nMsg]()
    {
        for (int i = 0; i < nMsg; i++)
        {
            mt_getQ().mt_push(MAKE_PTR<int>(i));  // surge push
        }
    });

    int nHdl = 0;
    DBG("before loop")
    while (nHdl < nMsg)
    {
        auto msg = mt_getQ().pop<int>();
        if (msg.get())
        {
            SCOPED_TRACE(nHdl);
            EXPECT_EQ(nHdl++, *(msg.get())) << "REQ: fifo";
        }
        else this_thread::yield();  // REQ: test cache_ performance
    }
    DBG("REQ: loop cost 2576us now, previously no cache & lock cost 4422us")
}
TEST_F(MtInQueueTest, GOLD_nonBlock_pop)
{
    ASSERT_EQ(nullptr, mt_getQ().pop<void>().get()) << "REQ: can pop empty";

    mt_getQ().mt_push(MAKE_PTR<string>("1st"));
    mt_getQ().mt_push(MAKE_PTR<string>("2nd"));
    mt_getQ().backdoor().lock();
    timedwait(600);  // push shall wakeup this func instead of 10m timeout
    ASSERT_EQ(nullptr, mt_getQ().pop<void>().get()) << "REQ: not blocked";
    timedwait(600);  // REQ: pop() shall mt_pingMainTH() if can't access queue_

    mt_getQ().backdoor().unlock();
    ASSERT_EQ("1st", *(mt_getQ().pop<string>().get())) << "REQ: can pop";

    mt_getQ().backdoor().lock();
    ASSERT_EQ("2nd", *(mt_getQ().pop<string>().get())) << "REQ: can pop from cache";
    mt_getQ().backdoor().unlock();
}
TEST_F(MtInQueueTest, size_and_nowait)
{
    mt_getQ().mt_push<int>(MAKE_PTR<int>(1));
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ())  << "REQ: inc size"  << endl;
    timedwait(600);  // ut can't tolerate 10m timer
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ())  << "REQ: wait() ret immediately since mt_getQ() not empty"  << endl;

    mt_getQ().mt_push<int>(MAKE_PTR<int>(2));
    ASSERT_EQ(2u, mt_getQ().mt_sizeQ())  << "REQ: inc size"  << endl;

    EXPECT_EQ(1, *(mt_getQ().pop<int>().get())) << "REQ: fifo";
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ())  << "REQ: dec size"  << endl;
    timedwait(600);
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ())  << "REQ: wait() ret immediately since mt_getQ() not empty"  << endl;

    mt_getQ().mt_push<int>(MAKE_PTR<int>(3));
    timedwait(600);
    ASSERT_EQ(2u, mt_getQ().mt_sizeQ())  << "REQ: wait() ret immediately since mt_getQ() not empty"  << endl;

    EXPECT_EQ(2, *(mt_getQ().pop<int>().get())) << "REQ: keep fifo after wait_for()";
    EXPECT_EQ(3, *(mt_getQ().pop<int>().get())) << "REQ: keep fifo after wait_for()";
    ASSERT_EQ(0u, mt_getQ().mt_sizeQ())  << "REQ: dec size"  << endl;
}

#define DESTRUCT
// ***********************************************************************************************
struct TestObj
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST_F(MtInQueueTest, GOLD_destructCorrectly)
{
    bool isDestructed;
    mt_getQ().mt_push(MAKE_PTR<TestObj>(isDestructed));
    ASSERT_FALSE(isDestructed);

    mt_getQ().mt_clear();
    ASSERT_TRUE(isDestructed) << "REQ: destruct correctly";
}
TEST_F(MtInQueueTest, clear)
{
    mt_getQ().mt_push<void>(nullptr);
    mt_getQ().mt_push<void>(nullptr);
    mt_getQ().pop();
    mt_getQ().mt_push<void>(nullptr);
    mt_getQ().hdlr<void>([](UniPtr){});

    mt_getQ().mt_clear();
    ASSERT_EQ(0u, mt_getQ().mt_sizeQ()) << "REQ: clear all ele";
    EXPECT_EQ(0u, mt_getQ().nHdlr()) << "REQ: clear all hdlr";
}

#define HDLR
// ***********************************************************************************************
// normal covered by MT_SemaphoreTest
TEST_F(MtInQueueTest, discard_noHdlrEle)
{
    mt_getQ().mt_push<void>(nullptr);
    EXPECT_EQ(1u, mt_getQ().mt_sizeQ());

    mt_getQ().handleAllEle();
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ()) << "REQ: discard ele w/o hdlr - simple & no mem leak";
}
TEST_F(MtInQueueTest, handleAllEle_shallnot_block)
{
    MtInQueue mtQ;  // cov destructor with ele left

    mtQ.mt_push<void>(nullptr);
    EXPECT_EQ(1u, mtQ.mt_sizeQ());
    mtQ.backdoor().lock();

    mtQ.handleAllEle();
    mtQ.backdoor().unlock();  // for mt_sizeQ()
    EXPECT_EQ(1u, mtQ.mt_sizeQ()) << "REQ: no block";
}
TEST_F(MtInQueueTest, GOLD_shallHandle_bothCacheAndQueue_ifPossible)
{
    // init
    size_t nCalled = 0;
    mt_getQ().hdlr<void>([&nCalled](UniPtr){ ++nCalled; });
    mt_getQ().mt_push<void>(nullptr);
    mt_getQ().mt_push<void>(nullptr);
    mt_getQ().pop();  // still 1 ele in cache_
    mt_getQ().mt_push<void>(nullptr);  // and 1 ele in queue_
    EXPECT_EQ(2u, mt_getQ().mt_sizeQ());

    mt_getQ().backdoor().lock();
    timedwait(600);  // last push shall wakeup this func instead of 10m timeout
    EXPECT_EQ(1u, mt_getQ().handleAllEle()) << "REQ: shall handle cache_";
    EXPECT_EQ(1u, nCalled) << "REQ: hdlr is called";
    timedwait(600);  // REQ: handleAllEle() shall mt_pingMainTH() if can't access queue_

    mt_getQ().backdoor().unlock();
    mt_getQ().handleAllEle();
    EXPECT_EQ(2u, nCalled) << "REQ: hdlr is called";
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ()) << "REQ: shall handle queue_";
}
TEST_F(MtInQueueTest, handle_emptyQ)
{
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ()) << "REQ: can handle empty Q";
    mt_getQ().handleAllEle();
}

}  // namespace
