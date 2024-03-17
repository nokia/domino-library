/**
 * Copyright 2019 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: auto rm hdlr after calling it
//   * not-repeated hdlr can not be triggered more than once
//   * whenever hdlr on road, rm hdlr still can cancel it
//   . FreeHdlrDomino need not child of all *HdlrDomino, still can rm/invalidate hdlr
//   . no req if allow to add new hdlr during previous on road; current impl will reject new-add
//
// - why: free mem (eg when huge hdlrs)
//   . save manual-del-hdlr code (MOST SWM hdlrs are not repeated hdlr)
//   . safer to reduce wrong call, etc. [MUST-HAVE!]
//   . why can't reset flag? this class is to convient user, shall not complex on-road, rmHdlr etc
//
// - mem safe: yes
// ***********************************************************************************************
#pragma once

#include <vector>

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class FreeHdlrDomino : public aDominoType
{
public:
    explicit FreeHdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    Domino::Event repeatedHdlr(const Domino::EvName&, const bool isRepeated = true);  // set false = simple rm
    bool isRepeatHdlr(const Domino::Event&) const;

protected:
    void triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv) override;
    using aDominoType::effect_;

    void rmEv_(const Domino::Event& aValidEv) override;

    // -------------------------------------------------------------------------------------------
private:
    // - bitmap & dyn expand, [event]=t/f
    // - don't know if repeated hdlrs are much less than non-repeated, so bitmap is simpler than set<Event>
    vector<bool> isRepeatHdlr_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
bool FreeHdlrDomino<aDominoType>::isRepeatHdlr(const Domino::Event& aEv) const
{
    return aEv < isRepeatHdlr_.size()
        ? isRepeatHdlr_[aEv]
        : false;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event FreeHdlrDomino<aDominoType>::repeatedHdlr(const Domino::EvName& aEvName, const bool isRepeated)
{
    auto&& newEv = this->newEvent(aEvName);
    if (newEv >= isRepeatHdlr_.size())
        isRepeatHdlr_.resize(newEv + 1);
    isRepeatHdlr_[newEv] = isRepeated;
    return newEv;
}

// ***********************************************************************************************
template<typename aDominoType>
void FreeHdlrDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    if (aValidEv < isRepeatHdlr_.size())
        isRepeatHdlr_[aValidEv] = false;

    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
void FreeHdlrDomino<aDominoType>::triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv)
{
    aDominoType::triggerHdlr_(aValidHdlr, aValidEv);
    if (isRepeatHdlr(aValidEv))
        return;

    // auto free aValidHdlr
    // - simple solution (FreeHdlrDomino is not crucial but convenient user)
    // - higher priority hdlr can't insert between hdlr & its auto-free since single thread
    HID("(FreeHdlrDom) trigger a new msg.");
    this->msgSelf_->newMsg([this, aValidEv, weakHdlr = WeakMsgCB(aValidHdlr)]()  // weak_ptr to avoid fail rmHdlr
        {
            if (weakHdlr.expired())
                return;  // otherwise crash
            this->rmOneHdlrOK_(aValidEv, weakHdlr.lock());  // is valid "this" since still own valid aValidHdlr
        },
        this->getPriority(aValidEv)
    );
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   ......................................................................
// 2019-09-06  CSZ       1)Create
// 2020-12-30  CSZ       2)rm on-road hdlr
//                       - clean ut
// 2021-04-01  CSZ       - coding req
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-12-04  CSZ       - simple & natural
// ***********************************************************************************************
