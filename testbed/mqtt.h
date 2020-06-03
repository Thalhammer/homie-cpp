#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <ttl/promise.h>
#include <ttl/logger.h>

namespace homiecpp {
namespace mqtt {
    class message;
    class connect_options;
    class abstract_client;
    typedef std::shared_ptr<message> message_ptr;
    typedef std::shared_ptr<const message> const_message_ptr;
    typedef std::shared_ptr<connect_options> connect_options_ptr;
    typedef std::shared_ptr<const connect_options> const_connect_options_ptr;
    typedef std::shared_ptr<abstract_client> abstract_client_ptr;
    typedef std::shared_ptr<const abstract_client> const_abstract_client_ptr;
    class message {
        int m_id;
        std::string m_topic;
        std::string m_payload;
        int m_qos;
        bool m_retain;
        bool m_duplicate;
    public:
        typedef message_ptr ptr;
        typedef const_message_ptr const_ptr;
        int get_id() const noexcept { return m_id; }
        const std::string& get_topic() const noexcept { return m_topic; }
        const std::string& get_payload() const noexcept { return m_payload; }
        int get_qos() const noexcept { return m_qos; }
        bool is_retained() const noexcept { return m_retain; }
        bool is_duplicate() const noexcept { return m_duplicate;}
        void set_id(int id) noexcept { m_id = id; }
        void set_topic(std::string topic) noexcept { m_topic = std::move(topic); }
        void set_payload(std::string payload) noexcept { m_payload = std::move(payload); }
        void set_qos(int qos) noexcept { m_qos = qos; }
        void set_retained(bool retain) noexcept { m_retain = retain; }
        void set_duplicate(bool duplicate) noexcept { m_duplicate = duplicate; }

        static bool validate_qos(int qos) noexcept {
            if(qos < 0 || qos > 2) return false;
            return true;
        }
    };
    class connect_options {
    public:
        static constexpr size_t default_keep_alive_interval = 60;
        static constexpr size_t default_timeout = 30;
        static constexpr size_t default_max_inflight = 10;
        static constexpr bool default_clean_session = true;
        enum class version {
            auto_select,
            v311,
            v31,
            v5
        };
        bool is_auto_reconnect() const noexcept { return m_auto_reconnect; }
        bool is_clean_session() const noexcept { return m_clean_session; }
        size_t get_connection_timeout() const noexcept { return m_connection_timeout; }
        size_t get_keepalive_interval() const noexcept { return m_keepalive_interval; }
        size_t get_max_in_flight() const noexcept { return m_max_in_flight; }
        version get_version() const noexcept { return m_version; }
        const std::string& get_password() const noexcept { return m_password; }
        const std::string& get_username() const noexcept { return m_username; }
        const std::string& get_clientid() const noexcept { return m_clientid; }
        const std::set<std::pair<std::string, uint16_t>>& get_serveruris() const noexcept { return m_serveruris; }
        const_message_ptr get_last_will() const noexcept { return m_last_will; }
        void set_auto_reconnect(bool enable) noexcept { m_auto_reconnect = enable; }
        void set_clean_session(bool clean) noexcept { m_clean_session = clean; }
        void set_connection_timeout(size_t timeout) noexcept { m_connection_timeout = timeout; }
        void set_keepalive_interval(size_t interval) noexcept { m_keepalive_interval = interval; }
        void set_max_in_flight(size_t mif) noexcept { m_max_in_flight = mif; }
        void set_version(version v) noexcept { m_version = v; }
        void set_password(std::string pw) noexcept { m_password = std::move(pw); }
        void set_username(std::string uname) noexcept { m_username = std::move(uname); }
        void set_clientid(std::string clientid) noexcept { m_clientid = std::move(clientid); }
        void set_serveruris(std::set<std::pair<std::string, uint16_t>> uris) noexcept { m_serveruris = std::move(uris); }
        void set_last_will(const_message_ptr msg) noexcept { m_last_will = std::make_shared<message>(*msg); }
        void set_last_will(std::string topic, std::string payload, int qos, bool retain) {
            auto msg = std::make_shared<message>();
            msg->set_topic(std::move(topic));
            msg->set_payload(std::move(payload));
            msg->set_qos(qos);
            msg->set_retained(retain);
            msg->set_id(-1);
            msg->set_duplicate(false);
            m_last_will = msg;
        }
        static std::string generate_random_clientid();
    private:
        bool m_auto_reconnect = false;
        bool m_clean_session = default_clean_session;
        size_t m_connection_timeout = default_timeout;
        size_t m_keepalive_interval = default_keep_alive_interval;
        size_t m_max_in_flight = default_max_inflight;
        version m_version = version::auto_select;
        std::string m_password;
        std::string m_username;
        std::string m_clientid;
        std::set<std::pair<std::string, uint16_t>> m_serveruris;
        message_ptr m_last_will = nullptr;
    };
    class abstract_client {
    public:
        typedef std::function<void(ttl::loglevel, std::string)> on_log_cb_t;
        typedef std::function<void(const_message_ptr)> on_message_cb_t;

