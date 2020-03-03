// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef _WIN32
// prevents from defining min/max macros that conflict with std::min()/std::max() functions
#define NOMINMAX
#endif

#include "gtest/gtest.h"
#include "../../src/signalrclient/cancellation_token.h"
#include "../../src/signalrclient/make_unique.h"
#include <thread>