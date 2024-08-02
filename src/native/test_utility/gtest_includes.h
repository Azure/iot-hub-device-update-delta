/**
 * @file gtest_includes.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once


#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 26439)
#pragma warning(disable : 26495)
#endif

#include "gtest/gtest.h"
using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

#ifdef WIN32
#pragma warning(pop)
#endif