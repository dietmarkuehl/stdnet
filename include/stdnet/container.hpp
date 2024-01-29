// container.hpp                                                      -*-C++-*-
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

#ifndef INCLUDED_CONTAINER
#define INCLUDED_CONTAINER

#include <algorithm>
#include <functional>
#include <mutex>
#include <utility>
#include <cassert> //-dk:TODO remove
#include <ostream>
#include <iostream> //-dk:TODO remove
#include <queue> //-dk:TODO remove

// ----------------------------------------------------------------------------
// _Intrusive_list<Node> list
// access:
//     list.empty()
// get content:
//     Node& node{list.front()};
//     list.pop_front();
// ----------------------------------------------------------------------------
// _Intrusive_queue<Node> queue
// insert (note: the node needs to stay alive):
//     Node node{...};
//     queue._Push(node);
// get content:
//     auto list = queue._Extract();
// ----------------------------------------------------------------------------

namespace stdnet
{
    inline auto _TopBit(::std::size_t _V) -> ::std::size_t;

    template <typename _T>
    using _Default_link = decltype([](_T& _N)->_T& { return _N; });
    using _Default_wakeup = decltype([]{});

    template <typename>
        struct _Intrusive_node;
    template <typename _T, typename = ::stdnet::_Default_link<_T>>
        class _Intrusive_list;
    template <typename _T,
              typename = ::stdnet::_Default_wakeup,
              typename = ::stdnet::_Default_link<_T>
             >
        class _Intrusive_queue;

    template <typename _It, typename _Comp = ::std::less<>>
    auto remove_heap(_It _I, _It _Begin, _It _End, _Comp _C = {}) -> void;

    template <typename>
        struct _Intrusive_tree_node;
    template <typename _T, typename = ::std::less<>>
        class _Intrusive_priority_queue;
}

// ----------------------------------------------------------------------------

inline auto stdnet::_TopBit(::std::size_t _V) -> ::std::size_t
{
    ::std::size_t _R{1u};
    for (; _V >>= 1; _R <<= 1)
        ;
    return _R >> 1;
}

// ----------------------------------------------------------------------------

template <typename _T>
struct stdnet::_Intrusive_node
{
    _T* _D_next{};
    _Intrusive_node() = default;
    _Intrusive_node(_Intrusive_node&&) = delete;
};

// ----------------------------------------------------------------------------

template <typename _T, typename _Link>
class stdnet::_Intrusive_list
    : private _Link
{
    _T* _D_head{};
    _T* _D_tail{};
public:
    _Intrusive_list() = default;
    _Intrusive_list(_Intrusive_list&& _Other)
        : _D_head(::std::exchange(_Other._D_head, nullptr))
        , _D_tail(::std::exchange(_Other._D_tail, nullptr))
    {
    }
    auto operator= (_Intrusive_list&& _Other) -> _Intrusive_list&
    {
        this->_D_head = ::std::exchange(_Other._D_head, nullptr);
        this->_D_tail = ::std::exchange(_Other._D_tail, nullptr);
        return *this;
    }
    auto swap(_Intrusive_list& _Other) -> void
    {
        ::std::swap(this->_D_head, _Other._D_head);
        ::std::swap(this->_D_tail, _Other._D_tail);
    }

    auto _Head_next() -> _T*& { return (*this)(*this->_D_head)._D_next; }
    auto _Tail_next() -> _T*& { return (*this)(*this->_D_tail)._D_next; }
    auto empty() const -> bool { return this->_D_head == nullptr; }
    auto push_back(_T& _N) -> void
    {
        this->_D_tail = (this->empty()? this->_D_head: this->_Tail_next()) = &_N;
    }
    auto front() -> _T& { return *this->_D_head; }
    auto pop_front() -> void { this->_D_head = this->_Head_next(); }
};

// ----------------------------------------------------------------------------

