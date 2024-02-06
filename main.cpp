#include <any>
#include <iostream>
#include <stdexcept>
#include "stdnet/preferences.hpp"
#include "stdnet/preconnection.hpp"

template <typename T>
T extract(::std::any const& v)
{
    return v.has_value()
        ? ::std::any_cast<T>(v)
        : throw std::runtime_error("no value")
        ;
}

int main()
{
    try
    {
        ::stdnet::transport_properties tp;
        tp.set(::stdnet::reliability, ::stdnet::preference::prohibit);
        tp.push_back(::stdnet::interface, "eth0");
        tp.push_back(::stdnet::interface, "wifi0");

        std::cout << "transport-properties=" << tp << "\n";
        std::cout << "security_parameters=" << ::stdnet::security_parameters{} << "\n";
        std::cout << "message-properties=" << ::stdnet::message_properties{} << "\n";
    }
    catch(::std::exception const& ex)
    {
        ::std::cerr << ex.what() << '\n';
    }
    
}
