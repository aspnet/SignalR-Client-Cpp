/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Builder style class for creating URIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

// Copied and modified slightly from https://github.com/microsoft/cpprestsdk/blob/fd6aa00c18a7fb8dbe957175078f02346775045a/Release/include/cpprest/asyncrt_utils.h

#pragma once

#include <string>
#include <locale>
#include <sstream>
#include <limits.h>

namespace signalr
{
    namespace utility
    {
        struct to_lower_ch_impl
        {
            char operator()(char c) const noexcept
            {
                if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
                return c;
            }

            wchar_t operator()(wchar_t c) const noexcept
            {
                if (c >= L'A' && c <= L'Z') return static_cast<wchar_t>(c - L'A' + L'a');
                return c;
            }
        };

        constexpr to_lower_ch_impl to_lower_ch{};

        template<typename Target>
        Target scan_string(const std::string& str)
        {
            Target t;
            std::istringstream iss(str);
            iss.imbue(std::locale::classic());
            iss >> t;
            if (iss.bad())
            {
                throw std::bad_cast();
            }
            return t;
        }

        void inplace_tolower(std::string& target) noexcept
        {
            for (auto& ch : target)
            {
                ch = to_lower_ch(ch);
            }
        }

        /// <summary>
        /// Our own implementation of alpha numeric instead of std::isalnum to avoid
        /// taking global lock for performance reasons.
        /// </summary>
        inline bool is_alnum(const unsigned char uch) noexcept
        {
            // test if uch is an alnum character
            // special casing char to avoid branches
            // clang-format off
            static constexpr bool is_alnum_table[UCHAR_MAX + 1] = {
                /*        X0 X1 X2 X3 X4 X5 X6 X7 X8 X9 XA XB XC XD XE XF */
                /* 0X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 1X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 2X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 3X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0-9 */
                /* 4X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* A-Z */
                /* 5X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
                /* 6X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* a-z */
                /* 7X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
                /* non-ASCII values initialized to 0 */
            };
            // clang-format on
            return (is_alnum_table[uch]);
        }
    }
}