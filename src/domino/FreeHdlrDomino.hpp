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
#ifndef FREE_HDLR_DOMINO_HPP_
#define FREE_HDLR_DOMINO_HPP_

#include <vector>

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class FreeHdlrDomino : public aDominoType
{
public:
    explicit FreeHdlrDomino(const UniLogName& aUniLogName) : aDominoType(aUniLogName) {}

    Domino::Event repeatedHdlr(const Domino::EvName&, const bool isRepeated = true);  // set false = simple rm
    bool isRepeatHdlr(const Domino::Event) const;

protected:
    void triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event) override;
    using aDominoType::effect;

    void innerRmEv(const Domino::Event) override;

    // -------------------------------------------------------------------------------------------
private:
    // - bitmap & dyn expand, [event]=t/f
    // - don't know if repeated hdlrs are much less than non-repeated, so bitmap is simpler than set<Event>
    vector<bool> isRepeatHdlr_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
void FreeHdlrDomino<aDominoType>::innerRmEv(const Domino::Event aEv)
{
    if (aEv < isRepeatHdlr_.size())
        isRepeatHdlr_[aEv] = false;

    aDominoType::innerRmEv(aEv);
}

// ***********************************************************************************************
template<class aDominoType>
bool FreeHdlrDomino<aDominoType>::isRepeatHdlr(const Domino::Event aEv) const
{
    return aEv < isRepeatHdlr_.size()
        ? isRepeatHdlr_[aEv]
        : false;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event FreeHdlrDomino<aDominoType>::repeatedHdlr(const Domino::EvName& aEvName, const bool isRepeated)
{
    auto&& event = this->newEvent(aEvName);
    if (event >= isRepeatHdlr_.size())
        isRepeatHdlr_.resize(event + 1);
    isRepeatHdlr_[event] = isRepeated;
    return event;
}

// ***********************************************************************************************
template<class aDominoType>
void FreeHdlrDomino<aDominoType>::triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event aEv)
{
    aDominoType::triggerHdlr(aHdlr, aEv);
    if (isRepeatHdlr(aEv))
        return;

    // auto free aHdlr
    // - simple solution (FreeHdlrDomino is not crucial but convenient user)
    // - gap: higher priority hdlr may insert between hdlr & its auto-free
    HID("(FreeHdlrDom) trigger a new msg.");
    this->msgSelf_->newMsg([this, aEv, weakHdlr = WeakMsgCB(aHdlr)]()  // weak_ptr to avoid fail rmHdlr
        {
            if (weakHdlr.expired())
                return;
            this->rmOneHdlrOK(aEv, weakHdlr.lock());  // is valid "this" since still own aHdlr
        },
        this->getPriority(aEv)
    );
}

}  // namespace
#endif  // FREE_HDLR_DOMINO_HPP_
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
