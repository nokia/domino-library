/**
 * Copyright 2020 Nokia. All rights reserved.
 */
// ***********************************************************************************************
// - what:
//   . mv hdlr out Domino
//   . extend Domino to basic hdlr: 1 hdlr, MsgSelf
//   . can extend to PriDomino by overriding getPriority()
//   . can extend to FreeHdlrDomino+MultiHdlrDomino by overriding invalidAllHdlrOK()
// - why:
//   . pure Domino w/o hdlr (SOLID#1: single-responbility)
//   * basic hdlr for common usage
//   * hdlr extendable
// - core: hdlrs_
// - why invalidate hdlr: fix cb-invalid for all domino hdlr
//   . whenever succ, no cb, even cb already on road
// ***********************************************************************************************
#ifndef HDLR_DOMINO_HPP_
#define HDLR_DOMINO_HPP_

#include <functional>
#include <memory>  // for shared_ptr
#include <unordered_map>

#include "MsgSelf.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class HdlrDomino : public aDominoType
{
public:
    HdlrDomino() { msgSelf_ = MSG_SELF; }  // default
    void setMsgSelf(std::shared_ptr<MsgSelf>& aMsgSelf) { msgSelf_ = aMsgSelf; }  // can replace default

    Domino::Event setHdlr(const Domino::EvName&, const MsgCB&);
    bool rmOneHdlrOK(const Domino::EvName& aEvName);

    // -------------------------------------------------------------------------------------------
    // - add a new ev=aAliasEN to store aHdlr (aAliasEN's true prev is aHostEN)
    // . pros: can FreeHdlrDomino::flagRepeatedHdlr() for each hdlr
    // . cons: the state of aHostEN & aAliasEN may not sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrByAliasEv(const Domino::EvName& aAliasEN, const MsgCB& aHdlr,
        const Domino::EvName& aHostEN);

    virtual EMsgPriority getPriority(const Domino::Event) const { return EMsgPri_NORM; }

protected:
    void effect(const Domino::Event) override;
    virtual void triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event aEv)
    {
        msgSelf_->newMsg(aHdlr, getPriority(aEv));
    }
    virtual bool pureRmHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr = SharedMsgCB());

    size_t nHdlrRef(const Domino::Event) const;

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, SharedMsgCB> hdlrs_;
    std::shared_ptr<MsgSelf> msgSelf_;
public:
    using aDominoType::log_;
};

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::effect(const Domino::Event aEv)
{
    auto&& it = hdlrs_.find(aEv);
    if (it == hdlrs_.end()) return;

    DBG("(HdlrDomino) Succeed to trigger 1 hdlr of EvName=" << this->evName(aEv));
    triggerHdlr(it->second, aEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::multiHdlrByAliasEv(const Domino::EvName& aAliasEN,
    const MsgCB& aHdlr, const Domino::EvName& aHostEN)
{
    auto&& event = this->setHdlr(aAliasEN, aHdlr);
    if (event == Domino::D_EVENT_FAILED_RET) return Domino::D_EVENT_FAILED_RET;

    return this->setPrev(aAliasEN, {{aHostEN, true}});
}

// ***********************************************************************************************
template<class aDominoType>
size_t HdlrDomino<aDominoType>::nHdlrRef(const Domino::Event aEvent) const
{
    auto&& it = hdlrs_.find(aEvent);
    return it == hdlrs_.end() ? Domino::D_EVENT_FAILED_RET : it->second.use_count();
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::pureRmHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr)
{
    // req: "ret true" means real rm
    auto&& itHdlr = hdlrs_.find(aEv);
    if (itHdlr == hdlrs_.end())
        return false;

    // req: must match
    if (aHdlr && itHdlr->second != aHdlr)
        return false;

    HID("(HdlrDomino) Succeed to remove hdlr of EvName=" << this->evName(aEv)
        << ", nHdlrRef=" << itHdlr->second.use_count());
    hdlrs_.erase(itHdlr);
    return true;
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName)
{
    return pureRmHdlrOK(this->getEventBy(aEvName));
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::setHdlr(const Domino::EvName& aEvName, const MsgCB& aHdlr)
{
    auto&& event = this->newEvent(aEvName);
    if (hdlrs_.find(event) != hdlrs_.end())
    {
        WRN("(HdlrDomino) Failed!!! Not support overwrite hdlr for " << aEvName << ". Use MultiHdlrDomino instead.");
        return Domino::D_EVENT_FAILED_RET;
    }
    auto&& hdlr = std::make_shared<MsgCB>(aHdlr);
    hdlrs_[event] = hdlr;
    HID("(HdlrDomino) Succeed for EvName=" << aEvName);

    if (this->state(event) == true)
    {
        DBG("(HdlrDomino) Trigger the new hdlr of EvName=" << aEvName);
        triggerHdlr(hdlr, event);
    }
    return event;
}
}  // namespace
#endif  // HDLR_DOMINO_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   ......................................................................
// 2020-12-11  CSZ       1)create
// 2020-12-28  CSZ       2)fix invalid cb by weak_ptr<cb>
// 2021-04-01  CSZ       - coding req
// 2022-01-04  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// ***********************************************************************************************
