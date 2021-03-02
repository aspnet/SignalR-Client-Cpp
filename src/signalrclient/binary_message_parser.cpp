// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_MSGPACK
#include "binary_message_parser.h"
#include "signalrclient/signalr_exception.h"

namespace signalr
{
    namespace binary_message_parser
    {
        bool try_parse_message(const unsigned char* message, size_t length, size_t* length_prefix_length, size_t* length_of_message)
        {
            if (length == 0)
            {
                return false;
            }

            // The payload starts with a length prefix encoded as a VarInt. VarInts use the most significant bit
            // as a marker whether the byte is the last byte of the VarInt or if it spans to the next byte. Bytes
            // appear in the reverse order - i.e. the first byte contains the least significant bits of the value
            // Examples:
            // VarInt: 0x35 - %00110101 - the most significant bit is 0 so the value is %x0110101 i.e. 0x35 (53)
            // VarInt: 0x80 0x25 - %10000000 %00101001 - the most significant bit of the first byte is 1 so the
            // remaining bits (%x0000000) are the lowest bits of the value. The most significant bit of the second
            // byte is 0 meaning this is last byte of the VarInt. The actual value bits (%x0101001) need to be
            // prepended to the bits we already read so the values is %01010010000000 i.e. 0x1480 (5248)

            size_t num_bytes = 0;
            size_t message_length = 0;
            size_t length_prefix_buffer = std::min((size_t)5, length);

            unsigned char byte_read;
            do
            {
                byte_read = message[num_bytes];
                message_length = message_length | (((size_t)(byte_read & 0x7f)) << (num_bytes * 7));
                num_bytes++;
            }
            while (num_bytes < length_prefix_buffer && ((int)byte_read & 0x80) != 0);

            // size bytes are missing
            if ((byte_read & 0x80) != 0 && (num_bytes < 5))
            {
                throw signalr_exception("partial messages are not supported.");
            }

            if ((byte_read & 0x80) != 0 || (num_bytes == 5 && byte_read > 7))
            {
                throw signalr_exception("messages over 2GB are not supported.");
            }

            // not enough data with the given length prefix
            if (length < message_length + num_bytes)
            {
                throw signalr_exception("partial messages are not supported.");
            }

            *length_prefix_length = num_bytes;
            *length_of_message = message_length;
            return true;
        }
    }
}

#endif