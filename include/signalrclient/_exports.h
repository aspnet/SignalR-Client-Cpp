// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef NO_SIGNALRCLIENT_EXPORTS
#define SIGNALRCLIENT_API
#else
#ifdef SIGNALRCLIENT_EXPORTS
#define SIGNALRCLIENT_API __declspec(dllexport)
#else
#define SIGNALRCLIENT_API __declspec(dllimport)
#endif // SIGNALRCLIENT_EXPORTS
#endif // NO_SIGNALRCLIENT_EXPORTS

#ifndef _WIN32
#ifndef __cdecl
#ifdef cdecl
#define __cdecl __attribute__((cdecl))
#else
#define __cdecl
#endif // cdecl
#endif // !__cdecl
#endif // _WIN32