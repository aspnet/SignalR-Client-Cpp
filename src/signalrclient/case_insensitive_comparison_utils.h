// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <string>
#include <cctype>

namespace signalr
{
    // Note: These functions are not pretending to be all-purpose helpers for case insensitive string comparison. Rather
    // we use them to compare hub and hub method names which are expected to be almost exclusively ASCII and this is the
    // simplest thing that would work without having to take dependencies on third party libraries.
    struct case_insensitive_equals
    {
        bool operator()(const std::string& s1, const std::string& s2) const
        {
            if (s1.length() != s2.length())
            {
                return false;
            }

            for (unsigned i = 0; i < s1.size(); ++i)
            {
                if (std::toupper(s1[i]) != std::toupper(s2[i]))
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct case_insensitive_hash
    {
        std::size_t operator()(const std::string& s) const noexcept
        {
            size_t hash = 0;
            std::hash<size_t> hasher;
            for (const auto& c : s)
            {
                hash ^= hasher(static_cast<size_t>(std::toupper(c))) + (hash << 5) + (hash >> 2);
            }

            return hash;
        }
    };
}
