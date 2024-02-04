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
        ::std::cout << "reliability=" << tp.get(::stdnet::reliability) << "\n";
        ::std::cout << "reliability=" << extract<::stdnet::preference>(tp.get("reliability")) << "\n";
        ::std::cout << "preserve_msg_boundaries=" << tp.get(::stdnet::preserve_msg_boundaries) << "\n";
        ::std::cout << "preserve_msg_boundaries=" << extract<::stdnet::preference>(tp.get("preserve_msg_boundaries")) << "\n";

        std::cout << tp << "\n";
    }
    catch(::std::exception const& ex)
    {
        ::std::cerr << ex.what() << '\n';
    }
    
}
