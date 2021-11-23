// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#define STRINGIFY_2(s) #s
#define STRINGIFY(s) STRINGIFY_2(s)

#ifdef _WIN32 // used in the default log writer and to build the dll

// prevents from defining min/max macros that conflict with std::min()/std::max() functions
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif

#include <functional>
#include <unordered_map>