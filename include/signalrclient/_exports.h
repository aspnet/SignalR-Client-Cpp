// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

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
#ifdef (cdecl)
#define __cdecl __attribute__((cdecl))
#else
#define __cdecl
#endif // cdecl
#endif // !__cdecl
#endif // _WIN32