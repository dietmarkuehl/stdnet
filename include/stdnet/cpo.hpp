// stdnet/cpo.hpp                                                     -*-C++-*-
// ----------------------------------------------------------------------------
//
//  Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Licensed under the Apache License Version 2.0 with LLVM Exceptions
//  (the "License"); you may not use this file except in compliance with
//  the License. You may obtain a copy of the License at
//
//    https://llvm.org/LICENSE.txt
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_CPO
#define INCLUDED_STDNET_CPO

#include <stdnet/io_base.hpp>
#include <stdexec/concepts.hpp>
#include <stdexec/execution.hpp>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    template <typename> struct _Cpo;
}

// ----------------------------------------------------------------------------

template <::stdexec::receiver _Receiver>
struct _Cpo_State_base
{
    _Receiver           _D_receiver;
    ::std::atomic<int>  _D_outstanding{};
    template <::stdexec::receiver _RT>
    _Cpo_State_base(_RT&& _R)
        : _D_receiver(::std::forward<_RT>(_R))
    {
    }
    virtual auto _Start() -> void = 0;
};

template <::stdexec::receiver _Receiver>
struct _Upstream_receiver
{
    using is_receiver = void;
    _Cpo_State_base<_Receiver>* _D_state;
    friend auto tag_invoke(::stdexec::set_value_t, _Upstream_receiver&& _Self) noexcept -> void
    {
        _Self._D_state->_Start();
    }
    template <typename _Error>
    friend auto tag_invoke(::stdexec::set_error_t, _Upstream_receiver&& _Self, _Error&& _E) -> void
    {
        ::stdexec::set_error(::std::move(_Self._D_state->_D_receiver), ::std::forward<_Error>(_E));
    }
    friend auto tag_invoke(::stdexec::set_stopped_t, _Upstream_receiver&& _Self) -> void
    {
        ::stdexec::set_stopped(::std::move(_Self._D_state->_D_receiver));
    }
    friend auto tag_invoke(::stdexec::get_env_t, _Upstream_receiver const& _Self) noexcept
    {
        return ::stdexec::get_env(_Self._D_state->_D_receiver);
    }
};

template <typename _Desc, typename _Data, ::stdexec::receiver _Receiver, ::stdexec::sender _Upstream>
struct _Cpo_State
    : _Desc::_Operation
    , _Cpo_State_base<_Receiver>
{
    struct _Cancel_callback
        : ::stdnet::_Hidden::_Io_base //-dk:TODO use a less heavy-weight base
    {
        _Cpo_State* _D_state;
        _Cancel_callback(_Cpo_State* _S)
            : ::stdnet::_Hidden::_Io_base(::stdnet::_Hidden::_Socket_id(), 0)
            , _D_state(_S)
        {
        }
        auto operator()()
        {
            if (1 < ++this->_D_state->_D_outstanding)
            {
                this->_D_state->_D_data._Get_scheduler()._Cancel(this, this->_D_state);
            }
        }
        auto _Complete() -> void override final
        {
            if (0u == --this->_D_state->_D_outstanding)
            {
                ::stdexec::set_stopped(::std::move(this->_D_state->_D_receiver));
            }
        }
        auto _Error(::std::error_code) -> void override final
        {
            this->_Complete();
        }
        auto _Cancel() -> void override final
        {
            this->_Complete();
        }
    };
    using _Upstream_state_t = decltype(::stdexec::connect(::std::declval<_Upstream&>(), ::std::declval<_Upstream_receiver<_Receiver>>()));
    using _Stop_token = std::remove_cvref_t<decltype(::stdexec::get_stop_token(::stdexec::get_env(::std::declval<_Receiver const&>())))>;
    using _Callback = typename _Stop_token::template callback_type<_Cancel_callback>;

    _Data                      _D_data;
    _Upstream_state_t          _D_state;
    ::std::optional<_Callback> _D_callback;

    template <typename _DT, ::stdexec::receiver _RT>
    _Cpo_State(_DT&& _D, _RT&& _R, _Upstream _Up)
        : _Desc::_Operation(_D._Id(), _D._Events())
        , _Cpo_State_base<_Receiver>(::std::forward<_RT>(_R))
        , _D_data(::std::forward<_DT>(_D))
        , _D_state(::stdexec::connect(_Up, _Upstream_receiver<_Receiver>{this}))
    {
    }
    friend auto tag_invoke(::stdexec::start_t, _Cpo_State& _Self) noexcept -> void
    {
        ::stdexec::start(_Self._D_state);
    }
    auto _Start() -> void override final
    {
        auto _Token(::stdexec::get_stop_token(::stdexec::get_env(this->_D_receiver)));
        ++this->_D_outstanding;
        this->_D_callback.emplace(_Token, _Cancel_callback(this));
        if (_Token.stop_requested())
        {
            this->_D_callback.reset();
            this->_Cancel();
            return;
        }
        if (!this->_D_data._Submit(this))
        {
            this->_Complete();
        }
    }
    auto _Complete() -> void override final
    {
        _D_callback.reset();
        if (0 == --this->_D_outstanding)
        {
            this->_D_data._Set_value(*this, ::std::move(this->_D_receiver));
        }
    }
    auto _Error(::std::error_code _Err) -> void override final
    {
        _D_callback.reset();
        if (0 == --this->_D_outstanding)
        {
            ::stdexec::set_error(::std::move(this->_D_receiver), std::move(_Err));
        }
    }
    auto _Cancel() -> void override final
    {
        if (0 == --this->_D_outstanding)
        {
            ::stdexec::set_stopped(::std::move(this->_D_receiver));
        }
    }
};
template <typename _Desc, typename _Data, ::stdexec::sender _Upstream>
struct _Cpo_Sender
{
    using is_sender = void;
    friend auto tag_invoke(stdexec::get_completion_signatures_t, _Cpo_Sender const&, auto) noexcept {
        return ::stdexec::completion_signatures<
            typename _Data::_Completion_signature,
            ::stdexec::set_error_t(::std::error_code), //-dk:TODO merge with _Upstream errors
            ::stdexec::set_stopped_t()
            >();
    }
    _Data     _D_data;
    _Upstream _D_upstream;

