/**
 * Copyright 2018 Nokia. All rights reserved.
 */
#include "ObjAnywhere.hpp"

namespace RLib
{
std::shared_ptr<ObjAnywhere::ObjStore> ObjAnywhere::objStore_;

// ***********************************************************************************************
void ObjAnywhere::deinit(CellLog& oneLog)
{
    if (!objStore_) return;

    INF("Succeed. ObjAnywhere giveup nSvc=" << objStore_->size());
    objStore_.reset();
}

// ***********************************************************************************************
void ObjAnywhere::init(CellLog& oneLog)
{
    if (objStore_) WRN("!!! Refuse dup init.")
    else
    {
        INF("Succeed.");
        objStore_ = std::make_shared<ObjStore>();
    }
}
}  // namespace
