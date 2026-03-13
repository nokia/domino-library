/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#define IN_GTEST
#include "UniCoutLog.hpp"
#undef IN_GTEST

#define UNI_LOG_TEST UniCoutLogTest
#define UNI_LOG      UniCoutLog
#include "UniLogTest.hpp"

// ***********************************************************************************************
namespace rlib
{
TEST_F(UniCoutLogTest, setFileOK_writeAndRestore)
{
    const std::string fname = "ut_log_file_test.log";
    std::remove(fname.c_str());

    // switch to file
    ASSERT_TRUE(UniCoutLog::setLogFileOK(fname));
    INF("hello file");
    ASSERT_GT(UniCoutLog::logLen(), 0u);

    // verify file has content
    {
        std::ifstream fin(fname);
        ASSERT_TRUE(fin.good());
        std::string line;
        std::getline(fin, line);
        EXPECT_NE(line.find("hello file"), std::string::npos) << "REQ: log written to file";
    }

    // dumpAll_forUt restores to cout
    UniCoutLog::dumpAll_forUt();
    INF("hello cout again");
    ASSERT_GT(UniCoutLog::logLen(), 0u) << "REQ: can still log after restore";

    std::remove(fname.c_str());
}

TEST_F(UniCoutLogTest, setLogFileOK_emptyName_switchToCout)
{
    // first switch to file
    const std::string fname = "ut_log_empty_test.log";
    std::remove(fname.c_str());
    ASSERT_TRUE(UniCoutLog::setLogFileOK(fname));

    // empty name → switch back to cout
    ASSERT_TRUE(UniCoutLog::setLogFileOK("")) << "REQ: empty name = switch to cout";
    INF("back to cout");
    ASSERT_GT(UniCoutLog::logLen(), 0u) << "REQ: can log after switch to cout";

    std::remove(fname.c_str());
}

TEST_F(UniCoutLogTest, setLogFileOK_badPath_fail)
{
    ASSERT_FALSE(UniCoutLog::setLogFileOK("/nonexistent_dir_12345/impossible.log"))
        << "REQ: bad path returns false";

    // should still log to cout (unchanged)
    INF("still cout");
    ASSERT_GT(UniCoutLog::logLen(), 0u) << "REQ: can still log after failed setLogFileOK";
}
}  // namespace rlib
