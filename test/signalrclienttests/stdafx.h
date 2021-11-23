// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef _WIN32
// prevents from defining min/max macros that conflict with std::min()/std::max() functions
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "gtest/gtest.h"
#include "../../src/signalrclient/cancellation_token_source.h"
#include <thread>