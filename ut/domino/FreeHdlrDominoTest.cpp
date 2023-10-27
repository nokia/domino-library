/**
 * Copyright 2019 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <memory>
#include <set>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct FreeHdlrDominoTest : public UtInitObjAnywhere
{
    MsgCB h1_ = [this](){ hdlrIDs_.insert(1); };
    MsgCB h2_ = [this](){ hdlrIDs_.insert(2); };
    MsgCB h3_ = [this](){ hdlrIDs_.insert(3); };
    MsgCB h4_ = [this](){ hdlrIDs_.insert(4); };
    MsgCB h5_ = [this](){ hdlrIDs_.insert(5); };
    MsgCB h6_ = [this](){ hdlrIDs_.insert(6); };
    MOCK_METHOD(void, h7, ());

    multiset<int> hdlrIDs_;
    set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(FreeHdlrDominoTest);

// ***********************************************************************************************
template<class aParaDom> using FreeMultiHdlrDominoTest = FreeHdlrDominoTest<aParaDom>;
TYPED_TEST_SUITE_P(FreeMultiHdlrDominoTest);

#define FLAG
// ***********************************************************************************************
TYPED_TEST_P(FreeHdlrDominoTest, GOLD_setFlag_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(Domino::D_EVENT_FAILED_RET)) << "REQ: invalid event" << endl;

    auto e1 = PARA_DOM->newEvent("e1");  // not exist in flag bitmap
    auto e2 = PARA_DOM->repeatedHdlr("e2");  // explicit set to bitmap
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1)) << "REQ: default flag" << endl;
    EXPECT_TRUE (PARA_DOM->isRepeatHdlr(e2)) << "REQ: get=set" << endl;
}

#define AUTO_FREE
// ***********************************************************************************************
TYPED_TEST_P(FreeHdlrDominoTest, GOLD_afterCallback_autoRmHdlr)
{
    auto e1 = PARA_DOM->setHdlr("e1", this->h1_);
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1)) << "REQ: default false (most hdlr used once)" << endl;

    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({1}), this->hdlrIDs_);  // cb

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({1}), this->hdlrIDs_) << "REQ: no more cb since auto-rm" << endl;
}
TYPED_TEST_P(FreeHdlrDominoTest, afterCallback_autoRmHdlr_aliasMultiHdlr)
{
    auto aliasE1 = PARA_DOM->multiHdlrByAliasEv("alias e1", this->h3_, "e1");
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(aliasE1));

    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({3}), this->hdlrIDs_);  // cb

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({3}), this->hdlrIDs_) << "REQ: no more cb since auto-rm" << endl;
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, afterCallback_autoRmHdlr_multiHdlr)
{
    auto e1 = PARA_DOM->setHdlr("e1", this->h1_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");
    auto aliasE1 = PARA_DOM->multiHdlrByAliasEv("alias e1", this->h3_, "e1");
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1));
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(aliasE1));

    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({1, 2, 3}), this->hdlrIDs_);  // cb

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({1, 2, 3}), this->hdlrIDs_) << "REQ: no more cb since auto-rm" << endl;

    // re-add hdlr
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->h6_, "e1");
    PARA_DOM->multiHdlrOnSameEv("e1", this->h5_, "h2_");
    PARA_DOM->setHdlr("e1", this->h4_);  // reverse order to inc coverage
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({1, 2, 3, 4, 5, 6}), this->hdlrIDs_) << "REQ: re-add ok" << endl;

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({1, 2, 3, 4, 5, 6}), this->hdlrIDs_) << "REQ: no more cb since auto-rm" << endl;
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, afterCallback_notRmHdlr)
{
    auto e1 = PARA_DOM->setHdlr("e1", this->h1_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");
    auto aliasE1 = PARA_DOM->multiHdlrByAliasEv("alias e1", this->h3_, "e1");
    PARA_DOM->repeatedHdlr("e1");  // req: not auto rm; d1 & d2 together
    PARA_DOM->repeatedHdlr("alias e1");  // req: not auto rm; d3 separately
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(e1));
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(aliasE1));

    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());  // 1st cb
    EXPECT_EQ(multiset<int>({1, 2, 3}), this->hdlrIDs_) << "REQ: not auto-rm" << endl;

    PARA_DOM->setState({{"e1", false}, {"alias e1", false}});  // SameEv simpler than AliasEv
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());  // 2nd cb
    EXPECT_EQ(multiset<int>({1, 2, 3, 1, 2, 3}), this->hdlrIDs_) << "REQ: not auto-rm" << endl;

    // re-add hdlr
    PARA_DOM->setHdlr("e1", this->h4_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h5_, "h2_");
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->h6_, "e1");

    PARA_DOM->setState({{"e1", false}, {"alias e1", false}});
    PARA_DOM->setState({{"e1", true}});
    MSG_SELF->handleAllMsg(MSG_SELF->getValid()) ;
    EXPECT_EQ(multiset<int>({1, 2, 3, 1, 2, 3, 1, 2, 3}), this->hdlrIDs_) << "REQ: re-add nok" << endl;
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, BugFix_disorderAutoRm_ok)
{
    PARA_DOM->multiHdlrOnSameEv("e1", this->h3_, "h3_");  // req: will rm before h1_ & h2_/h4_
    PARA_DOM->setState({{"e1", true}});                   // h3_ on road
    PARA_DOM->setHdlr("e1", this->h1_);                   // h1_ on road
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");  // h2_ on road
    PARA_DOM->multiHdlrOnSameEv("e1", this->h4_, "h4_");  // h4_ on road

    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({1, 2, 3, 4}), this->hdlrIDs_);
}
TYPED_TEST_P(FreeHdlrDominoTest, multiCallbackOnRoad_noCrash_noMultiCall)
{
    PARA_DOM->setHdlr("e1", this->h1_);
    PARA_DOM->setState({{"e1", true}});                   // 1st on road

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});                   // 2nd on road

    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({1}), this->hdlrIDs_);        // req: no more cb since auto-rm
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, BugFix_multiCallbackOnRoad_noCrash_noMultiCall)
{
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");
    PARA_DOM->setState({{"e1", true}});                   // 1st h2_ on road

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});                   // 2nd h2_ on road

    PARA_DOM->setHdlr("e1", this->h1_);                   // h1_ on road

    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>({1, 2}), this->hdlrIDs_);     // req: no more cb since auto-rm
}

#define MEM_LEAK
// ***********************************************************************************************
TYPED_TEST_P(FreeHdlrDominoTest, BugFix_noMemLeak_whenRmMsgSelf)  // checked by CI valgrind
{
    EXPECT_CALL(*this, h7()).Times(0);
    PARA_DOM->setHdlr("e1", [&](){ this->h7(); });
    PARA_DOM->setState({{"e1", true}});
    ASSERT_TRUE(MSG_SELF->nMsg());
    MSG_SELF.reset();  // req: no mem leak when rm MsgSelf with h7 in msg queue
}
TYPED_TEST_P(FreeHdlrDominoTest, BugFix_noCrash_whenRmDom)
{
    EXPECT_CALL(*this, h7()).Times(0);
    PARA_DOM->setHdlr("e1", [&](){ this->h7(); });
    PARA_DOM->setState({{"e1", true}});
    ASSERT_TRUE(MSG_SELF->nMsg());
    ObjAnywhere::set<TypeParam>(nullptr, *this);  // req: no mem leak when rm MsgSelf with h7 in msg queue
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(FreeHdlrDominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->repeatedHdlr("e1");
    PARA_DOM->repeatedHdlr("e1");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(2u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(FreeHdlrDominoTest
    , GOLD_setFlag_thenGetIt

    , GOLD_afterCallback_autoRmHdlr
    , afterCallback_autoRmHdlr_aliasMultiHdlr
    , multiCallbackOnRoad_noCrash_noMultiCall
    , BugFix_noMemLeak_whenRmMsgSelf
    , BugFix_noCrash_whenRmDom

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyFreeDom = Types<MinFreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, FreeHdlrDominoTest, AnyFreeDom);

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(FreeMultiHdlrDominoTest
    , afterCallback_autoRmHdlr_multiHdlr
    , afterCallback_notRmHdlr
    , BugFix_disorderAutoRm_ok
    , BugFix_multiCallbackOnRoad_noCrash_noMultiCall
);
using AnyFreeMultiDom = Types<MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, FreeMultiHdlrDominoTest, AnyFreeMultiDom);
}  // namespace