        virtual ~abstract_client() {}
        virtual void connect(const std::string& uri) = 0;
        virtual void connect(const_connect_options_ptr opts) = 0;
        virtual void disconnect() = 0;
        virtual const std::string& get_clientid() const = 0;
        virtual const std::string& get_serveruri() const = 0;
        virtual bool is_connected() const = 0;
        virtual void publish(std::string topic, std::string payload, int qos = 1, bool retained = false) = 0;
        virtual void publish(message_ptr msg) = 0;
        virtual void reconnect() = 0;
        virtual void subscribe(const std::string& topic) = 0;
        virtual void subscribe(const std::vector<std::string>& topics) = 0;
        virtual void subscribe(const std::string& topic, int qos) = 0;
        virtual void subscribe(const std::vector<std::pair<std::string, int>>& topics) = 0;
        virtual void unsubscribe(const std::string& topic) = 0;
        virtual void unsubscribe(const std::vector<std::string>& topic) = 0;
        virtual void set_on_message(on_message_cb_t cb) = 0;
        virtual void set_on_log(on_log_cb_t cb) = 0;
    };
    class abstract_async_client {
    public:
        typedef std::function<void(int)> on_connect_cb_t;
        typedef std::function<void(int)> on_disconnect_cb_t;
        typedef std::function<void(int)> on_publish_cb_t;
        typedef std::function<void(const_message_ptr)> on_message_cb_t;
        typedef std::function<void(int, std::vector<int>)> on_subscribe_cb_t;
        typedef std::function<void(int)> on_unsubscribe_cb_t;
        typedef std::function<void(ttl::loglevel, std::string)> on_log_cb_t;

        virtual ~abstract_async_client() {}
        virtual void connect(const std::string& uri) = 0;
        virtual void connect(const_connect_options_ptr opts) = 0;
        virtual void disconnect() = 0;
        virtual const std::string& get_clientid() const = 0;
        virtual const std::string& get_serveruri() const = 0;
        virtual bool is_connected() const = 0;
        virtual void publish(std::string topic, std::string payload, int qos = 1, bool retained = false) = 0;
        virtual void publish(message_ptr msg) = 0;
        virtual void reconnect() = 0;
        virtual void subscribe(const std::string& topic) = 0;
        virtual void subscribe(const std::vector<std::string>& topics) = 0;
        virtual void subscribe(const std::string& topic, int qos) = 0;
        virtual void subscribe(const std::vector<std::pair<std::string, int>>& topics) = 0;
        virtual void unsubscribe(const std::string& topic) = 0;
        virtual void unsubscribe(const std::vector<std::string>& topic) = 0;

        virtual void set_on_log(on_log_cb_t cb) = 0;
        virtual void set_on_connect(on_connect_cb_t) = 0;
        virtual void set_on_disconnect(on_disconnect_cb_t) = 0;
        virtual void set_on_publish(on_publish_cb_t) = 0;
        virtual void set_on_message(on_message_cb_t) = 0;
        virtual void set_on_subscribe(on_subscribe_cb_t) = 0;
        virtual void set_on_unsubscribe(on_unsubscribe_cb_t) = 0;
    };

    abstract_client_ptr create_client(); 
}
}