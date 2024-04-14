// stdnet/ring_buffer.hpp                                             -*-C++-*-
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

#ifndef INCLUDED_STDNET_RING_BUFFER
#define INCLUDED_STDNET_RING_BUFFER

#include <stdexec/execution.hpp>
#include <ostream>
#include <span>
#include <utility>
#include <cstddef>
#include <cstdint>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    template <std::uint64_t, typename = char> class _Ring_buffer;
}

// ----------------------------------------------------------------------------

template <std::uint64_t _Size, typename _T>
class stdnet::_Hidden::_Ring_buffer
{
public:
    enum class _Side_t { _Producer = 0, _Consumer = 1 };
    template <_Side_t _Side>
    class _Result
    {
    private:
        static constexpr _Side_t _Other_side{_Side == _Side_t::_Producer? _Side_t::_Consumer: _Side_t::_Producer};
        _Ring_buffer* _D_ring;
    public:
        ::std::span<_T> const _D_buffer;

        _Result(_Ring_buffer* _Ring, ::std::span<_T> _Buffer)
            : _D_ring(_Ring)
            , _D_buffer(_Buffer)
        {
        }
        auto data() -> _T*  { return this->_D_buffer.data(); }
        auto size() const -> ::std::size_t  { return this->_D_buffer.size(); }
        auto empty() const -> bool { return this->_D_buffer.empty(); }
        auto _Advance(std::uint64_t _N) -> void
        {
            this->_D_ring->pos[int(_Side)] += _N;
            if (this->_D_ring->_Awaiting[int(_Other_side)] && 0 < _N)
            {
                std::exchange(this->_Ring->_Awaiting[int(_Other_side)], nullptr)->_Complete();
            }
        }
    };
    struct _State_base
    {
        virtual auto _Complete() -> void = 0;
        virtual auto _Stop() -> void = 0;
    };
    template <_Side_t _Side, typename _Receiver>
    struct _State final
        : _State_base
    {
        struct _Stop_callback
        {
            _State_base* _D_obj;
            auto operator()() -> void { this->_D_obj->_Stop(); }
        };
        using _Stop_token_t = decltype(::stdexec::get_stop_token(::stdexec::get_env(std::declval<_Receiver>())));
        using _Callback_t = typename _Stop_token_t::template callback_type<_Stop_callback>;

        _Ring_buffer*                  _D_ring;
        std::remove_cvref_t<_Receiver> _D_receiver;
        std::optional<_Callback_t>     _D_callback;
        template <typename _R_t>
        _State(_Ring_buffer* _Ring, _R_t&& _R)
            : _D_ring(_Ring)
            , _D_receiver(std::forward<_R_t>(_R))
        {
        }
        friend auto tag_invoke(::stdexec::start_t, _State& _Self) noexcept -> void
        {
            std::uint64_t _Diff{_Self._D_ring->_D_pos[int(_Side_t::_Producer)] - _Self._D_ring->_D_pos[int(_Side_t::_Consumer)]};
            std::uint64_t _Sz{_Side == _Side_t::_Producer? _Size - _Diff: _Diff};
            if (0u < _Sz)
            {
                _Self._Complete();
            }
            else
            {
                _Self._D_ring->_Awaiting[int(_Side)] = &_Self;
                _Self._D_callback.emplace(::stdexec::get_stop_token(::stdexec::get_env(_Self._D_receiver), _Stop_callback{&_Self}));
            }
        }
        auto _Complete() -> void override final
        {
            std::uint64_t _Diff{this->_D_ring->_D_pos[int(_Side_t::_Producer)] - this->_D_ring->_D_pos[int(_Side_t::_Consumer)]};
            std::uint64_t _Sz{_Side == _Side_t::_Producer? _Size - _Diff: _Diff};
            std::uint64_t _Offset{this->_D_ring->_D_pos[int(_Side)] % _Size};
            _D_callback.reset();
            ::stdexec::set_value(std::move(this->_D_receiver),
                                 _Result<_Side>{this->_D_ring, std::span<_T>(this->_D_ring->_D_buffer + _Offset, std::min(_Size - _Offset, _Sz))});
        }
        auto _Stop() -> void override final
        {
            ::stdexec::set_stopped(std::move(this->_D_receiver));
        }
    };
    template <_Side_t _Side>
    struct _Sender
    {
        using _Result_t = _Result<_Side>;

        _Ring_buffer* _D_ring;

        template <typename _Receiver_t>
        friend auto tag_invoke(::stdexec::connect_t, _Sender const& _Self, _Receiver_t&& _Receiver)
            -> _State<_Side, _Receiver_t>
        {
            return _State<_Side, _Receiver_t>( _Self._D_ring, std::forward<_Receiver_t>(_Receiver) );
        }
    };

private:
    _T             _D_buffer[_Size];
    std::uint64_t  _D_pos[2]{};
    _State_base*   _D_awaiting[2]{};

public:
    auto _Produce() -> _Sender<_Side_t::_Producer>
    { 
        return _Sender<_Side_t::_Producer>{this};
    }
    auto _Consume() -> _Sender<_Side_t::_Consumer>
    {
        return _Sender<_Side_t::_Consumer>{this};
    }

    friend auto operator<< (std::ostream& _Out, _Ring_buffer const& _Ring) -> ::std::ostream&
    {
        return _Out << &_Ring << " "
                    << "prod=" << _Ring._D_pos[int(_Side_t::_Producer)] << " "
                    << "cons=" << _Ring._D_pos[int(_Side_t::_Consumer)] << "\n";
    }
};

// ----------------------------------------------------------------------------

#endif
