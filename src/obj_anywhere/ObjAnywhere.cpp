/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ObjAnywhere.hpp"

namespace RLib
{
shared_ptr<ObjAnywhere::ObjStore> ObjAnywhere::idx_obj_S_;

// ***********************************************************************************************
void ObjAnywhere::deinit(UniLog& oneLog)
{
    if (!idx_obj_S_)
        return;

    INF("(ObjAnywhere) Succeed. ObjAnywhere giveup nSvc=" << idx_obj_S_->size());
    idx_obj_S_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(UniLog& oneLog)
{
    if (idx_obj_S_)
        WRN("(ObjAnywhere) !!! Refuse dup init.")
    else
    {
        HID("(ObjAnywhere) Succeed.");
        idx_obj_S_ = make_shared<ObjStore>();
    }
}
}  // namespace
