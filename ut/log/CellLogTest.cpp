/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "CellLog.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct Cell : public CellLog
{
    // req: can log
    Cell(const CellName aCellName = CELL_NAME_DEFAULT) : CellLog(aCellName) { DBG("hello world, I'm a cell"); }
    ~Cell() { DBG("bye world, I'm a cell"); }
};

struct CellMember : public CellLog
{
    // req: can log
    CellMember(CellName aCellName = CELL_NAME_DEFAULT) : CellLog(aCellName) { DBG("hello world, I'm a cell member"); }
    ~CellMember() { DBG("bye world, I'm a cell member"); }
};

void cellParticipant(CellLog& ssLog = CellLog::defaultCellLog())
{
    DBG("hello world, I'm a cell's participant");  // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, GOLD_cell_member_participant)
{
    const char CELL_NAME[] = "GOLD_cell_member_participant";
    {
        Cell cell(CELL_NAME);
        const auto len_1 = CellLog::logLen(CELL_NAME);
        EXPECT_GT(len_1, 0);                    // req: can log

        CellMember member(CELL_NAME);
        const auto len_2 = CellLog::logLen(CELL_NAME);
        EXPECT_GT(len_2, len_1);                // req: can log more in same log

        cellParticipant(cell);                  // req: cell can call func & log into same smartlog
        const auto len_3 = CellLog::logLen(CELL_NAME);
        EXPECT_GT(len_3, len_2);                // req: can log more in same log

        cellParticipant(member);                // req: cell member can call func & log into same smartlog
        const auto len_4 = CellLog::logLen(CELL_NAME);
        EXPECT_GT(len_4, len_3);                // req: can log more in same log

        member.needLog();                       // req: shall output log to screen
    }
    EXPECT_EQ(0, CellLog::nCellLog());          // req: del log when no user
}

// ***********************************************************************************************
TEST(CellLogTest, low_couple_cell_and_member)
{
    const char CELL_NAME[] = "low_couple_cell_and_member";
    auto cell = std::make_shared<Cell>((CELL_NAME));
    const auto len_1 = CellLog::logLen(CELL_NAME);
    EXPECT_GT(len_1, 0);                        // req: can log

    auto member = std::make_shared<CellMember>(CELL_NAME);
    const auto len_2 = CellLog::logLen(CELL_NAME);
    EXPECT_GT(len_2, len_1);                    // req: can log

    cell.reset();
    const auto len_3 = CellLog::logLen(CELL_NAME);
    EXPECT_GT(len_3, len_2);                    // req: Cell-destructed shall not crash/impact CellMember's logging

    member.reset();
    EXPECT_EQ(0, CellLog::nCellLog());          // req: del log when no user
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
        Cell cell;                      // req: no explicit CellLog
        CellMember member;              // req: no explicit CellLog
        cellParticipant();              // req: no explicit CellLog

        member.needLog();               // req: shall output log to screen

        NonCell nonCell;                // req: class not based on CellLog
        nonCellFunc();                  // req: func w/o CellLog para
    }
    //EXPECT_EQ(0, CellLog::nCellLog());  // req: del log when no user
}

}  // namespace
