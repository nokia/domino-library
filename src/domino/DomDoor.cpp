/**
 * Copyright 2023 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "DomDoor.hpp"

namespace RLib
{
// ***********************************************************************************************
void DomDoor::setSubTree(const Domino::EvName& aSubRoot, shared_ptr<void> aDom)
{
    if (aDom == nullptr)
        domStore_.erase(aSubRoot);
    else
        domStore_[aSubRoot] = aDom;
}
}  // namespace
