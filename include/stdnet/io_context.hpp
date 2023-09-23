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
#include <unistd.h>
#include <limits>
#include <cerrno>

// ----------------------------------------------------------------------------

namespace stdnet
{
    namespace _Hidden
    {
        enum _Socket_id: ::std::uint_least32_t
        {
            _Invalid = ::std::numeric_limits<::std::uint_least32_t>::max()
        };

        struct _Io_base
        {
            virtual void _Complete(void*) = 0;
        };
        template <typename _Data>
        struct _Io_operation
            : _Io_base
            , _Data
        {
            template <typename _D = _Data>
            _Io_operation(_D&& _A = _Data())
                : _Io_base()
                , _Data(::std::forward<_D>(_A))
            {
            }
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
            auto operator[](::stdnet::_Hidden::_Socket_id _Id) -> _Record&
            {
                return ::std::get<1>(this->_Records[::std::size_t(_Id)]);
            }
        };
    }
    namespace _Hidden_abstract
    {
        struct _Context
        {
            virtual ~_Context() = default;
            virtual auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id = 0;
            virtual auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void = 0;
            virtual auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type = 0;

            virtual auto run_one() -> ::std::size_t = 0;

            virtual auto _Accept(::stdnet::_Hidden::_Socket_id,
                                 ::stdnet::_Hidden::_Io_operation<::stdnet::ip::tcp::endpoint>*) -> void = 0;
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

            auto _Make_socket(int _D, int _T, int _P, ::std::error_code& _Error)
                -> ::stdnet::_Hidden::_Socket_id override final
            {
                int _Fd(::socket(_D, _T, _P));
                if (_Fd < 0)
                {
                    _Error = ::std::error_code(errno, ::std::system_category());
                    return ::stdnet::_Hidden::_Socket_id::_Invalid;
                }
                return this->_D_sockets._Insert(_Fd);
            }
            auto _Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void override final
            {
                _Stdnet_native_handle_type _Handle(this->_D_sockets[_Id]._Handle);
                this->_D_sockets._Erase(_Id);
                if (::close(_Handle) < 0)
                {
                    _Error = ::std::error_code(errno, ::std::system_category());
                }
            }
            auto _Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type override final
            {
                return this->_D_sockets[_Id]._Handle;
            }

            auto run_one() -> ::std::size_t override final
            {
                return ::std::size_t{};
            }

            auto _Accept(::stdnet::_Hidden::_Socket_id,
                         ::stdnet::_Hidden::_Io_operation<::stdnet::ip::tcp::endpoint>*)
                -> void override final
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
    ::std::unique_ptr<::stdnet::_Hidden_abstract::_Context> _D_owned{new ::stdnet::_Hidden_poll::_Context()};
    ::stdnet::_Hidden_abstract::_Context&                   _D_context{*this->_D_owned};

public:
    class scheduler_type
    {
        friend class io_context;
    private:
        ::stdnet::_Hidden_abstract::_Context* _D_context;
        scheduler_type(::stdnet::_Hidden_abstract::_Context* _Context)
            : _D_context(_Context)
        {
        }

    public:
        auto _Accept(::stdnet::_Hidden::_Socket_id _Id,
                     ::stdnet::_Hidden::_Io_operation<::stdnet::ip::tcp::endpoint>* _Op) -> void
        {
            this->_D_context->_Accept(_Id, _Op);
        }
       
    };
    class executor_type {};

    io_context() = default;
    io_context(::stdnet::_Hidden_abstract::_Context& _Context): _D_owned(), _D_context(_Context) {}
    io_context(io_context&&) = delete;

    auto _Make_socket(int _D, int _T, int _P, ::std::error_code& _Error) -> ::stdnet::_Hidden::_Socket_id
    {
        return this->_D_context._Make_socket(_D, _T, _P, _Error);
    }
    auto _Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void
    {
        return this->_D_context._Release(_Id, _Error);
    }
    auto _Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type
    {
        return this->_D_context._Native_handle(_Id);
    }
    auto get_scheduler() -> scheduler_type { return scheduler_type(&this->_D_context); }

    ::std::size_t run_one() { return this->_D_context.run_one(); }
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
