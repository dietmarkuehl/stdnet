namespace stdnet
{
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
        inline constexpr char pvd[] = "pvd";
        inline constexpr char use_temporary_local_address[] = "use_temporary_local_address";
        inline constexpr char multipath[] = "multipath";
        inline constexpr char advertises_altaddr[] = "advertises_altaddr";
        inline constexpr char direction[] = "direction";
        inline constexpr char soft_error_notify[] = "soft_error_notify";
        inline constexpr char active_read_before_send[] = "active_read_before_send";
    }

    namespace _Names
    {
        inline constexpr char allowed_security_protocols[] = "allowed_security_protocols";
        inline constexpr char server_certificate[] = "server_certificate";
        inline constexpr char client_certificate[] = "client_certificate";
        inline constexpr char pinned_server_certificate[] = "pinned_server_certificate";
        inline constexpr char alpn[] = "alpn";
        inline constexpr char supported_group[] = "supported_group";
        inline constexpr char ciphersuite[] = "ciphersuite";
        inline constexpr char signature_algorithm[] = "signature_algorithm";
        inline constexpr char max_cached_sessions[] = "max_cached_sessions";
        inline constexpr char cached_session_lifetime_seconds[] = "cached_session_lifetime_seconds";
        inline constexpr char pre_shared_key[] = "pre_shared_key";
    }

    namespace _Names
    {
        inline constexpr char msg_lifetime[] = "msg_lifetime";
        inline constexpr char msg_priority[] = "msg_priority";
        inline constexpr char msg_ordered[] = "msg_ordered";
        inline constexpr char safely_replayable[] = "safely_replayable";
        inline constexpr char final[] = "final";
        inline constexpr char msg_checksum_len[] = "msg_checksum_len";
        inline constexpr char msg_reliable[] = "msg_reliable";
        inline constexpr char msg_capacity_profile[] = "msg_capacity_profile";
        inline constexpr char no_fragmentation[] = "no_fragmentation";
        inline constexpr char no_segmentation[] = "no_segmentation";
    }

    using reliability_t = ::stdnet::property<
        ::stdnet::_Names::reliability,
        ::stdnet::preference::require
        >;
    inline constexpr reliability_t reliability{};

    using preserve_msg_boundaries_t = ::stdnet::property<
        ::stdnet::_Names::preserve_msg_boundaries,
        ::stdnet::preference::no_preference
        >;
    inline constexpr preserve_msg_boundaries_t preserve_msg_boundaries{};

    using per_msg_reliability_t = ::stdnet::property<
        ::stdnet::_Names::per_msg_reliability,
        ::stdnet::preference::no_preference
        >;
    inline constexpr per_msg_reliability_t per_msg_reliability{};

    using preserve_order_t = ::stdnet::property<
        ::stdnet::_Names::preserve_order,
        ::stdnet::preference::require
        >;
    inline constexpr preserve_order_t preserve_order{};

    using zero_rtt_msg_t = ::stdnet::property<
        ::stdnet::_Names::zero_rtt_msg,
        ::stdnet::preference::no_preference
        >;
    inline constexpr zero_rtt_msg_t zero_rtt_msg{};

    using multistreaming_t = ::stdnet::property<
        ::stdnet::_Names::multistreaming,
        ::stdnet::preference::prefer
        >;
    inline constexpr multistreaming_t multistreaming{};

    using full_checksum_send_t = ::stdnet::property<
        ::stdnet::_Names::full_checksum_send,
        ::stdnet::preference::require
        >;
    inline constexpr full_checksum_send_t full_checksum_send{};

    using full_checksum_recv_t = ::stdnet::property<
        ::stdnet::_Names::full_checksum_recv,
        ::stdnet::preference::require
        >;
    inline constexpr full_checksum_recv_t full_checksum_recv{};

    using congestion_control_t = ::stdnet::property<
        ::stdnet::_Names::congestion_control,
        ::stdnet::preference::require
        >;
    inline constexpr congestion_control_t congestion_control{};

    using keep_alive_t = ::stdnet::property<
        ::stdnet::_Names::keep_alive,
        ::stdnet::preference::no_preference
        >;
    inline constexpr keep_alive_t keep_alive{};

    using interface_t = ::stdnet::property<
        ::stdnet::_Names::interface,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr interface_t interface{};

    using pvd_t = ::stdnet::property<
        ::stdnet::_Names::pvd,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr pvd_t pvd{};

    using use_temporary_local_address_t = ::stdnet::property<
        ::stdnet::_Names::use_temporary_local_address,
        ::stdnet::preference::avoid
        >;
    inline constexpr use_temporary_local_address_t use_temporary_local_address{};

    using multipath_t = ::stdnet::property<
        ::stdnet::_Names::multipath,
        ::stdnet::multipath_type::disabled
        >;
    inline constexpr multipath_t multipath{};

    using advertises_altaddr_t = ::stdnet::property<
        ::stdnet::_Names::advertises_altaddr,
        false
        >;
    inline constexpr advertises_altaddr_t advertises_altaddr{};

    using direction_t = ::stdnet::property<
        ::stdnet::_Names::direction,
        ::stdnet::direction_type::bidirectional
        >;
    inline constexpr direction_t direction{};

    using soft_error_notify_t = ::stdnet::property<
        ::stdnet::_Names::soft_error_notify,
        ::stdnet::preference::no_preference
        >;
    inline constexpr soft_error_notify_t soft_error_notify{};

    using active_read_before_send_t = ::stdnet::property<
        ::stdnet::_Names::active_read_before_send,
        ::stdnet::preference::no_preference
        >;
    inline constexpr active_read_before_send_t active_read_before_send{};

    using allowed_security_protocols_t = ::stdnet::property<
        ::stdnet::_Names::allowed_security_protocols,
        ::stdnet::_Empty{},
        ::std::vector<::stdnet::security_protocol>
        >;
    inline constexpr allowed_security_protocols_t allowed_security_protocols{};

    using server_certificate_t = ::stdnet::property<
        ::stdnet::_Names::server_certificate,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr server_certificate_t server_certificate{};

    using client_certificate_t = ::stdnet::property<
        ::stdnet::_Names::client_certificate,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr client_certificate_t client_certificate{};

    using pinned_server_certificate_t = ::stdnet::property<
        ::stdnet::_Names::pinned_server_certificate,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr pinned_server_certificate_t pinned_server_certificate{};

    using alpn_t = ::stdnet::property<
        ::stdnet::_Names::alpn,
        ::stdnet::_Empty{},
        ::std::vector<::std::string>
        >;
    inline constexpr alpn_t alpn{};

    using supported_group_t = ::stdnet::property<
        ::stdnet::_Names::supported_group,
        ::stdnet::_Empty{},
        ::std::vector<::stdnet::group_type>
        >;
    inline constexpr supported_group_t supported_group{};

    using ciphersuite_t = ::stdnet::property<
        ::stdnet::_Names::ciphersuite,
        ::stdnet::_Empty{},
        ::std::vector<::stdnet::cipher_suite_type>
        >;
    inline constexpr ciphersuite_t ciphersuite{};

    using signature_algorithm_t = ::stdnet::property<
        ::stdnet::_Names::signature_algorithm,
        ::stdnet::_Empty{},
        ::std::vector<::stdnet::signature_algorithm_type>
        >;
    inline constexpr signature_algorithm_t signature_algorithm{};

    using max_cached_sessions_t = ::stdnet::property<
        ::stdnet::_Names::max_cached_sessions,
        ::std::size_t(5)
        >;
    inline constexpr max_cached_sessions_t max_cached_sessions{};

    using cached_session_lifetime_seconds_t = ::stdnet::property<
        ::stdnet::_Names::cached_session_lifetime_seconds,
        ::std::size_t(60)
        >;
    inline constexpr cached_session_lifetime_seconds_t cached_session_lifetime_seconds{};

    using pre_shared_key_t = ::stdnet::property<
        ::stdnet::_Names::pre_shared_key,
        ::stdnet::_Empty{},
        ::std::string
        >;
    inline constexpr pre_shared_key_t pre_shared_key{};

    using msg_lifetime_t = ::stdnet::property<
        ::stdnet::_Names::msg_lifetime,
        ~::std::size_t{}
        >;
    inline constexpr msg_lifetime_t msg_lifetime{};

    using msg_priority_t = ::stdnet::property<
        ::stdnet::_Names::msg_priority,
        ::std::size_t{100}
        >;
    inline constexpr msg_priority_t msg_priority{};

    using msg_ordered_t = ::stdnet::property<
        ::stdnet::_Names::msg_ordered,
        true
        >;
    inline constexpr msg_ordered_t msg_ordered{};

    using safely_replayable_t = ::stdnet::property<
        ::stdnet::_Names::safely_replayable,
        false
        >;
    inline constexpr safely_replayable_t safely_replayable{};

    using final_t = ::stdnet::property<
        ::stdnet::_Names::final,
        false
        >;
    inline constexpr final_t final{};

    using msg_checksum_len_t = ::stdnet::property<
        ::stdnet::_Names::msg_checksum_len,
        ~::std::size_t{}
        >;
    inline constexpr msg_checksum_len_t msg_checksum_len{};

    using msg_reliable_t = ::stdnet::property<
        ::stdnet::_Names::msg_reliable,
        true
        >;
    inline constexpr msg_reliable_t msg_reliable{};

    using msg_capacity_profile_t = ::stdnet::property<
        ::stdnet::_Names::msg_capacity_profile,
        ::stdnet::conn_capacity_profile::best_effort
        >;
    inline constexpr msg_capacity_profile_t msg_capacity_profile{};

    using no_fragmentation_t = ::stdnet::property<
        ::stdnet::_Names::no_fragmentation,
        false
        >;
    inline constexpr no_fragmentation_t no_fragmentation{};

    using no_segmentation_t = ::stdnet::property<
        ::stdnet::_Names::no_segmentation,
        false
        >;
    inline constexpr no_segmentation_t no_segmentation{};

    using transport_properties = ::stdnet::properties<
        reliability_t,
        preserve_msg_boundaries_t,
        per_msg_reliability_t,
        preserve_order_t,
        zero_rtt_msg_t,
        multistreaming_t,
        full_checksum_send_t,
        full_checksum_recv_t,
        congestion_control_t,
        keep_alive_t,
        interface_t,
        pvd_t,
        use_temporary_local_address_t,
        multipath_t,
        advertises_altaddr_t,
        direction_t,
        soft_error_notify_t,
        active_read_before_send_t
        >;

    using security_parameters = ::stdnet::properties<
        allowed_security_protocols_t,
        server_certificate_t,
        client_certificate_t,
        pinned_server_certificate_t,
        alpn_t,
        supported_group_t,
        ciphersuite_t,
        signature_algorithm_t,
        max_cached_sessions_t,
        cached_session_lifetime_seconds_t,
        pre_shared_key_t
        >;

    using message_properties = ::stdnet::properties<
        msg_lifetime_t,
        msg_priority_t,
        msg_ordered_t,
        safely_replayable_t,
        final_t,
        msg_checksum_len_t,
        msg_reliable_t,
        msg_capacity_profile_t,
        no_fragmentation_t,
        no_segmentation_t
        >;

}
