#include <any>
#include <iostream>
#include <stdexcept>
#include <stdnet/properties.hpp>
#include <stdnet/preconnection.hpp>

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
        ::std::cout << "reliability=" << tp.get(::stdnet::reliability) << "\n";
        ::std::cout << "reliability=" << extract<::stdnet::preference>(tp.get("reliability")) << "\n";
        ::std::cout << "preserve_msg_boundaries=" << tp.get(::stdnet::preserve_msg_boundaries) << "\n";
        ::std::cout << "preserve_msg_boundaries=" << extract<::stdnet::preference>(tp.get("preserve_msg_boundaries")) << "\n";
    }
    catch(::std::exception const& ex)
    {
        ::std::cerr << ex.what() << '\n';
    }
    
}
