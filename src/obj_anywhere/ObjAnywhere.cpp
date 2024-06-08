/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ObjAnywhere.hpp"

namespace RLib
{
shared_ptr<DataStore> ObjAnywhere::name_obj_S_;

// ***********************************************************************************************
void ObjAnywhere::deinit()
{
    if (isInit())
        name_obj_S_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(UniLog& oneLog)
{
    if (isInit())
        WRN("(ObjAnywhere) !!! Refuse dup init.")
    else
    {
        HID("(ObjAnywhere) Succeed.");
        name_obj_S_ = make_shared<DataStore>();
    }
}
}  // namespace
