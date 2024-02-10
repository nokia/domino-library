/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ObjAnywhere.hpp"

namespace RLib
{
shared_ptr<ObjAnywhere::ObjStore> ObjAnywhere::objStore_;

// ***********************************************************************************************
void ObjAnywhere::deinit(UniLog& oneLog)
{
    if (!objStore_)
        return;

    INF("(ObjAnywhere) Succeed. ObjAnywhere giveup nSvc=" << objStore_->size());
    objStore_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(UniLog& oneLog)
{
    if (objStore_)
        WRN("(ObjAnywhere) !!! Refuse dup init.")
    else
    {
        HID("(ObjAnywhere) Succeed.");
        objStore_ = make_shared<ObjStore>();
    }
}
}  // namespace