template <typename _T, typename _Wakeup, typename _Link>
class stdnet::_Intrusive_queue
    : private _Wakeup
{
public:
    using _List_type = _Intrusive_list<_T, _Link>;

private:
    ::std::mutex _Mutex;
    _List_type   _List;

public:
    template <typename... _Args>
    _Intrusive_queue(_Args&&... _A)
        : _Wakeup(::std::forward<_Args>(_A)...)
    {
    }
    auto _Push(_T& _N) -> void
    {
        bool _Empty(false);
        {
            ::std::lock_guard kerberos(this->_Mutex);
            _Empty = (this->_List.empty());
            this->_List.push_back(_N);
        }
        if (_Empty)
        {
            (*this)(); // wake up
        }
    }
    auto _Extract() -> _List_type
    {
        ::std::lock_guard kerberos(this->_Mutex);
        return ::std::move(this->_List);
    }
};

// ----------------------------------------------------------------------------
//-dk:TODO remove?

template <typename _It, typename _Comp>
auto stdnet::remove_heap(_It _I, _It _Begin, _It _End, _Comp _C) -> void
{
    if (_I != --_End)
    {
        using namespace std;
        auto _D{_I - _Begin};
        while (0 < _D)
        {
            auto _N{(_D - 1) / 2};
            swap(_Begin[_D], _Begin[_N]);
            _D = _N;
        }
        swap(_Begin[_D], *_End);
        auto _S{_End - _Begin};
        while ((_D * 2 + 2 < _S))
        {
            auto _N{_D * 2 + 2};
            if (_C(_Begin[_N], _Begin[_N - 1]))
            {
                --_N;
            }
            if (_C(_Begin[_D], _Begin[_N]))
            {
                swap(_Begin[_D], _Begin[_N]);
                _D = _N;
            }
            else
            {
                return;
            }
        }
        if (_D * 2 + 1 < _S && _C(_Begin[_D], _Begin[_D * 2 + 1]))
        {
            swap(_Begin[_D], _Begin[_D * 2 + 1]);
        }
    }
}

// ----------------------------------------------------------------------------

template <typename _T>
struct stdnet::_Intrusive_tree_node
{
    _T                                  _D_value{};
    ::stdnet::_Intrusive_tree_node<_T>* _D_parent{nullptr};
    ::stdnet::_Intrusive_tree_node<_T>* _D_left{nullptr};
    ::stdnet::_Intrusive_tree_node<_T>* _D_right{nullptr};
};

template <typename _T, typename _Cmp>
class stdnet::_Intrusive_priority_queue
{
private:
    using _Node_t = ::stdnet::_Intrusive_tree_node<_T>;

    _Node_t*      _D_root{nullptr};
    ::std::size_t _D_size{0u};
    _Cmp          _D_cmp;

    static auto _High_index(::std::size_t _Size) -> ::std::size_t;

    auto _Swap_nodes(_Node_t* _P, _Node_t* _N) -> void;
    auto _Sift_up(::stdnet::_Intrusive_tree_node<_T>* _N) -> void;
    auto _Print(::std::ostream& _Out) -> void
    {
        ::std::queue<_Node_t*> _Q;
        _Q.push(this->_D_root);
        _Q.push(nullptr);
        while (!_Q.empty())
        {
            auto _N(_Q.front());
            _Q.pop();
            if (_N)
            {
                _Out << _N->_D_value << " ";
                if (_N->_D_left)
                {
                    _Q.push(_N->_D_left);
                }
                if (_N->_D_right)
                {
                    _Q.push(_N->_D_right);
                }
            }
            else
            {
                _Out << '\n';
                if (!_Q.empty())
                {
                    _Q.push(nullptr);
                }
            }
        }
        _Out << "------\n";
    }

public:
    template <typename... _A>
    explicit _Intrusive_priority_queue(_A&&...);
    auto empty() const -> bool { return _D_root == nullptr; }
    auto size() const -> ::std::size_t { return _D_size; }
    auto push(::stdnet::_Intrusive_tree_node<_T>& _N) -> void;
    auto top() -> ::stdnet::_Intrusive_tree_node<_T>& { return *this->_D_root; }
    auto pop() -> ::stdnet::_Intrusive_tree_node<_T>&;
    auto erase(::stdnet::_Intrusive_tree_node<_T>& _N) -> void;
};

