/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
*/

#include "gtest/gtest.h"

#include "internal/Error.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    int retval = RUN_ALL_TESTS();
    return retval;
}
