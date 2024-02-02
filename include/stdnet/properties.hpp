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
    enum class preference { prohibit, avoid, no_preference, prefer, require };
    ::std::ostream& operator<< (std::ostream&, preference);

    template <char const* _Name, auto _Default, typename _Type = decltype(_Default)>
    struct property
    {
        using value_type = _Type;
        static constexpr char const* name = _Name;
        static constexpr auto get_default() { return _Default; }
        friend ::std::ostream& operator<< (::std::ostream& _Out, property const&)
        {
            return _Out << "prop(" << _Name << ", default=" << _Default << ")";
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
    };

    namespace _Names
    {
        inline constexpr char reliability[] = "reliability";
        inline constexpr char preserve_msg_boundaries[] = "preserve_msg_boundaries";
        inline constexpr char per_msg_reliability[] = "per_msg_reliability";
        inline constexpr char preserve_order[] = "preserve_order";
        inline constexpr char zero_rtt_msg[] = "zero_rtt_msg";
        inline constexpr char multistreaming[] = "multistreaming";
        inline constexpr char full_checksum_send[] = "full_checksum_send";
        inline constexpr char full_checksum_recv[] = "full_checksum_recv";
        inline constexpr char congestion_control[] = "congestion_control";
        inline constexpr char keep_alive[] = "keep_alive";

        inline constexpr char interface[] = "interface";
    }

    using reliability_t = property<
        ::stdnet::_Names::reliability,
        ::stdnet::preference::require
        >;
    inline constexpr reliability_t reliability{};

    using preserve_msg_boundaries_t = ::stdnet::property<
        ::stdnet::_Names::preserve_msg_boundaries,
        ::stdnet::preference::no_preference
        >;
    inline constexpr preserve_msg_boundaries_t preserve_msg_boundaries{};

    using interface_t = property<
        ::stdnet::_Names::interface,
        ::stdnet::_Empty{},
        ::std::vector<::std::pair<::stdnet::preference, ::std::string>>
        >;
    inline constexpr interface_t interface{};

    using transport_properties
        = properties<
        reliability_t,
        preserve_msg_boundaries_t,
        interface_t
        >;
}

// ----------------------------------------------------------------------------

inline ::std::ostream& stdnet::operator<< (std::ostream& _Out, ::stdnet::preference _P)
{
    switch (_P)
    {
    default: return _Out << "<unknown>";
    case ::stdnet::preference::require: return _Out << "require";
    case ::stdnet::preference::avoid: return _Out << "avoid";
    case ::stdnet::preference::no_preference: return _Out << "no_preference";
    case ::stdnet::preference::prefer: return _Out << "prefer";
    case ::stdnet::preference::prohibit: return _Out << "prohibit";
    }
}

// ----------------------------------------------------------------------------

#endif