template <typename _T, typename _Cmp>
    template <typename... _A>
inline stdnet::_Intrusive_priority_queue<_T, _Cmp>::_Intrusive_priority_queue(_A&&... _P)
    : _D_cmp(::std::forward<_A>(_P)...)
{
}

template <typename _T, typename _Cmp>
inline auto
stdnet::_Intrusive_priority_queue<_T, _Cmp>::_High_index(std::size_t _Size)
    -> ::std::size_t
{
    ::std::size_t _Rc(0);
    for (; _Size; _Size >>= 1) {
        ++_Rc;
    }
    return _Rc;
}

template <typename _T, typename _Cmp>
inline auto
stdnet::_Intrusive_priority_queue<_T, _Cmp>::_Swap_nodes(_Node_t* _P, _Node_t* _N)
    -> void
{
    assert(_P == _N->_D_parent);
    auto _Gp(_P->_D_parent);
    _N->_D_parent = _Gp;
    if (_Gp)
    {
        (_Gp->_D_left == _P? _Gp->_D_left: _Gp->_D_right) = _N;
    }
    _P->_D_parent = _N;

    if (_P->_D_left == _N)
    {
        _P->_D_left = _N->_D_left;
        if (_P->_D_left)
        {
            _P->_D_left->_D_parent = _P;
        }
        _N->_D_left = _P;

        ::std::swap(_N->_D_right, _P->_D_right);
        if (_N->_D_right)
        {
            _N->_D_right->_D_parent = _N;
        }
        if (_P->_D_right)
        {
            _P->_D_right->_D_parent = _P;
        }
    }
    else
    {
        _P->_D_right = _N->_D_right;
        if (_P->_D_right)
        {
            _P->_D_right->_D_parent = _P;
        }
        _N->_D_right = _P;

        ::std::swap(_P->_D_left, _N->_D_left);
        if (_N->_D_left) {
            _N->_D_left->_D_parent = _N;
        }
        if (_P->_D_left) {
            _P->_D_left->_D_parent = _P;
        }
    }
}

template <typename _T, typename _Cmp>
inline auto
stdnet::_Intrusive_priority_queue<_T, _Cmp>::_Sift_up(::stdnet::_Intrusive_tree_node<_T>* _N)
    -> void
{
    while (_N->_D_parent && this->_D_cmp(_N->_D_parent->_D_value, _N->_D_value))
    {
        this->_Swap_nodes(_N->_D_parent, _N);
    }
    if (!_N->_D_parent)
    {
        this->_D_root = _N;
    }
}

template <typename _T, typename _Cmp>
inline auto stdnet::_Intrusive_priority_queue<_T, _Cmp>::push(_Intrusive_tree_node<_T>& _N) -> void
{
    if (this->empty())
    {
        this->_D_root = &_N;
        ++this->_D_size;
        return;
    }

    ::std::size_t _Index(this->_High_index(++this->_D_size));
    ::std::size_t _Path((1u << _Index) - 1 - this->_D_size);
    ::std::size_t _Bit(1u << (_Index - 2));
    auto          _Tmp(this->_D_root);
    for ( ; 1u < _Bit; _Bit >>=1)
    {
        _Tmp = (_Path & _Bit)? _Tmp->_D_left: _Tmp->_D_right;
    }
    _N._D_parent = _Tmp;
    ((_Path & 1u)? _Tmp->_D_left: _Tmp->_D_right) = &_N;

    this->_Sift_up(&_N);
}

// ----------------------------------------------------------------------------

#endif
