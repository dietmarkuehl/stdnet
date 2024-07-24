#include <concepts>
#include <chrono>
#include <span>
#include <string_view>
#include <type_traits>
#include <cstdint>
#include <system_error>

struct set_value_t {}; //-dk:TODO
struct set_error_t {}; //-dk:TODO

struct todo {}; //-dk:TODO
template <typename... T> struct todo_sender {}; //-dk:TODO

// ----------------------------------------------------------------------------

enum class preference // TAPS.6.2
{
    none = 0, // needs to be first
    require,
    prefer,
    avoid,
    prohibit
};

enum class availability // TAPS.6.2.14; TODO name
{
    disabled,
    active,
    passive
};

enum class direction_type // TAPS.6.2.16
{
    bidirectional,
    unidirectional_send,
    unidirectional_receive
};

template <typename, typename Type = preference, Type Default = Type()>
struct property
{
    static constexpr preference default_value() { return Default; }
};

enum reliability_tag {};
inline constexpr property<reliability_tag, preference, preference::require> reliability; // TAPS.6.2.1
enum preserve_message_boundaries_tag {};
inline constexpr property<preserve_message_boundaries_tag, preference, preference::none> preserve_message_boundaries; // TAPS.6.2.2
enum per_msg_reliability_tag {};
inline constexpr property<per_msg_reliability_tag, preference, preference::none> per_msg_reliability; // TAPS.6.2.3
enum preserve_order_tag {};
inline constexpr property<preserve_order_tag, preference, preference::require> preserve_order; // TAPS.6.2.4
enum zero_rtt_msg_tag {};
inline constexpr property<zero_rtt_msg_tag, preference, preference::none> zero_rtt_msg; // TAPS.6.2.5
enum multistreaming_tag {};
inline constexpr property<multistreaming_tag, preference, preference::prefer> multistreaming; // TAPS.6.2.6
enum full_checksum_send_tag {};
inline constexpr property<full_checksum_send_tag, preference, preference::require> full_checksum_send; // TAPS.6.2.7
enum full_checksum_recv_tag {};
inline constexpr property<full_checksum_recv_tag, preference, preference::require> full_checksum_recv; // TAPS.6.2.8
enum congestion_control_tag {};
inline constexpr property<congestion_control_tag, preference, preference::require> congestion_control; // TAPS.6.2.9
enum keep_alive_tag {};
inline constexpr property<keep_alive_tag, preference, preference::require> keep_alive; // TAPS.6.2.10
enum use_temporary_local_address_tag {};
inline constexpr property<use_temporary_local_address_tag, preference, preference::require> use_temporary_local_address; // TAPS.6.2.13
enum multipath_tag {};
inline constexpr property<multipath_tag, availability, availability::disabled> multipath; // TAPS.6.2.14
enum advertises_alt_addr_tag {};
inline constexpr property<advertises_alt_addr_tag, bool, false> advertises_alt_addr; // TAPS.6.2.15
enum direction_tag {};
inline constexpr property<direction_tag, direction_type, direction_type::bidirectional> direction; // TAPS.6.2.16
enum soft_error_notify_tag {};
inline constexpr property<soft_error_notify_tag, preference, preference::none> soft_error_notify; // TAPS.6.2.17
enum active_read_before_send_tag {};
inline constexpr property<active_read_before_send_tag, preference, preference::none> active_read_before_send; // TAPS.6.2.1

class transport_properties
{
public:
    transport_properties(); // TAPS.6.2
    template <typename Property>
    void set(Property, preference); // TAPS.6.2
    void add_interface(std::string_view, preference); // TAPS.6.2.11
    void add_pvd(std::string_view, preference); // TAPS.6.2.12
};

template <typename> struct parameter_type;
template <typename Parameter> using parameter_t = typename parameter_type<Parameter>::type;

enum class security_protocol // TAPS.6.3.1
{
    //-dk:TODO
};
security_protocol to_security_protocol(std::string_view);

enum class allowed_security_protocols_t {}; // TAPS.6.3.1
inline constexpr allowed_security_protocols_t allowed_security_protocols{};
template <>
struct parameter_type<allowed_security_protocols_t>
{
    using type = std::span<security_protocol>;
};

struct certificate // TAPS.6.3.2
{
};
enum class server_certificate_t {}; // TAPS.6.3.2
inline constexpr server_certificate_t server_certificate{};
template <>
struct parameter_type<server_certificate_t>
{
    using type = std::span<certificate>;
};
enum class client_certificate_t {}; // TAPS.6.3.2
inline constexpr client_certificate_t client_certificate{};
template <>
struct parameter_type<client_certificate_t>
{
    using type = std::span<certificate>;
};

struct certificate_chain // TAPS.6.3.3
{
};
enum class pinned_server_certificates_t {};
inline constexpr pinned_server_certificates_t pinner_server_certificates{}; // TAPS.6.3.3
template <>
struct parameter_type<pinned_server_certificates_t>
{
    using type = std::span<certificate_chain>;
};

enum class alpn_t {};
inline constexpr alpn_t alpn{}; // TAPS.6.3.4
template <>
struct parameter_type<alpn_t>
{
    using type = std::span<std::string_view>;
};

enum class group { /* implementation_defined */ }; // TAPS.6.3.5
group to_group(std::string_view);
enum class supported_group_t {};
inline constexpr supported_group_t supported_group{}; // TAPS.6.3.5
template <>
struct parameter_type<supported_group_t>
{
    using type = std::span<group>;
};

