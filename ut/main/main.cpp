/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include<gtest/gtest.h>
#include "CppLog.hpp"

using namespace std;

struct MyListener : public testing::EmptyTestEventListener
{
    ~MyListener()
    {
        cout << __func__ << "/destructed; nLogged=" << nLogged
            << ". [nLogged==0(smart log) or maxCase(force log) means StrCoutFSL works well!!!]" << endl;
    };

    void OnTestEnd(const testing::TestInfo& aInfo)
    {
        const bool needLog = not aInfo.result()->Passed() || not RLib::CppLog::smartLog_.canDelLog();
        if(needLog)
        {
            RLib::CppLog::smartLog_.forceSave();
            cout << "!!!nLogged=" << ++nLogged << endl;
        }
        RLib::CppLog::smartLog_.forceDel();
    }

    static size_t nLogged;
};
size_t MyListener::nLogged = 0;


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::UnitTest::GetInstance()->listeners().Append(new MyListener);
    int result = RUN_ALL_TESTS();
    return result;
}
