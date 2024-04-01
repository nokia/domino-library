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

#define RLIB_UT
#include "MT_PingMainTH.hpp"
#include "MtInQueue.hpp"
#undef RLIB_UT

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
        mt_getQ().mt_clear();  // avoid other case interference
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
        auto msg = mt_getQ().pop<int>();  // CAN'T directly get() that destruct shared_ptr afterward
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
    INF("REQ(sleep 1us/push): e2e user=0.354s->0.123s, sys=0.412s->0.159s")
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
    INF("before loop")
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
    INF("REQ: loop cost 2576us now, previously no cache & lock cost 4422us")
}

#define PUSH_POP
// ***********************************************************************************************
TEST_F(MtInQueueTest, pushWakeup_popNoBlockAndWakeup)
{
    mt_getQ().mt_push(MAKE_PTR<string>("1st"));
    mt_getQ().mt_push(MAKE_PTR<string>("2nd"));
    mt_getQ().backdoor().lock();
    timedwait(600);  // REQ: not blocked 10min but waked by prev push

    ASSERT_EQ(nullptr, mt_getQ().pop<void>().get());
    timedwait(600);  // REQ: not blocked but waked by pop() that can't access queue_
    mt_getQ().backdoor().unlock();
    ASSERT_EQ("1st", *(mt_getQ().pop<string>().get())) << "REQ: timedwait() not blocked but waked by failed pop()";

    mt_getQ().backdoor().lock();
    ASSERT_EQ("2nd", *(mt_getQ().pop<string>().get())) << "REQ: no-blocked pop from cache";
    mt_getQ().backdoor().unlock();
}
TEST_F(MtInQueueTest, push_null_NOK)
{
    mt_getQ().mt_push<void>(nullptr);
    EXPECT_EQ(0, mt_getQ().mt_sizeQ(true)) << "REQ: can't push nullptr since pop empty will ret nullptr";
}
TEST_F(MtInQueueTest, push_takeover_toEnsureMtSafe)
{
    auto ele = MAKE_PTR<int>(1);
    auto e2  = ele;
    mt_getQ().mt_push<int>(move(ele));
    EXPECT_EQ(0, mt_getQ().mt_sizeQ(true)) << "REQ: push failed since can't takeover ele";

    e2 = nullptr;
    mt_getQ().mt_push<int>(move(ele));
    EXPECT_EQ(1, mt_getQ().mt_sizeQ(true)) << "REQ: push succ since takeover";
    EXPECT_EQ(0, ele.use_count())      << "REQ: own nothing after push";
}
TEST_F(MtInQueueTest, popEmpty_popMismatchType)
{
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "empty MtQ";
    EXPECT_EQ(nullptr, mt_getQ().pop<void>().get()) << "REQ: can pop empty";

    mt_getQ().mt_push<int>(MAKE_PTR<int>(10));
    EXPECT_EQ(nullptr, mt_getQ().pop<void>().get()) << "REQ: pop failed since type mismatch";
    EXPECT_EQ(1u, mt_getQ().mt_sizeQ(true)) << "REQ: pop fail = no pop - natural";
}

#define SIZE_Q
// ***********************************************************************************************
TEST_F(MtInQueueTest, sizeQ_block_nonBlock)
{
    ASSERT_EQ(0u, mt_getQ().mt_sizeQ(true )) << "REQ: blocked init";
    ASSERT_EQ(0u, mt_getQ().mt_sizeQ(false)) << "REQ: unblocked init";

    mt_getQ().mt_push<int>(MAKE_PTR<int>(1));
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(true )) << "REQ: inc blocked size";
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(false)) << "REQ: inc unblocked size";

    mt_getQ().backdoor().lock();
    ASSERT_EQ(0u, mt_getQ().mt_sizeQ(false)) << "REQ: sizeQ can be unblocked (blocked will hang)";
    mt_getQ().backdoor().unlock();

    mt_getQ().mt_push<int>(MAKE_PTR<int>(2));
    EXPECT_EQ(1, *mt_getQ().pop<int>().get());
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(true )) << "REQ: dec blocked size";
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(false)) << "REQ: dec unblocked size";

    mt_getQ().mt_push<int>(MAKE_PTR<int>(3));
    ASSERT_EQ(2u, mt_getQ().mt_sizeQ(true )) << "REQ: re-inc blocked size (now 1 in cache_, 1 in queue)";
    ASSERT_EQ(2u, mt_getQ().mt_sizeQ(false)) << "REQ: re-inc unblocked size";

    mt_getQ().backdoor().lock();
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(false)) << "REQ: sizeQ can be unblocked (get cache_ only)";
    mt_getQ().backdoor().unlock();
}

