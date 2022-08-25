/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "UniLog.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct Cell : public UniLog
{
    // req: can log
    Cell(const UniLogName aCellName = ULN_DEFAULT) : UniLog(aCellName) { DBG("hello world, I'm a cell, this=" << this); }
    ~Cell() { DBG("bye world, I'm a cell, this=" << this); }
};

struct CellMember : public UniLog
{
    // req: can log
    CellMember(UniLogName aCellName = ULN_DEFAULT) : UniLog(aCellName) { DBG("hello world, I'm a cell member"); }
    ~CellMember() { DBG("bye world, I'm a cell member"); }
};

void cellParticipant(UniLog& oneLog = UniLog::defaultCellLog())
{
    DBG("hello world, I'm a cell's participant");  // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, GOLD_cell_member_participant)
{
    const char CELL_NAME[] = "GOLD_cell_member_participant";
    EXPECT_EQ(0, UniLog::logLen(CELL_NAME));
    {
        Cell cell(CELL_NAME);
        const auto len_1 = UniLog::logLen(CELL_NAME);
        EXPECT_GT(len_1, 0);                    // req: can log

        CellMember member(CELL_NAME);
        const auto len_2 = UniLog::logLen(CELL_NAME);
        EXPECT_GT(len_2, len_1);                // req: can log more in same log

        cellParticipant(cell);                  // req: cell can call func & log into same smartlog
        const auto len_3 = UniLog::logLen(CELL_NAME);
        EXPECT_GT(len_3, len_2);                // req: can log more in same log

        cellParticipant(member);                // req: cell member can call func & log into same smartlog
        const auto len_4 = UniLog::logLen(CELL_NAME);
        EXPECT_GT(len_4, len_3);                // req: can log more in same log

        UniLog::needLog();                     // req: shall output log to screen
    }
    EXPECT_EQ(0, UniLog::nLog());              // req: del log when no user
}

// ***********************************************************************************************
TEST(CellLogTest, low_couple_cell_and_member)
{
    const char CELL_NAME[] = "low_couple_cell_and_member";
    auto cell = std::make_shared<Cell>((CELL_NAME));
    const auto len_1 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_1, 0);                        // req: can log

    auto member = std::make_shared<CellMember>(CELL_NAME);
    const auto len_2 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_2, len_1);                    // req: can log

    cell.reset();
    const auto len_3 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_3, len_2);                    // req: Cell-destructed shall not crash/impact CellMember's logging

    if (Test::HasFailure()) UniLog::needLog();
    member.reset();
    EXPECT_EQ(0, UniLog::nLog());              // req: del log when no user
}
TEST(CellLogTest, low_couple_between_copies)
{
    const char CELL_NAME[] = "low_couple_between_copies";
    auto cell = std::make_shared<Cell>((CELL_NAME));
    const auto len_1 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_1, 0);                        // req: can log

    auto copy = std::make_shared<Cell>(*cell);
    const auto len_2 = UniLog::logLen(CELL_NAME);
    EXPECT_EQ(len_2, len_1);                    // req: log still there

    cell.reset();
    const auto len_3 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_3, len_2);                    // req: Cell-destructed shall not crash/impact copy's logging

    if (Test::HasFailure()) UniLog::needLog();
    copy.reset();
    EXPECT_EQ(0, UniLog::nLog());              // req: del log when no user
}
TEST(CellLogTest, low_couple_callbackFunc)
{
    const char CELL_NAME[] = "low_couple_callbackFunc";
    auto cell = std::make_shared<Cell>((CELL_NAME));
    const auto len_1 = UniLog::logLen(CELL_NAME);
    EXPECT_GT(len_1, 0);                        // req: can log
    {
        std::function<void()> cb = [oneLog = *cell]() mutable { INF("hello world, I'm a callback func"); };
        const auto len_2 = UniLog::logLen(CELL_NAME);
        EXPECT_GE(len_2, len_1);                // req: log still there (more log since no move-construct of Cell)

        cell.reset();
        cb();
        const auto len_3 = UniLog::logLen(CELL_NAME);
        EXPECT_GT(len_3, len_2);                // req: can log

        if (Test::HasFailure()) UniLog::needLog();
    }
    EXPECT_EQ(0, UniLog::nLog());              // req: del log when no user
}

// ***********************************************************************************************
struct NonCell
{
    NonCell() { DBG("hello world, I'm NOT a cell"); }  // req: can log
    ~NonCell() { DBG("bye world, I'm NOT a cell"); }   // req: can log
};

void nonCellFunc()
{
    DBG("hello world, I'm NOT a cell's participant");  // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, no_explicit_CellLog_like_legacy)
{
    {
        Cell cell;                      // req: no explicit UniLog
        CellMember member;              // req: no explicit UniLog
        cellParticipant();              // req: no explicit UniLog

        NonCell nonCell;                // req: class not based on UniLog
        nonCellFunc();                  // req: func w/o UniLog para
    }
    if (Test::HasFailure()) UniLog::needLog();
    UniLog::defaultCellLog_.reset();   // dump log in time
    EXPECT_EQ(0, UniLog::nLog());      // req: del log when no user
}

}  // namespace
