/**
 * Copyright 2018 Nokia. All rights reserved.
 */
#include "ObjAnywhere.hpp"

namespace RLib
{
std::shared_ptr<ObjAnywhere::ObjStore> ObjAnywhere::objStore_;

// ***********************************************************************************************
void ObjAnywhere::deinit(CellLog& log)
{
    if (!objStore_) return;

    SL_INF("Succeed. ObjAnywhere giveup nSvc=" << objStore_->size());
    objStore_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(CellLog& log)
{
    if (objStore_) SL_WRN("!!! Refuse dup init.")
    else
    {
        SL_INF("Succeed.");
        objStore_ = std::make_shared<ObjStore>();
    }
}
}  // namespace
