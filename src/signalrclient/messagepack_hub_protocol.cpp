// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_MSGPACK
#include "messagepack_hub_protocol.h"
#include "message_type.h"
#include "signalrclient/signalr_exception.h"
#include <msgpack.hpp>
#include "binary_message_parser.h"
#include "binary_message_formatter.h"

namespace signalr
{
    // replaces use of msgpack::sbuffer when packing so that we can write directly to the string instead of writing to another buffer and copying to a string later
    class string_wrapper
    {
    public:
        std::string str;

        string_wrapper& write(const char* buf, size_t s)
        {
            str.append(buf, s);
            return *this;
        }
    };

    signalr::value createValue(const msgpack::object& v)
    {
        switch (v.type)
        {
        case msgpack::type::object_type::BOOLEAN:
            return signalr::value(v.via.boolean);
        case msgpack::type::object_type::FLOAT64:
        case msgpack::type::object_type::FLOAT32:
            return signalr::value(v.via.f64);
        case msgpack::type::object_type::POSITIVE_INTEGER:
            return signalr::value((double)v.via.u64);
        case msgpack::type::object_type::NEGATIVE_INTEGER:
            return signalr::value((double)v.via.i64);
        case msgpack::type::object_type::STR:
            return signalr::value(v.via.str.ptr, v.via.str.size);
        case msgpack::type::object_type::ARRAY:
        {
            std::vector<signalr::value> vec;
            for (size_t i = 0; i < v.via.array.size; ++i)
            {
                vec.push_back(createValue(*(v.via.array.ptr + i)));
            }
            return signalr::value(std::move(vec));
        }
        case msgpack::type::object_type::MAP:
        {
            std::map<std::string, signalr::value> map;
            for (size_t i = 0; i < v.via.map.size; ++i)
            {
                auto key = (v.via.map.ptr + i)->key.as<std::string>();
                map.insert({ key, createValue((v.via.map.ptr + i)->val) });
            }
            return signalr::value(std::move(map));
        }
        case msgpack::type::object_type::BIN:
            throw signalr_exception("messagepack type 'BIN' not supported");
        case msgpack::type::object_type::EXT:
            throw signalr_exception("messagepack type 'EXT' not supported");
        case msgpack::type::object_type::NIL:
        default:
            return signalr::value();
        }
    }

    void pack_messagepack(const signalr::value& v, msgpack::packer<string_wrapper>& packer)
    {
        switch (v.type())
        {
        case signalr::value_type::boolean:
        {
            if (v.as_bool())
            {
                packer.pack_true();
            }
            else
            {
                packer.pack_false();
            }
            return;
        }
        case signalr::value_type::float64:
        {
            auto value = v.as_double();
            double intPart;
            // Workaround for 1.0 being output as 1.0 instead of 1
            // because the server expects certain values to be 1 instead of 1.0 (like protocol version)
            if (std::modf(value, &intPart) == 0)
            {
                if (value < 0)
                {
                    if (value >= (double)INT64_MIN)
                    {
                        // Fits within int64_t
                        packer.pack_int64(static_cast<int64_t>(intPart));
                        return;
                    }
                    else
                    {
                        // Remain as double
                        packer.pack_double(value);
                        return;
                    }
                }
                else
                {
                    if (value <= (double)UINT64_MAX)
                    {
                        // Fits within uint64_t
                        packer.pack_uint64((uint64_t)intPart);
                        return;
                    }
                    else
                    {
                        // Remain as double
                        packer.pack_double(value);
                        return;
                    }
                }
            }
            packer.pack_double(v.as_double());
            return;
        }
        case signalr::value_type::string:
        {
            auto length = v.as_string().length();
            packer.pack_str((uint32_t)length);
            packer.pack_str_body(v.as_string().data(), (uint32_t)length);
            return;
        }
        case signalr::value_type::array:
        {
            const auto& array = v.as_array();
            packer.pack_array((uint32_t)array.size());
            for (auto& val : array)
            {
                pack_messagepack(val, packer);
            }
            return;
        }
        case signalr::value_type::map:
        {
            const auto& obj = v.as_map();
            packer.pack_map((uint32_t)obj.size());
            for (auto& val : obj)
            {
                packer.pack_str((uint32_t)val.first.size());
                packer.pack_str_body(val.first.data(), (uint32_t)val.first.size());
                pack_messagepack(val.second, packer);
            }
            return;
        }
        case signalr::value_type::null:
        default:
        {
            packer.pack_nil();
            return;
        }
        }
    }

