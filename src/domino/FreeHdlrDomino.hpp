/**
 * Copyright 2019 Nokia. All rights reserved.
 */
// ***********************************************************************************************
// - what: auto rm hdlr after calling it
//   * not-repeated hdlr can not be triggered more than once
//   * whenever hdlr on road, rm hdlr still can cancel it
//   . FreeHdlrDomino need not child of all *HdlrDomino, still can rm/invalidate hdlr
//   . no req if allow to add new hdlr during previous on road; current impl will reject new-add
// - why: free mem (eg when huge hdlrs)
//   . save manual-del-hdlr code (MOST SWM hdlrs are not repeated hdlr)
//   . safer to reduce wrong call, etc. [MUST-HAVE!]
//   . why can't reset flag? this class is to convient user, shall not complex on-road, rmHdlr etc
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
    Domino::Event flagRepeatedHdlr(const Domino::EvName&);
    bool isRepeatHdlr(const Domino::Event) const;

protected:
    void triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event aEv) override;
    using aDominoType::effect;
private:
    std::vector<bool> isRepeatHdlr_;  // bitmap & dyn expand, [event]=t/f
public:
    using aDominoType::log_;
};

// ***********************************************************************************************
template<class aDominoType>
Domino::Event FreeHdlrDomino<aDominoType>::flagRepeatedHdlr(const Domino::EvName& aEvName)
{
    auto&& event = this->newEvent(aEvName);
    if (event >= isRepeatHdlr_.size()) isRepeatHdlr_.resize(event + 1);
    isRepeatHdlr_[event] = true;
    return event;
}

// ***********************************************************************************************
template<class aDominoType>
bool FreeHdlrDomino<aDominoType>::isRepeatHdlr(const Domino::Event aEv) const
{
    return aEv < isRepeatHdlr_.size() ? isRepeatHdlr_.at(aEv) : false;
}

// ***********************************************************************************************
template<class aDominoType>
void FreeHdlrDomino<aDominoType>::triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event aEv)
{
    if (isRepeatHdlr(aEv)) aDominoType::triggerHdlr(aHdlr, aEv);
    else
    {
        WeakMsgCB weakHdlr = aHdlr;
        auto&& superHdlr = std::make_shared<MsgCB>();
        *superHdlr = [weakHdlr, this, aEv, superHdlr]() mutable
        {
            // req: call hdlr
            auto&& hdlr = weakHdlr.lock();
            if (hdlr && *hdlr)
                (*hdlr)();

            // req: rm hdlr
            this->pureRmHdlrOK(aEv, hdlr);  // only *HdlrDomino owns shared hdlr, prove "this" available

            // req: rm superHdlr
            // superHdlr.reset();
        };
        aDominoType::triggerHdlr(superHdlr, aEv);
    }
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
// ***********************************************************************************************
