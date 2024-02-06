#!/usr/bin/perl -w
use strict;

my @transport_properties = (
    # 6.2 Transport Properties
    [ "reliability", "::stdnet::preference::require" ],
    [ "preserve_msg_boundaries", "::stdnet::preference::no_preference" ],
    [ "per_msg_reliability", "::stdnet::preference::no_preference" ],
    [ "preserve_order", "::stdnet::preference::require" ],
    [ "zero_rtt_msg", "::stdnet::preference::no_preference" ],
    [ "multistreaming", "::stdnet::preference::prefer" ],
    [ "full_checksum_send", "::stdnet::preference::require" ],
    [ "full_checksum_recv", "::stdnet::preference::require" ],
    [ "congestion_control", "::stdnet::preference::require" ],
    [ "keep_alive", "::stdnet::preference::no_preference" ],
    [ "interface", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "pvd", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "use_temporary_local_address", "::stdnet::preference::avoid" ], #-dk:TODO the default depends on the use!?!
    [ "multipath", "::stdnet::multipath_type::disabled" ], #-dk:TODO the default depends on the use!?!
    [ "advertises_altaddr", "false" ],
    [ "direction", "::stdnet::direction_type::bidirectional" ],
    [ "soft_error_notify", "::stdnet::preference::no_preference" ],
    [ "active_read_before_send", "::stdnet::preference::no_preference" ],
);

my @security_parameters = (
    # 6.3 Security Parameters
    [ "allowed_security_protocols", "::stdnet::_Empty{}", "::std::vector<::stdnet::security_protocol>" ], #-dk:TODO fix default
    [ "server_certificate", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "client_certificate", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "pinned_server_certificate", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "alpn", "::stdnet::_Empty{}", "::std::vector<::std::string>" ],
    [ "supported_group", "::stdnet::_Empty{}", "::std::vector<::stdnet::group_type>" ], #-dk:TODO set the default
    [ "ciphersuite", "::stdnet::_Empty{}", "::std::vector<::stdnet::cipher_suite_type>" ], #-dk:TODO set the default
    [ "signature_algorithm", "::stdnet::_Empty{}", "::std::vector<::stdnet::signature_algorithm_type>" ], #-dk:TODO set the default
    [ "max_cached_sessions", "::std::size_t(5)" ], #-dk:TODO determine value dynamically
    [ "cached_session_lifetime_seconds", "::std::size_t(60)" ], #-dk:TODO determine value dynamically
    [ "pre_shared_key", "::stdnet::_Empty{}", "::std::string" ],
    #-dk:TODO trust_callback
    #-dk:TODO challenge_callback
);

my @message_properties = (
    # 9.1.3 Message Properties
    [ "msg_lifetime", "~::std::size_t{}" ],
    [ "msg_priority", "::std::size_t{100}" ],
    [ "msg_ordered", "true" ], #-dk:TODO determine dynamically
    [ "safely_replayable", "false" ],
    [ "final", "false" ],
    [ "msg_checksum_len", "~::std::size_t{}" ],
    [ "msg_reliable", "true" ], #-dk:TODO determine dynamically
    [ "msg_capacity_profile", "::stdnet::conn_capacity_profile::best_effort" ], #-dk:TODO determine dynamically
    [ "no_fragmentation", "false" ],
    [ "no_segmentation", "false" ],
);

sub mknames(@)
{
    my @properties = @_;
    print "    namespace _Names\n";
    print "    {\n";
    foreach my $i (0..$#properties)
    {
        my @spec = @{$properties[$i]};
        print "        inline constexpr char ", $spec[0], "[] = \"", $spec[0], "\";\n";
    }
    print "    }\n";
    print "\n";
}
    
sub mkproperties(@)
{
    my @properties = @_;
    foreach my $i (0..$#properties)
    {
        my($name, $dfault, $type) = @{$properties[$i]};
        print "    using $name" . "_t = ::stdnet::property<\n";
        print "        ::stdnet::_Names::$name,\n";
        print "        $dfault";
        if ($type)
        {
            print ",\n        $type";
        }
        print "\n        >;\n";
        print "    inline constexpr $name" . "_t $name" . "{};\n";
        print "\n";
    }
    
}
    
sub mktype($@)
{
    my $name = shift;
    my @properties = @_;
    print "    using $name = ::stdnet::properties<\n";
    foreach my $i (0..$#properties)
    {
        my($name, $dfault, $type) = @{$properties[$i]};
        print "        $name", "_t", ($i != $#properties?",": "") ,"\n";
    }
    print "        >;\n";
    print "\n";
}

print "namespace stdnet\n";
print "{\n";
mknames(@transport_properties);
mknames(@security_parameters);
mknames(@message_properties);

mkproperties(@transport_properties);
mkproperties(@security_parameters);
mkproperties(@message_properties);

mktype("transport_properties", @transport_properties);
mktype("security_parameters", @security_parameters);
mktype("message_properties", @message_properties);

print "}\n";
    