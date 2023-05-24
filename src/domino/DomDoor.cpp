/**
 * Copyright 2023 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "DomDoor.hpp"

namespace RLib
{
// ***********************************************************************************************
bool DomDoor::setSubTreeOK(const Domino::EvName& aSubRoot, shared_ptr<void> aDom)
{
    if (aDom == nullptr)
    {
        domStore_.erase(aSubRoot);
        return true;
    }
    else
    {
        auto candidateDom = subTree<Domino>(aSubRoot);
        if (candidateDom && candidateDom->getEventBy(aSubRoot) != Domino::D_EVENT_FAILED_RET)
        {
            ERR("!!! Failed since subRoot=[" << aSubRoot << "] is alreay in use.")
            return false;
        }

        domStore_[aSubRoot] = aDom;
        return true;
    }
}
}  // namespace
