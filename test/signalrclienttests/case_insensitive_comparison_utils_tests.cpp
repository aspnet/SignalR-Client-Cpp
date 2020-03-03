// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "case_insensitive_comparison_utils.h"

using namespace signalr;

TEST(case_insensitive_equals_functor, basic_comparison_tests)
{
    case_insensitive_equals case_insensitive_compare;

    ASSERT_TRUE(case_insensitive_compare("", ""));
    ASSERT_TRUE(case_insensitive_compare("abc", "ABC"));
    ASSERT_TRUE(case_insensitive_compare("abc123!@", "ABC123!@"));

    ASSERT_FALSE(case_insensitive_compare("abc", "ABCD"));
    ASSERT_FALSE(case_insensitive_compare("abce", "ABCD"));
}

TEST(case_insensitive_hash_functor, basic_hash_tests)
{
    case_insensitive_hash case_insensitive_hasher;

    ASSERT_EQ(0U, case_insensitive_hasher(""));

    ASSERT_EQ(case_insensitive_hasher("abc"), case_insensitive_hasher("ABC"));
    ASSERT_EQ(case_insensitive_hasher("abc123!@"), case_insensitive_hasher("ABC123!@"));
    ASSERT_NE(case_insensitive_hasher("abcd"), case_insensitive_hasher("ABC"));
}
