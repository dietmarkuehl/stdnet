// stdnet/io_context_scheduler.hpp                                    -*-C++-*-
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

#ifndef INCLUDED_STDNET_IO_CONTEXT_SCHEDULER
#define INCLUDED_STDNET_IO_CONTEXT_SCHEDULER

#include <stdnet/context_base.hpp>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    class _Io_context_scheduler;
}

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Io_context_scheduler
{
private:
    ::stdnet::_Hidden::_Context_base* _D_context;

public:
    _Io_context_scheduler(::stdnet::_Hidden::_Context_base* _Context)
        : _D_context(_Context)
    {
    }

    auto _Cancel(_Hidden::_Io_base* _Op) -> void
    {
        this->_D_context->_Cancel(_Op);
    }
    auto _Accept(::stdnet::_Hidden::_Socket_id _Id, _Hidden::_Context_base::_Accept_operation* _Op) -> bool
    {
        return this->_D_context->_Accept(_Op);
    }
    auto _Receive(_Hidden::_Context_base::_Receive_operation* _Op) -> bool
    {
        return this->_D_context->_Receive(_Op);
    }
    auto _Send(_Hidden::_Context_base::_Send_operation* _Op) -> bool
    {
        return this->_D_context->_Send(_Op);
    }
};

// ----------------------------------------------------------------------------

#endif