#define DESTRUCT
// ***********************************************************************************************
struct TestObj
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST_F(MtInQueueTest, destruct_right_type)
{
    bool isDestructed;
    mt_getQ().mt_push(MAKE_PTR<TestObj>(isDestructed));
    ASSERT_FALSE(isDestructed);

    mt_getQ().mt_clear();
    ASSERT_TRUE(isDestructed) << "REQ: destruct correctly";
}
TEST_F(MtInQueueTest, clear_queue_cache_hdlr)
{
    mt_getQ().mt_push<int>(make_safe<int>(1));
    mt_getQ().mt_push<int>(make_safe<int>(2));
    mt_getQ().pop();
    mt_getQ().mt_push<int>(make_safe<int>(3));

    mt_getQ().backdoor().lock();
    ASSERT_EQ(1u, mt_getQ().mt_sizeQ(false)) << "1 in cache_";
    mt_getQ().backdoor().unlock();
    EXPECT_EQ(2u, mt_getQ().mt_sizeQ(true)) << "1 in queue_";

    mt_getQ().setHdlrOK<int>([](UniPtr){});
    EXPECT_EQ(1u, mt_getQ().nHdlr()) << "1 hdlr";

    mt_getQ().mt_clear();
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "REQ: clear all ele";
    EXPECT_EQ(0u, mt_getQ().nHdlr()) << "REQ: clear all hdlr";
}
TEST_F(MtInQueueTest, cov_destructor)
{
    MtInQueue mtQ;  // cov destructor with ele left
    mtQ.mt_push<int>(MAKE_PTR<int>(1));

    MtInQueue mtQ2;  // cov destructor without ele left
}

#define HDLR
// ***********************************************************************************************
// normal covered by MT_SemaphoreTest
TEST_F(MtInQueueTest, GOLD_handle_bothCacheAndQueue_ifPossible_withoutBlocked)
{
    // init
    size_t nCalled = 0;
    mt_getQ().setHdlrOK<int>([&nCalled](UniPtr){ ++nCalled; });
    mt_getQ().mt_push<int>(make_safe<int>(1));
    mt_getQ().mt_push<int>(make_safe<int>(1));
    mt_getQ().pop();  // still 1 ele in cache_
    mt_getQ().mt_push<int>(make_safe<int>(1));  // and 1 ele in queue_
    EXPECT_EQ(2u, mt_getQ().mt_sizeQ(true));

    mt_getQ().backdoor().lock();
    EXPECT_EQ(1u, mt_getQ().handleAllEle()) << "REQ: handle cache_, avoid blocked on queue_";
    EXPECT_EQ(1u, nCalled);

    mt_getQ().backdoor().unlock();
    mt_getQ().handleAllEle();
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "REQ: shall handle queue_ since unlocked";
    EXPECT_EQ(2u, nCalled);
}
TEST_F(MtInQueueTest, discard_noHdlrEle)
{
    mt_getQ().mt_push<int>(make_safe<int>(1));
    EXPECT_EQ(1u, mt_getQ().mt_sizeQ(true));

    mt_getQ().handleAllEle();
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "REQ: discard ele w/o hdlr - simple & no mem leak";
}
TEST_F(MtInQueueTest, handle_emptyQ)
{
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "REQ: can handle empty Q";
    mt_getQ().handleAllEle();
}
TEST_F(MtInQueueTest, hdlr_cannot_null_overwrite)
{
    EXPECT_FALSE(mt_getQ().setHdlrOK<void>(nullptr));
    EXPECT_EQ(0, mt_getQ().nHdlr()) << "REQ: hdlr=null doesn't make sense";

    EXPECT_TRUE(mt_getQ().setHdlrOK<void>([](UniPtr){}));
    EXPECT_EQ(1, mt_getQ().nHdlr()) << "set new hdlr";
    EXPECT_FALSE(mt_getQ().setHdlrOK<void>([](UniPtr){})) << "REQ: can NOT overwrite hdlr";
}
TEST_F(MtInQueueTest, handle_via_base)
{
    struct Base { virtual int value() { return 1; } };
    struct Derive : public Base { int value() override { return 2; } };
    mt_getQ().mt_push<Base>(MAKE_PTR<Base>());
    EXPECT_EQ(nullptr, mt_getQ().pop<Derive>().get()) << "REQ: pop derive type is invalid";

    auto d = MAKE_PTR<Derive>();
    mt_getQ().mt_push<Base>(move(d));
    EXPECT_EQ(2u, mt_getQ().mt_sizeQ(true)) << "REQ: can store Derive obj as Base";

    EXPECT_TRUE(mt_getQ().setHdlrOK<Base>([](UniPtr aEle)
    {
        static int exp = 1;
        auto ele = static_pointer_cast<Base>(aEle);
        EXPECT_NE(nullptr, ele.get());
        EXPECT_EQ(exp++, ele.get()->value());
    }));
    mt_getQ().handleAllEle();
    EXPECT_EQ(0u, mt_getQ().mt_sizeQ(true)) << "req: all ele handled";
}

}  // namespace
