/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2013 German Aerospace Center (DLR)
*
* Created: 2012-10-17 Markus Litz <Markus.Litz@dlr.de>
*/

#include "gtest/gtest.h"
#include "internal/Error.h"


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    int retval = RUN_ALL_TESTS();
    return retval;
}
