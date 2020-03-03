// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "url_builder.h"

using namespace signalr;

TEST(url_builder_negotiate, url_correct_if_query_string_empty)
{
    ASSERT_EQ(
        "http://fake/negotiate",
        url_builder::build_negotiate("http://fake/"));
}

TEST(url_builder_negotiate, url_correct_if_query_string_not_empty)
{
    ASSERT_EQ(
        "http://fake/negotiate?q1=1&q2=2",
        url_builder::build_negotiate("http://fake/?q1=1&q2=2"));
}

TEST(url_builder_connect_webSockets, url_correct_if_query_string_not_empty)
{
    ASSERT_EQ(
        "ws://fake/?q1=1&q2=2",
        url_builder::build_connect("http://fake/", transport_type::websockets, "q1=1&q2=2"));
}

TEST(url_builder_connect, url_correct_if_query_string_not_empty_and_adding_query_string)
{
    ASSERT_EQ(
        "ws://fake/?q=0&q1=1&q2=2",
        url_builder::build_connect("http://fake/?q=0", transport_type::websockets, "q1=1&q2=2"));
}