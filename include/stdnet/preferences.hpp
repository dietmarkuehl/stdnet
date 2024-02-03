// stdnet/preferences.hpp                                             -*-C++-*-
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

#ifndef INCLUDED_STDNET_PREFERENCES
#define INCLUDED_STDNET_PREFERENCES

#include "stdnet/properties.hpp"

// ----------------------------------------------------------------------------

namespace stdnet
{
    enum class preference
    {
        prohibit,
        avoid,
        no_preference,
        prefer,
        require
    };
    ::std::ostream& operator<< (std::ostream&, preference);

    enum class multipath_type
    {
        disabled,
        active,
        passive
    };
    ::std::ostream& operator<< (std::ostream&, multipath_type);

    enum class direction_type
    {
        bidirectional,
        unidirectional_send,
        unidirectional_receive
    };
    ::std::ostream& operator<< (std::ostream&, direction_type);

    enum class security_protocol
    {
        tls_1_2,
        tls_1_3
        };
    ::std::ostream& operator<< (std::ostream&, security_protocol);

    enum class conn_capacity_profile
    {
        best_effort, // default
        scavenger,
        low_latency_interactive,
        low_latency_non_interactive,
        constant_rate_streaming,
        capacity_seeking
    };
    ::std::ostream& operator<< (std::ostream&, conn_capacity_profile);

    enum class group_type
    {
    };

    enum class cipher_suite_type
    {
    };

    enum class signature_algorithm_type
    {
    };

    // ------------------------------------------------------------------------

}
#include "stdnet/properties_def.hpp"

namespace stdnet
{
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

inline ::std::ostream& stdnet::operator<< (std::ostream& _Out, ::stdnet::multipath_type _T)
{
    switch (_T)
    {
    default: return _Out << "<unknown>";
    case ::stdnet::multipath_type::active: return _Out << "active";
    case ::stdnet::multipath_type::passive: return _Out << "passive";
    case ::stdnet::multipath_type::disabled: return _Out << "disabled";
    }
}

inline ::std::ostream& stdnet::operator<< (std::ostream& _Out, ::stdnet::direction_type _T)
{
    switch (_T)
    {
    default: return _Out << "<unknown>";
    case ::stdnet::direction_type::bidirectional: return _Out << "bidirectional";
    case ::stdnet::direction_type::unidirectional_send: return _Out << "unidirectional_send";
    case ::stdnet::direction_type::unidirectional_receive: return _Out << "unidirectional_receive";
    }
}

inline ::std::ostream& stdnet::operator<< (std::ostream& _Out, stdnet::security_protocol _P)
{
    switch (_P)
    {
    default: return _Out << "<unknown>";
    case ::stdnet::security_protocol::tls_1_2: return _Out << "tls_1_2";
    case ::stdnet::security_protocol::tls_1_3: return _Out << "tls_1_3";
    }
}

inline ::std::ostream& stdnet::operator<< (std::ostream& _Out, stdnet::conn_capacity_profile _P)
{
    switch (_P)
    {
    default: return _Out << "<unknown>";
    case ::stdnet::conn_capacity_profile::best_effort: return _Out << "best_effort";
    case ::stdnet::conn_capacity_profile::scavenger: return _Out << "scavenger";
    case ::stdnet::conn_capacity_profile::low_latency_interactive: return _Out << "low_latency_interactive";
    case ::stdnet::conn_capacity_profile::low_latency_non_interactive: return _Out << "low_latency_non_interactive";
    case ::stdnet::conn_capacity_profile::constant_rate_streaming: return _Out << "constant_rate_streaming";
    case ::stdnet::conn_capacity_profile::capacity_seeking: return _Out << "capacity_seeking";
    }
}

// ----------------------------------------------------------------------------

#endif
