/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ObjAnywhere.hpp"

using namespace std;

namespace rlib
{
unique_ptr<DataStore<ObjName>> ObjAnywhere::name_obj_S_;

// ***********************************************************************************************
void ObjAnywhere::deinit() noexcept
{
    name_obj_S_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(UniLog& oneLog) noexcept
{
    if (isInit())
        WRN("(ObjAnywhere) !!! Refuse dup init.")
    else
    {
        name_obj_S_ = make_unique<DataStore<ObjName>>();
        HID("(ObjAnywhere) Succeed.");
    }
}
}  // namespace
