[![Build Status](https://github.com/nokia/domino-library/actions/workflows/ci.yml/badge.svg)](https://github.com/nokia/domino-library/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/nokia/domino-library/branch/main/graph/badge.svg?token=LGK8GD9GJD)](https://codecov.io/gh/nokia/domino-library)

![](image/domino.jpg)
# Domino Library
Reuse-Library of C++, include classes eg:

1. ObjAnywhere ([手册*](https://mp.weixin.qq.com/s/SYE3xkz-Zqp-l46ZpjnKWg))

    How to get any **object anywhere** in a program directly (not by passing parameter)?

2. Domino(s) ([手册*](https://mp.weixin.qq.com/s/ckF2LXH4hDcIYbZNqSIb0g))

    How can a program **adaptively** perform at its best in any scenario?

3. MsgSelf ([手册*](https://mp.weixin.qq.com/s/aPjhY7nRmlD4xHhUL_ykxg))

    How to avoid nested callback?

4. SmartLog ([手册](https://mp.weixin.qq.com/s/KNKBC-uHOylRXxpspZbVnA) | [Manual](https://mp.weixin.qq.com/s/X3XZOGOQGDQtwQDEPNA32A))

    How to collect only error related logs? eg, only print for failed ut case(s):
    ![](image/ut_smartlog.jpg)

5. ThreadBack ([手册*](https://mp.weixin.qq.com/s/bb1slMqhuoBLZZCd3NmbYA))

    How to coordinate main thread(containing all logic) with other threads(time-consuming tasks)

**(*) English manual is not ready yet.**


# New Practices
1. UT testcase is requirement. (Refer to ./ut/\*/\*.cpp)

2. LEGO domino classes. (Refer to ./ut/obj_anywhere/UtInitObjAnywhere.hpp)


# How to use this library?
Just copy the source code into your project. Please obey its license.

# Contribution
(not ready yet)


# Contact
sz.chen@nokia-sbell.com
