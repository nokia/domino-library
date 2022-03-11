/**
 * Copyright 2019 Nokia. All rights reserved.
 */
#include <gtest/gtest.h>
#include <memory>
#include <set>

#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct FreeHdlrDominoTest : public Test
{
    FreeHdlrDominoTest() { ObjAnywhere::get<aParaDom>()->setMsgSelf(msgSelf_); }

    // -------------------------------------------------------------------------------------------
    UtInitObjAnywhere utInit_;
    std::shared_ptr<MsgSelf> msgSelf_ = std::make_shared<MsgSelf>([this](LoopBackFUNC aFunc){ loopbackFunc_ = aFunc; });
    LoopBackFUNC loopbackFunc_;

    MsgCB h1_ = [this](){ hdlrIDs_.insert(1); };
    MsgCB h2_ = [this](){ hdlrIDs_.insert(2); };
    MsgCB h3_ = [this](){ hdlrIDs_.insert(3); };
    MsgCB h4_ = [this](){ hdlrIDs_.insert(4); };
    MsgCB h5_ = [this](){ hdlrIDs_.insert(5); };
    MsgCB h6_ = [this](){ hdlrIDs_.insert(6); };

    std::multiset<int> hdlrIDs_;
    std::set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(FreeHdlrDominoTest);

// ***********************************************************************************************
template<class aParaDom> using FreeMultiHdlrDominoTest = FreeHdlrDominoTest<aParaDom>;
TYPED_TEST_SUITE_P(FreeMultiHdlrDominoTest);

#define AUTO_FREE
// ***********************************************************************************************
TYPED_TEST_P(FreeMultiHdlrDominoTest, GOLD_afterCallback_autoRmHdlr)
{
    auto e1 = PARA_DOM->setHdlr("e1", this->h1_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");
    auto aliasE1 = PARA_DOM->multiHdlrByAliasEv("alias e1", this->h3_, "e1");
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1));
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(aliasE1));

    PARA_DOM->setState({{"e1", true}});
    EXPECT_EQ(0u, this->hdlrIDs_.size());                               // on road
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();
    EXPECT_EQ(std::multiset<int>({1, 2, 3}), this->hdlrIDs_);           // cb

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_() ;
    EXPECT_EQ(std::multiset<int>({1, 2, 3}), this->hdlrIDs_);           // req: no more cb since auto-rm

    // re-add hdlr
    PARA_DOM->setHdlr("e1", this->h4_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h5_, "h2_");
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->h6_, "e1");

    PARA_DOM->setState({{"e1", false}});
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_() ;
    EXPECT_EQ(std::multiset<int>({1, 2, 3, 4, 5, 6}), this->hdlrIDs_);  // req: re-add ok
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, GOLD_afterCallback_notRmHdlr)
{
    auto e1 = PARA_DOM->setHdlr("e1", this->h1_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");
    auto aliasE1 = PARA_DOM->multiHdlrByAliasEv("alias e1", this->h3_, "e1");
    PARA_DOM->flagRepeatedHdlr("e1");                                   // req: not auto rm; d1 & d2 together
    PARA_DOM->flagRepeatedHdlr("alias e1");                             // req: not auto rm; d3 separately
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(e1));
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(aliasE1));

    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();                // 1st cb
    EXPECT_EQ(std::multiset<int>({1, 2, 3}), this->hdlrIDs_);           // req: not auto-rm

    PARA_DOM->setState({{"e1", false}, {"alias e1", false}});           // SameEv simpler than AliasEv
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();                // 2nd cb
    EXPECT_EQ(std::multiset<int>({1, 2, 3, 1, 2, 3}), this->hdlrIDs_);  // req: not auto-rm

    // re-add hdlr
    PARA_DOM->setHdlr("e1", this->h4_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h5_, "h2_");
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->h6_, "e1");

    PARA_DOM->setState({{"e1", false}, {"alias e1", false}});
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_() ;
    EXPECT_EQ(std::multiset<int>({1, 2, 3, 1, 2, 3, 1, 2, 3}), this->hdlrIDs_);  // req: re-add nok
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, BugFix_disorderAutoRm_ok)
{
    PARA_DOM->multiHdlrOnSameEv("e1", this->h3_, "h3_");  // req: will rm before h1_ & h2_/h4_
    PARA_DOM->setState({{"e1", true}});                   // h3_ on road
    PARA_DOM->setHdlr("e1", this->h1_);                   // h1_ on road
    PARA_DOM->multiHdlrOnSameEv("e1", this->h2_, "h2_");  // h2_ on road
    PARA_DOM->multiHdlrOnSameEv("e1", this->h4_, "h4_");  // h4_ on road

    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();
    EXPECT_EQ(std::multiset<int>({1, 2, 3, 4}), this->hdlrIDs_);
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, BugFix_invalidHdlr_noCrash)
{
    PARA_DOM->setHdlr("e1", nullptr);
    PARA_DOM->multiHdlrOnSameEv("e1", nullptr, "h2_");
    PARA_DOM->multiHdlrByAliasEv("alias e1", nullptr, "e1");
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();

    // re-add hdlr
    PARA_DOM->setHdlr("e1", this->h4_);
    PARA_DOM->multiHdlrOnSameEv("e1", this->h5_, "h2_");
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->h6_, "e1");

    PARA_DOM->setState({{"e1", false}, {"alias e1", false}});
    PARA_DOM->setState({{"e1", true}});
    if (this->msgSelf_->hasMsg()) this->loopbackFunc_() ;
    EXPECT_EQ(std::multiset<int>({4, 5, 6}), this->hdlrIDs_);  // req: re-add ok
}
TYPED_TEST_P(FreeMultiHdlrDominoTest, invalidEv_isRepeatFalse)
{
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(0));  // ev=0 is invalid ID
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(FreeHdlrDominoTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->flagRepeatedHdlr("e1");
    PARA_DOM->flagRepeatedHdlr("e1");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(2u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(FreeHdlrDominoTest
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyFreeDom = Types<MinFreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, FreeHdlrDominoTest, AnyFreeDom);

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(FreeMultiHdlrDominoTest
    , GOLD_afterCallback_autoRmHdlr
    , GOLD_afterCallback_notRmHdlr
    , BugFix_disorderAutoRm_ok
    , BugFix_invalidHdlr_noCrash
    , invalidEv_isRepeatFalse
);
using AnyFreeMultiDom = Types<MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, FreeMultiHdlrDominoTest, AnyFreeMultiDom);
}  // namespace
