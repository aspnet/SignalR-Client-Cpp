// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#define STRINGIFY_2(s) #s
#define STRINGIFY(s) STRINGIFY_2(s)

#ifdef INJECT_TRACE_HEADER
#include STRINGIFY(INJECT_TRACE_HEADER)
#endif

#ifndef DbgLogInfo
#define DbgLogInfo(...)
#endif

#include <functional>
#include <unordered_map>
#include <string>