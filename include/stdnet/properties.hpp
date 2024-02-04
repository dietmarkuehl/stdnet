// stdnet/properties.hpp                                              -*-C++-*-
// ----------------------------------------------------------------------------
//  Copyright (C) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify,
//  merge, publish, distribute, sublicense, and/or sell copies of
//  the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_PROPERTIES
#define INCLUDED_STDNET_PROPERTIES

#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

// ----------------------------------------------------------------------------

namespace stdnet
{
    struct _Empty
    {
        template <typename _T>
        operator _T() const { return {}; }
    };

    template <char const* _Name, auto _Default, typename _Type = decltype(_Default)>
    struct property
    {
        using value_type = _Type;
        static constexpr char const* name = _Name;
        static constexpr auto get_default() { return _Default; }
        friend ::std::ostream& operator<< (::std::ostream& _Out, property const&)
        {
            if constexpr (::std::same_as<::stdnet::_Empty, decltype(_Default)>)
            {
                return _Out << _Name << "(<empty>)";
            }
            else
            {
                return _Out << _Name << "(" << _Default << ")";
            }
        }
    };

    template <typename, typename, typename...> struct _MapKey;
    template <typename _K>
    struct _MapKey<_K, ::std::index_sequence<>>;
    template <typename _K, ::std::size_t _I0, ::std::size_t... _I, typename _T0, typename... _T>
    struct _MapKey<_K, ::std::index_sequence<_I0, _I...>, _T0, _T...>
    {
        static constexpr ::std::size_t value()
        {
            if constexpr (_K::name == _T0::name)
                return _I0;
            else
                return _MapKey<_K, ::std::index_sequence<_I...>, _T...>::value();
        }
    };

    template <typename, typename...> struct _GetKey;
    template <>
    struct _GetKey<::std::index_sequence<>>
    {
        static ::std::any get(::std::any&, ::std::string_view, auto&&) { return {}; }
    };
    template <::std::size_t _I0, ::std::size_t... _I, typename _T0, typename... _T>
    struct _GetKey<::std::index_sequence<_I0, _I...>, _T0, _T...>
    {
        static void get(::std::any& _Rc, ::std::string_view _Key, auto&& _V)
        {
            if (_Key == _T0::name)
                _Rc = ::std::get<_I0>(_V);
            else
                _GetKey<::std::index_sequence<_I...>, _T...>::get(_Rc, _Key, _V);
        }
    };

    template <typename... _P>
    class properties
    {
    public:
        ::std::tuple<typename _P::value_type...> _Values;
        properties()
            : _Values(_P::get_default()...)
        {
        }

        template <typename _Key>
        auto get(_Key const&)
        {
            return ::std::get<_MapKey<_Key, ::std::index_sequence_for<_P...>, _P...>::value()>(this->_Values);
        }
        template <typename _Key, typename _Value>
        auto set(_Key const&, _Value const& _V)
        {
            ::std::get<_MapKey<_Key, ::std::index_sequence_for<_P...>, _P...>::value()>(this->_Values) = _V;
        }
        template <typename _Key, typename _Value>
        auto push_back(_Key const&, _Value const& _V)
        {
            ::std::get<_MapKey<_Key, ::std::index_sequence_for<_P...>, _P...>::value()>(this->_Values).push_back(_V);
        }
        ::std::any get(char const* _Key)
        {
            return this->get(::std::string_view(_Key));
        }
        ::std::any get(::std::string_view _Key)
        {
            ::std::any _Rc;
            _GetKey<::std::index_sequence_for<_P...>, _P...>::get(_Rc, _Key, this->_Values);
            return _Rc;
        }
        friend ::std::ostream& operator<< (::std::ostream& _Out, properties _V)
        {
            ::std::apply([&_Out](auto&& ... _X){
                (::std::invoke([&_Out](auto&& _Pv, auto&& _Vv){
                    if constexpr (requires(::std::ostream& _Out, decltype(_Vv) _Vv){ _Out << _Vv; })
                    {
                        _Out << _Pv << "=" << std::boolalpha << _Vv << ", ";
                    }
                    else if constexpr (requires(decltype(_Vv) _Vv){ ::std::begin(_Vv); ::std::end(_Vv); })
                    {
                        _Out << _Pv << "=[";
                        auto _It(::std::begin(_Vv));
                        auto _End(::std::end(_Vv));
                        if (_It != _End)
                        {
                            _Out << *_It;
                            while (++_It != _End)
                            {
                                _Out << ", " << *_It;
                            }
                        }
                        _Out << "], ";
                    }
                    else
                    {
                        _Out << _Pv << "=<not-printable>, ";
                    }
                }, _P{}, _X), ...);
            }, _V._Values);
            return _Out;
        }
    };

}

// ----------------------------------------------------------------------------

#endif
