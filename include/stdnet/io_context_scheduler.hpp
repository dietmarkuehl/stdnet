// stdnet/io_context_scheduler.hpp                                    -*-C++-*-
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

#ifndef INCLUDED_STDNET_IO_CONTEXT_SCHEDULER
#define INCLUDED_STDNET_IO_CONTEXT_SCHEDULER

#include <stdnet/context_base.hpp>
#include <cassert>

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
        assert(this->_D_context);
    }

    auto _Get_context() const { return this->_D_context; }

    auto _Cancel(_Hidden::_Io_base* _Cancel_op, _Hidden::_Io_base* _Op) -> void
    {
        this->_D_context->_Cancel(_Cancel_op, _Op);
    }
    auto _Accept(_Hidden::_Context_base::_Accept_operation* _Op) -> bool
    {
        return this->_D_context->_Accept(_Op);
    }
    auto _Connect(_Hidden::_Context_base::_Connect_operation* _Op) -> bool
    {
        return this->_D_context->_Connect(_Op);
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
