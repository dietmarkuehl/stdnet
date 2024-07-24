#include "taps_preconnection.hpp"

int main()
{
    {
        // TAPS.6.1.4
        remote_endpoint remote_specifier;
        remote_specifier.with_host_name("example.com");
        remote_specifier.with_service("https");
    }
    {
        // TAPS.6.1.4
        remote_endpoint remote_specifier;
        remote_specifier.with_ip_address(address_v6("2001:db8:e29d:a420:7461:7073:a"));
        remote_specifier.with_service("https");
    }
    {
        // TAPS.6.1.4
        remote_endpoint remote_specifier;
        remote_specifier.with_ip_address(address_v4(192, 0, 2, 21));
        remote_specifier.with_service("https");
    }
    {
        // TAPS.6.1.4
        local_endpoint local_specifier;
        local_specifier.with_interface("en0");
    }
    {
        // TAPS.6.1.4
        local_endpoint local_specifier;
        local_specifier.with_interface("en0");
        local_specifier.with_port(443);
    }
    {
        // TAPS.6.1.4
        local_endpoint local_specifier;
        local_specifier.with_stun_server(address(), 443, "credentials");
    }
    {
        // TAPS.6.1.5
        remote_endpoint remote_specifier;
        local_endpoint  local_specifier;

        local_specifier.with_any_source_multicast_group_ip("233.252.0.0");
        local_specifier.with_port(5353);
        local_specifier.with_interface("en0");

        transport_properties transport;
        security_parameters  security;

        preconnection connection(local_specifier,
                                 remote_specifier,
                                 transport,
                                 security);
        auto listener = connection.listen();
    }
    {
        // TAPS.6.1.5
        remote_endpoint remote_specifier;
        local_endpoint  local_specifier;

        local_specifier.with_single_source_multicast_group_ip("233.252.0.0", "198.51.100.10");
        local_specifier.with_port(5353);
        local_specifier.with_interface("en0");

        transport_properties transport;
        security_parameters  security;

        preconnection connection(local_specifier,
                                 remote_specifier,
                                 transport,
                                 security);
        auto listener = connection.listen();
    }
    {
        // TAPS.6.1.5
        remote_endpoint remote_specifier;
        remote_specifier.with_multicast_group_ip("233.251.240.1");
        remote_specifier.with_port(5353);
        remote_specifier.with_hop_limit(8);

        local_endpoint  local_specifier;

        local_specifier.with_ip_address("192.0.2.22");
        local_specifier.with_interface("en0");
        local_specifier.with_port(5353);

        transport_properties transport;
        security_parameters  security;

        preconnection pre(local_specifier,
                                 remote_specifier,
                                 transport,
                                 security);
        auto connection = pre.initiate();
    }
    {
        // TAPS.6.1.5
        remote_endpoint remote_specifier;
        remote_specifier.with_multicast_group_ip("233.252.0.0");
        remote_specifier.with_port(5353);
        remote_specifier.with_hop_limit(8);

        local_endpoint  local_specifier;
        local_specifier.with_any_source_multicast_group_ip("233.252.0.0");
        local_specifier.with_ip_address("192.0.2.22");
        local_specifier.with_interface("en0");
        local_specifier.with_port(5353);

        transport_properties transport;
        security_parameters  security;

        preconnection pre(local_specifier,
                                 remote_specifier,
                                 transport,
                                 security);
        auto sender_connection_listener = pre.rendezvous();
    }
}
