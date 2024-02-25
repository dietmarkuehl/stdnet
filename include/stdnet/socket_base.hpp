// stdnet/socket_base.hpp                                             -*-C++-*-
// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_SOCKET_BASE
#define INCLUDED_STDNET_SOCKET_BASE

#include <stdnet/netfwd.hpp>
#include <sys/socket.h>

// ----------------------------------------------------------------------------

class stdnet::socket_base
{
public:
    template <typename _Value_t, int _Level, int _Name>
    class _Socket_option
    {
    private:
        _Value_t _D_value;
    
    public:
        explicit _Socket_option(_Value_t _V): _D_value(_V) {}
        _Value_t _Value() const { return this->_D_value; }
        template <typename _Protocol> auto data(_Protocol&&) const -> _Value_t const* { return &this->_D_value; }
        template <typename _Protocol> auto data(_Protocol&&)       -> _Value_t const* { return &this->_D_value; }
        template <typename _Protocol> constexpr auto level(_Protocol&&) const -> int { return _Level; }
        template <typename _Protocol> constexpr auto name(_Protocol&&) const -> int { return _Name; }
        template <typename _Protocol> constexpr auto size(_Protocol&&) const -> ::socklen_t { return sizeof(_Value_t); }
    };
    class broadcast;
    class debug;
    class do_not_route;
    class keep_alive;
    class linger;
    class out_of_band_inline;
    class receive_buffer_size;
    class receive_low_watermark;
    class reuse_address
        : public _Socket_option<int, SOL_SOCKET, SO_REUSEADDR>
    {
    public:
        explicit reuse_address(bool _Value): _Socket_option(_Value) {}
        explicit operator bool() const { return this->_Value(); }
    };
    class send_buffer_size;
    class send_low_watermark;

    using shutdown_type = int; //-dk:TODO
    static constexpr shutdown_type shutdown_receive{1};
    static constexpr shutdown_type shutdown_send{2};
    static constexpr shutdown_type shutdown_both{3};

    using wait_type = int; //-dk:TODO
    static constexpr wait_type wait_read{1};
    static constexpr wait_type wait_write{2};
    static constexpr wait_type wait_error{3};

    using message_flags = int; //-dk:TODO
    static constexpr message_flags message_peek{1};
    static constexpr message_flags message_out_of_band{2};
    static constexpr message_flags message_do_not_route{3};

    static constexpr int max_listen_connections{SOMAXCONN};

protected:
    socket_base() = default;
    ~socket_base() = default;
};

// ----------------------------------------------------------------------------

#endif
