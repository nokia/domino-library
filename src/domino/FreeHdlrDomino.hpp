/**
 * Copyright 2019 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: auto rm hdlr after calling it
// - why:
//   . free mem (eg when huge hdlrs)
//   . save manual-del-hdlr code (MOST SWM hdlrs are not repeated hdlr)
//   * safer to reduce wrong call, etc. [MUST-HAVE!]
//
// - REQ:
//   * facilitate user so must be safe-simple (avoid complex/dangeous, manual can handle them)
//   . whenever hdlr on road, rm hdlr still can cancel it
//   * no gap between call-hdlr & rm-hdlr - safe as user's expectation
//
// - class safe: yes
//   . forbid change flag when hdlr available, avoid complex scenario eg hdlr on-road
// ***********************************************************************************************
#pragma once

#include <vector>

namespace rlib
{
// ***********************************************************************************************
template<class aDominoType>
class FreeHdlrDomino : public aDominoType
{
public:
    explicit FreeHdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) noexcept : aDominoType(aUniLogName) {}

    Domino::Event repeatedHdlr(const Domino::EvName&, const bool isRepeated = true) noexcept;  // set false = simple rm
    bool isRepeatHdlr(const Domino::Event&) const noexcept;

protected:
    void triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv) noexcept override;

    void rmEv_(const Domino::Event& aValidEv) noexcept override;

    // -------------------------------------------------------------------------------------------
private:
    // - bitmap & dyn expand, [event]=t/f
    // - don't know if repeated hdlrs are much less than non-repeated, so bitmap is simpler than set<Event>
    std::vector<bool> isRepeatHdlr_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
bool FreeHdlrDomino<aDominoType>::isRepeatHdlr(const Domino::Event& aEv) const noexcept
{
    return aEv < isRepeatHdlr_.size()
        ? isRepeatHdlr_[aEv]
        : false;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event FreeHdlrDomino<aDominoType>::repeatedHdlr(const Domino::EvName& aEvName, const bool isRepeated) noexcept
{
    // validate
    if (this->nHdlr(aEvName) > 0)
    {
        ERR("(PriDom) FAILED since exist hdlr(s) in en=" << aEvName << ", avoid complex/mislead result");
        return Domino::D_EVENT_FAILED_RET;
    }

    // set flag
    auto&& newEv = this->newEvent(aEvName);
    if (newEv >= isRepeatHdlr_.size())
        isRepeatHdlr_.resize(newEv + 1);
    isRepeatHdlr_[newEv] = isRepeated;
    return newEv;
}

// ***********************************************************************************************
template<typename aDominoType>
void FreeHdlrDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv) noexcept
{
    if (aValidEv < isRepeatHdlr_.size())
        isRepeatHdlr_[aValidEv] = false;

    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
void FreeHdlrDomino<aDominoType>::triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv) noexcept
{
    // repeated hdlr
    if (isRepeatHdlr(aValidEv))
    {
        aDominoType::triggerHdlr_(aValidHdlr, aValidEv);
        return;
    }

    HID("(FreeHdlrDom) trigger a call-then-rm msg for en=" << this->evName_(aValidEv));
    this->msgSelf_->newMsgOK(
        [this, aValidEv, weakHdlr = WeakMsgCB(aValidHdlr)]() noexcept {
            if (weakHdlr.expired())  // validate
                return;  // otherwise crash
            auto hdlr = weakHdlr.lock();  // get
            this->rmOneHdlrOK_(aValidEv, hdlr);  // safer to rm first to avoid hdlr does sth strange
            try { (*(hdlr.get()))(); }  // call; setHdlr() forbid cb==null
            catch(...) { ERR("(FreeHdlrDom) except when exe callback!!! ev=" << aValidEv); }
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
// 2025-02-13  CSZ       - support both SafePtr & shared_ptr
// 2025-04-05  CSZ       3)tolerate exception
// ***********************************************************************************************