    template <::stdexec::receiver _Receiver>
    friend auto tag_invoke(::stdexec::connect_t, _Cpo_Sender const& _Self, _Receiver&& _R)
    {
        return _Cpo_State<_Desc, _Data, ::std::remove_cvref_t<_Receiver>, _Upstream>(
            _Self._D_data,
            ::std::forward<_Receiver>(_R),
            _Self._D_upstream
            );
    }
    template <::stdexec::receiver _Receiver>
    friend auto tag_invoke(::stdexec::connect_t, _Cpo_Sender&& _Self, _Receiver&& _R)
    {
        return _Cpo_State<_Desc, _Data, ::std::remove_cvref_t<_Receiver>, _Upstream>(
            ::std::move(_Self._D_data),
            ::std::forward<_Receiver>(_R),
            ::std::move(_Self._D_upstream)
            );
    }
};

template <typename _Desc>
struct stdnet::_Hidden::_Cpo
{
    template <::stdexec::sender _Upstream, typename... _Args_t>
    friend auto tag_invoke(_Cpo const&, _Upstream&& _U, _Args_t&&... _Args)
    {
        using _Data = _Desc::template _Data<_Args_t...>;
        return _Cpo_Sender<_Desc, _Data, ::std::remove_cvref_t<_Upstream>>{{_Data{::std::forward<_Args_t>(_Args)...}}, ::std::forward<_Upstream>(_U)};
    }

    template <typename _Arg0_t, typename... _Args_t>
        requires (!::stdexec::sender<::std::remove_cvref_t<_Arg0_t>>)
            &&   ::stdexec::tag_invocable<_Cpo const, decltype(::stdexec::just()), _Arg0_t, _Args_t...>
    auto operator()(_Arg0_t&& _Arg0, _Args_t&&... _Args) const
    {
        return tag_invoke(*this, ::stdexec::just(), ::std::forward<_Arg0_t>(_Arg0), ::std::forward<_Args_t>(_Args)...);
    }
    template <::stdexec::sender _Upstream, typename... _Args_t>
        requires ::stdexec::tag_invocable<_Cpo const, _Upstream, _Args_t...>
    auto operator()(_Upstream&& _U, _Args_t&&... _Args) const
    {
        return tag_invoke(*this, ::std::forward<_Upstream>(_U), ::std::forward<_Args_t>(_Args)...);
    }
};

// ----------------------------------------------------------------------------

#endif
