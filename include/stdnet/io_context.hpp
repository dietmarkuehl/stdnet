// stdnet/io_context.hpp                                              -*-C++-*-
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

#ifndef INCLUDED_STDNET_IO_CONTEXT
#define INCLUDED_STDNET_IO_CONTEXT
#pragma once

#include <stdnet/netfwd.hpp>
#include <cstdint>
#include <sys/socket.h>

// ----------------------------------------------------------------------------

namespace stdnet
{
    namespace _Hidden
    {
        enum _Socket_id: ::std::uint_least32_t {};

        template <typename... _Args>
        struct _Io_base
        {
            virtual void _Complete(_Args...) = 0;
        };

        template <typename _Record>
        struct _Container
        {
            ::std::vector<::std::variant<::std::size_t, _Record>> _Records;
            ::std::size_t                                         _Free{};

            ::stdnet::_Hidden::_Socket_id _Insert(_Record _R)
            {
                if (this->_Free == this->_Records.size())
                {
                    this->_Records.emplace_back(::std::move(_R));
                    return ::stdnet::_Hidden::_Socket_id(++this->_Free);
                }
                else
                {
                    ::std::size_t _Rc(std::exchange(this->_Free, ::std::get<0>(this->_Records[this->_Free])));
                    this->_Records[_Rc] = ::std::move(_R);
                    return ::stdnet::_Hidden::_Socket_id(_Rc);
                }
            }
            void _Erase(::stdnet::_Hidden::_Socket_id _Id)
            {
                this->_Records[::std::size_t(_Id)] = std::exchange(this->_Free, ::std::size_t(_Id));
            }
        };
    }
    namespace _Hidden_abstract
    {
        struct _Context
        {
            virtual ~_Context() = default;
            virtual ::stdnet::_Hidden::_Socket_id _Make_socket(int, int, int) = 0;
            virtual void _Release(::stdnet::_Hidden::_Socket_id) = 0;

            virtual ::std::size_t run_one() = 0;

            virtual void _Accept(::stdnet::_Hidden::_Socket_id,
                                 ::stdnet::_Hidden::_Io_base<::stdnet::_Hidden::_Socket_id, ::stdnet::ip::tcp::endpoint>*) = 0;
        };
    }
    namespace _Hidden_poll
    {
        struct _Record
        {
            _Record(::stdnet::_Stdnet_native_handle_type _H): _Handle(_H) {}
            ::stdnet::_Stdnet_native_handle_type _Handle;
        };
        struct _Context final
            : ::stdnet::_Hidden_abstract::_Context
        {
            ::stdnet::_Hidden::_Container<::stdnet::_Hidden_poll::_Record> _D_sockets;

            ::stdnet::_Hidden::_Socket_id _Make_socket(int _D, int _T, int _P) override final
            {
                 return this->_D_sockets._Insert(::socket(_D, _T, _P));
            }
            void _Release(::stdnet::_Hidden::_Socket_id _Id) override final
            {
                this->_D_sockets._Erase(_Id);
            }

            ::std::size_t run_one() override final
            {
                return ::std::size_t{};
            }

            void _Accept(::stdnet::_Hidden::_Socket_id,
                         ::stdnet::_Hidden::_Io_base<::stdnet::_Hidden::_Socket_id, ::stdnet::ip::tcp::endpoint>*) override final
            {
            }
        };
    }
    namespace _Hidden_kqueue
    {

    }
    
    class io_context;
}

// ----------------------------------------------------------------------------

class stdnet::io_context
{
private:
    ::std::unique_ptr<::stdnet::_Hidden_abstract::_Context> _Owned{new ::stdnet::_Hidden_poll::_Context()};
    ::stdnet::_Hidden_abstract::_Context&                   _Context{*this->_Owned};

public:
    class scheduler_type {};
    class executor_type {};

    io_context() = default;
    io_context(::stdnet::_Hidden_abstract::_Context& _Context): _Owned(), _Context(_Context) {}
    io_context(io_context&&) = delete;

    ::std::size_t run_one() { return this->_Context.run_one(); }
    ::std::size_t run()
    {
        ::std::size_t _Count{};
        while (::std::size_t _C = this->run_one())
        {
            _Count += _C;
        }
        return _Count;
    }
};

// ----------------------------------------------------------------------------

#endif