enum class cipher { /* implementation_defined */ }; // TAPS.6.3.5
cipher to_cipher(std::string_view);
enum class ciphersuite_t {};
inline constexpr ciphersuite_t ciphersuite{}; // TAPS.6.3.5
template <>
struct parameter_type<ciphersuite_t>
{
    using type = std::span<cipher>;
};

enum class algorithm { /* implementation_defined */ }; // TAPS.6.3.5
algorithm to_algorithm(std::string_view);
enum class signature_algorithm_t {};
inline constexpr signature_algorithm_t signature_algorithm{}; // TAPS.6.3.5
template <>
struct parameter_type<signature_algorithm_t>
{
    using type = std::span<algorithm>;
};

enum class max_cached_sessions_t {};
inline constexpr max_cached_sessions_t max_cached_sessions{}; // TAPS.6.3.6
template <>
struct parameter_type<max_cached_sessions_t>
{
    using type = ::std::size_t;
};

enum class cached_session_lifetime_t {};
inline constexpr cached_session_lifetime_t cached_session_lifetime{}; // TAPS.6.3.6
template <>
struct parameter_type<cached_session_lifetime_t>
{
    using type = std::chrono::seconds;
};

enum class pre_shared_key_t {};
inline constexpr pre_shared_key_t pre_shared_key{}; // TAPS.6.3.7

class security_parameters
{
public:
    enum enablement { enabled, disabled, opportunitistic }; // TAPS.6.3
    security_parameters(enablement = enabled); // TAPS.6.3
    template <typename Parameter>
    void set(Parameter, parameter_t<Parameter>); // TAPS.6.3
    void set(pre_shared_key_t, std::string_view, std::string_view); // TAPS.6.3.7
    //-dk:TODO callbacks: TAPS.6.3.8
};

// ----------------------------------------------------------------------------

class address_v4
{
public:
    address_v4(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t);
};
class address_v6
{
public:
    address_v6(std::string_view);
    // may need to cope with zone ID (see TAPS.6.1)
};

class address
{
public:
    address();
    address(address_v4);
    address(address_v6);
    address(char const*);
    address(std::string_view);
};

class group_address
{
public:
    group_address(char const*) {}
    group_address(std::string_view) {}
};
class source_address
{
public:
    source_address(char const*) {}
    source_address(std::string_view) {}
};

using port_number = std::uint16_t;

struct hop_limit
{
    hop_limit(std::size_t hops): hops(hops) {}
    todo ttl;  // ??? TAPS.6.1.1
    std::optional<std::size_t> hops; // ??? TAPS.6.1.1
};

class endpoint
{
};

class local_endpoint
    : endpoint
{
public:
    void with_ip_address(address_v4);      // TAPS.6.1.5
    void with_ip_address(address_v6);      // TAPS.6.1.5
    void with_ip_address(address);         // TAPS.6.1.5
    void with_interface(std::string_view); // TAPS.6.1
    void with_single_source_multicast_group_ip(group_address, source_address); //TAPS.6.1.1
    void with_any_source_multicast_group_ip(group_address); // TAPS.6.1.1
    void with_port(port_number);
    void with_stun_server(address, port_number, std::string_view); // TAPS.6.1.4
};

class remote_endpoint
    : endpoint
{
public:
    void with_host_name(std::string_view);
        // TAPS.6.1; should use fully-qualified domain names (FQDN)
        // TAPS.6.1: precondition: at most one identifier of a given type
    void with_port(port_number);         // TAPS.6.1
    void with_service(std::string_view);   // TAPS.6.1 determines the port
    void with_ip_address(address_v4);      // TAPS.6.1
    void with_ip_address(address_v6);      // TAPS.6.1
    void with_ip_address(address);         // TAPS.6.1?
    void with_multicast_group_ip(group_address); // TAPS.6.1.1
    void with_hop_limit(hop_limit);              // TAPS.6.1.1
    void with_protocol(std::string_view); // TAPS.6.1.3; argument should be an object or an enum?
};

class establishment_error
{
};

class connection
{
    todo_sender<set_value_t()> on_ready();
    todo_sender<set_error_t(establishment_error, std::error_code)> on_error();
};

class listener
{
};

class preconnection
{
public:
    // TAPS.6
    template <typename Local, typename Remote>
        requires(
            (std::same_as<std::remove_cvref_t<Local>, std::span<local_endpoint>>
                || std::same_as<std::remove_cvref_t<Local>, local_endpoint>
            )
            &&
            (std::same_as<std::remove_cvref_t<Remote>, std::span<remote_endpoint>>
                || std::same_as<std::remove_cvref_t<Remote>, remote_endpoint>
            )
        )
    preconnection(Local&& local,  // precondition: listen => non-empty
                  Remote&& remote, // precondition: initiate => non-empty
                  transport_properties,
                  security_parameters);

    todo_sender<std::span<std::string_view>> resolve(); // TAPS.6.1: yield available local interfaces

    todo_sender<set_value_t(connection)> initiate();   // TAPS.7.1
    todo_sender<set_value_t(connection)> initiate(std::chrono::milliseconds);   // TAPS.7.1
        // requires remote endpoint object
    todo_sender<set_value_t(listener)> listen();     // TAPS.7.2
    todo_sender<set_value_t(connection, listener), set_error_t(establishment_error)> rendezvous(); // TAPS.6.1.1
};