    std::string signalr::messagepack_hub_protocol::write_message(const hub_message* hub_message) const
    {
        string_wrapper str;
        msgpack::packer<string_wrapper> packer(str);

#pragma warning (push)
#pragma warning (disable: 4061)
        switch (hub_message->message_type)
        {
        case message_type::invocation:
        {
            auto invocation = static_cast<invocation_message const*>(hub_message);

            packer.pack_array(6);

            packer.pack_int((int)message_type::invocation);
            // Headers
            packer.pack_map(0);

            if (invocation->invocation_id.empty())
            {
                packer.pack_nil();
            }
            else
            {
                packer.pack_str((uint32_t)invocation->invocation_id.length());
                packer.pack_str_body(invocation->invocation_id.data(), (uint32_t)invocation->invocation_id.length());
            }

            packer.pack_str((uint32_t)invocation->target.length());
            packer.pack_str_body(invocation->target.data(), (uint32_t)invocation->target.length());

            packer.pack_array((uint32_t)invocation->arguments.as_array().size());
            for (auto& val : invocation->arguments.as_array())
            {
                pack_messagepack(val, packer);
            }

            // StreamIds
            packer.pack_array(0);

            break;
        }
        case message_type::completion:
        {
            auto completion = static_cast<completion_message const*>(hub_message);

            size_t result_kind = completion->error.empty() ? (completion->has_result ? 3U : 2U) : 1U;
            packer.pack_array(4U + (result_kind != 2U ? 1U : 0U));

            packer.pack_int((int)message_type::completion);

            // Headers
            packer.pack_map(0);

            packer.pack_str((uint32_t)completion->invocation_id.length());
            packer.pack_str_body(completion->invocation_id.data(), (uint32_t)completion->invocation_id.length());

            packer.pack_int((int)result_kind);
            switch (result_kind)
            {
            // error result
            case 1:
                packer.pack_str((uint32_t)completion->error.length());
                packer.pack_str_body(completion->error.data(), (uint32_t)completion->error.length());
                break;
            // non-void result
            case 3:
                pack_messagepack(completion->result, packer);
                break;
            }

            break;
        }
        case message_type::ping:
        {
            // If we need the ping this is how you get it
            // auto ping = static_cast<ping_message const*>(hub_message);

            packer.pack_array(1);
            packer.pack_int((int)message_type::ping);

            break;
        }
        default:
            break;
        }
#pragma warning (pop)

        binary_message_formatter::write_length_prefix(str.str);
        return str.str;
    }

    std::vector<std::unique_ptr<hub_message>> messagepack_hub_protocol::parse_messages(const std::string& message) const
    {
        std::vector<std::unique_ptr<hub_message>> vec;

        size_t length_prefix_length;
        size_t length_of_message;
        const char* remaining_message = message.data();
        size_t remaining_message_length = message.length();

        while (binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(remaining_message), remaining_message_length, &length_prefix_length, &length_of_message))
        {
            assert(length_prefix_length <= remaining_message_length);
            remaining_message += length_prefix_length;
            remaining_message_length -= length_prefix_length;
            assert(remaining_message_length >= length_of_message);

            msgpack::unpacker pac;
            pac.reserve_buffer(length_of_message);
            memcpy(pac.buffer(), remaining_message, length_of_message);
            pac.buffer_consumed(length_of_message);
            msgpack::object_handle obj_handle;

            if (!pac.next(obj_handle))
            {
                throw signalr_exception("messagepack object was incomplete");
            }

            auto& msgpack_obj = obj_handle.get();

            if (msgpack_obj.type != msgpack::type::ARRAY)
            {
                throw signalr_exception("Message was not an 'array' type");
            }

            auto num_elements_of_message = msgpack_obj.via.array.size;
            if (msgpack_obj.via.array.size == 0)
            {
                throw signalr_exception("Message was an empty array");
            }

            auto msgpack_obj_index = msgpack_obj.via.array.ptr;
            if (msgpack_obj_index->type != msgpack::type::POSITIVE_INTEGER)
            {
                throw signalr_exception("reading 'type' as int failed");
            }
            auto type = msgpack_obj_index->via.i64;
            ++msgpack_obj_index;

#pragma warning (push)
#pragma warning (disable: 4061)
            switch ((message_type)type)
            {
            case message_type::invocation:
            {
                if (num_elements_of_message < 5)
                {
                    throw signalr_exception("invocation message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                std::string invocation_id;
                if (msgpack_obj_index->type == msgpack::type::STR)
                {
                    invocation_id.append(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                }
                else if (msgpack_obj_index->type != msgpack::type::NIL)
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::STR)
                {
                    throw signalr_exception("reading 'target' as string failed");
                }

                std::string target(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::ARRAY)
                {
                    throw signalr_exception("reading 'arguments' as array failed");
                }

                vec.emplace_back(std::unique_ptr<hub_message>(
                    new invocation_message(std::move(invocation_id), std::move(target), createValue(*msgpack_obj_index))));

                if (num_elements_of_message > 5)
                {
                    // This is for the StreamIds when they are supported
                    ++msgpack_obj_index;
                }

                break;
            }
            case message_type::completion:
            {
                if (num_elements_of_message < 4)
                {
                    throw signalr_exception("completion message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::STR)
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                std::string invocation_id(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::POSITIVE_INTEGER)
                {
                    throw signalr_exception("reading 'result_kind' as int failed");
                }
                auto result_kind = msgpack_obj_index->via.i64;
                ++msgpack_obj_index;

                if (num_elements_of_message < 5 && result_kind != 2)
                {
                    throw signalr_exception("completion message has too few properties");
                }

                std::string error;
                signalr::value result;
                // 1: error
                // 2: void result
                // 3: non void result
                if (result_kind == 1)
                {
                    if (msgpack_obj_index->type != msgpack::type::STR)
                    {
                        throw signalr_exception("reading 'error' as string failed");
                    }
                    error.append(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                }
                else if (result_kind == 3)
                {
                    result = createValue(*msgpack_obj_index);
                }

                vec.emplace_back(std::unique_ptr<hub_message>(
                    new completion_message(std::move(invocation_id), std::move(error), std::move(result), result_kind == 3)));
                break;
            }
            case message_type::ping:
            {
                vec.emplace_back(std::unique_ptr<hub_message>(new ping_message()));
                break;
            }
            // TODO: other message types
            default:
                // Future protocol changes can add message types, old clients can ignore them
                break;
            }
#pragma warning (pop)

            remaining_message += length_of_message;
            assert(remaining_message_length - length_of_message < remaining_message_length);
            remaining_message_length -= length_of_message;
        }

        return vec;
    }
}

#endif