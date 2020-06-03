#include "mqtt.h"
#include <mosquitto.h>
#include <iostream>
#include <cstring>

struct mosq_error_category : std::error_category
{
  	const char* name() const noexcept override { return "mosquitto_error"; }
  	std::string message(int ev) const override {
	  	return mosquitto_strerror(ev);
  	}
};
const mosq_error_category& get_mosq_error_category() {
	const static mosq_error_category category;
	return category;
}
namespace std
{
  	template <>
	struct is_error_code_enum<mosq_err_t> : true_type {};
}
std::error_code make_error_code(mosq_err_t e) {
	return std::error_code{(int)e, get_mosq_error_category() };
}

namespace homiecpp {
namespace mqtt {
    std::string connect_options::generate_random_clientid() {
        return "random";
    }

    class client : public abstract_client {
        mosquitto* m_inst = nullptr;
        connect_options_ptr m_connect_options = nullptr;
        std::string m_current_uri;
        bool m_connected = false;
        on_message_cb_t m_on_msg;
        on_log_cb_t m_on_log;
        
        std::mutex m_inflight_mtx;
        std::condition_variable m_inflight_cv;
        std::set<int> m_inflight_list;

        void await_msg(int mid, std::unique_lock<std::mutex>& lck);
    public:
        client();
        ~client();
        void connect(const std::string& uri) override;
        void connect(const_connect_options_ptr opts) override;
        void disconnect() override;
        const std::string& get_clientid() const override;
        const std::string& get_serveruri() const override;
        bool is_connected() const override;
        void publish(std::string topic, std::string payload, int qos, bool retained) override;
        void publish(message_ptr msg) override;
        void reconnect() override;
        void subscribe(const std::string& topic) override;
        void subscribe(const std::vector<std::string>& topics) override;
        void subscribe(const std::string& topic, int qos) override;
        void subscribe(const std::vector<std::pair<std::string, int>>& topics) override;
        void unsubscribe(const std::string& topic) override;
        void unsubscribe(const std::vector<std::string>& topic) override;
        void set_on_message(on_message_cb_t cb) override;
        void set_on_log(on_log_cb_t cb) override;
    };

    class async_client : public abstract_async_client {
        mosquitto* m_inst = nullptr;
        connect_options_ptr m_connect_options = nullptr;
        std::string m_current_uri;
        bool m_connected = false;
        on_message_cb_t m_on_msg;
        on_log_cb_t m_on_log;
    public:
        async_client();
        ~async_client();
        void connect(const std::string& uri) override;
        void connect(const_connect_options_ptr opts) override;
        void disconnect() override;
        const std::string& get_clientid() const override;
        const std::string& get_serveruri() const override;
        bool is_connected() const override;
        void publish(std::string topic, std::string payload, int qos, bool retained) override;
        void publish(message_ptr msg) override;
        void reconnect() override;
        void subscribe(const std::string& topic) override;
        void subscribe(const std::vector<std::string>& topics) override;
        void subscribe(const std::string& topic, int qos) override;
        void subscribe(const std::vector<std::pair<std::string, int>>& topics) override;
        void unsubscribe(const std::string& topic) override;
        void unsubscribe(const std::vector<std::string>& topic) override;
        
        void set_on_log(on_log_cb_t cb) override;
        void set_on_connect(on_connect_cb_t) override;
        void set_on_disconnect(on_disconnect_cb_t) override;
        void set_on_publish(on_publish_cb_t) override;
        void set_on_message(on_message_cb_t) override;
        void set_on_subscribe(on_subscribe_cb_t) override;
        void set_on_unsubscribe(on_unsubscribe_cb_t) override;
    };
    
    void client::await_msg(int mid, std::unique_lock<std::mutex>& lck) {
        m_inflight_list.insert(mid);
        while(m_inflight_list.count(mid) != 0) {
            m_inflight_cv.wait(lck);
        }
        m_inflight_list.erase(mid);
    }
    client::client() {
        if(mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
            throw std::runtime_error("mosquitto_lib_init failed");
    }
    client::~client() {
        if(m_inst) {
            mosquitto_disconnect(m_inst);
            mosquitto_loop_stop(m_inst, true);
            mosquitto_destroy(m_inst);
            m_inst = nullptr;
        }
        mosquitto_lib_cleanup();
    }
    void client::connect(const std::string& uri) {
        auto opts = std::make_shared<connect_options>();
        opts->set_serveruris({{uri, 1883}});
        opts->set_clientid(connect_options::generate_random_clientid());
        this->connect(opts);
    }
    void client::connect(const_connect_options_ptr opts) {
        m_connect_options = std::make_shared<connect_options>(*opts);

        int major, minor, revision;
        if(mosquitto_lib_version(&major, &minor, &revision) != LIBMOSQUITTO_VERSION_NUMBER)
            throw std::runtime_error("libmosquitto version missmatch");
        if(m_on_log)
            m_on_log(ttl::loglevel::INFO, "Initialized libmosquitto v" + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(revision));

        m_inst = mosquitto_new(m_connect_options->get_clientid().c_str(), m_connect_options->is_clean_session(), this);
        if(m_inst == nullptr)
            throw std::system_error(errno, std::system_category());
        mosquitto_log_callback_set(m_inst, [](struct mosquitto *, void* arg, int level, const char *str) {
            auto inst = reinterpret_cast<client*>(arg);
            if(inst->m_on_log) {
                ttl::loglevel lvl = ttl::loglevel::INFO;
                switch(level) {
                    case MOSQ_LOG_INFO: lvl = ttl::loglevel::INFO; break;
                    case MOSQ_LOG_NOTICE: lvl = ttl::loglevel::INFO; break;
                    case MOSQ_LOG_WARNING: lvl = ttl::loglevel::WARN; break;
                    case MOSQ_LOG_ERR: lvl = ttl::loglevel::ERR; break;
                    case MOSQ_LOG_DEBUG: lvl = ttl::loglevel::DEBUG; break;
                }
                inst->m_on_log(lvl, str);
            }
        });
        mosquitto_disconnect_callback_set(m_inst, [](mosquitto*, void* arg, int rc){
            auto inst = reinterpret_cast<client*>(arg);
            inst->m_connected = false;
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Disconnected " + std::to_string(rc));
        });
        mosquitto_connect_callback_set(m_inst, [](mosquitto*, void* arg, int rc){
            auto inst = reinterpret_cast<client*>(arg);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, std::string("Connect callback ") + mosquitto_connack_string(rc));
            if(rc != 0) return;
            inst->m_connected = true;
        });
        mosquitto_publish_callback_set(m_inst, [](mosquitto*, void* arg, int mid){
            auto inst = reinterpret_cast<client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_inflight_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_inflight_cv.notify_all();
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Published " + std::to_string(mid));
        });
        mosquitto_subscribe_callback_set(m_inst, [](mosquitto*, void* arg, int mid, int qos_count, const int* granted_qos){
            (void)qos_count;
            (void)granted_qos;
            auto inst = reinterpret_cast<client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_inflight_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_inflight_cv.notify_all();
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Subscribed " + std::to_string(mid) + " granted_qos=" + std::to_string(*granted_qos));
        });
        mosquitto_unsubscribe_callback_set(m_inst, [](mosquitto*, void* arg, int mid){
            auto inst = reinterpret_cast<client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_inflight_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_inflight_cv.notify_all();
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Unsubscribed " + std::to_string(mid));
        });
        mosquitto_message_callback_set(m_inst, [](mosquitto*, void* arg, const struct mosquitto_message* msg){
            auto inst = reinterpret_cast<client*>(arg);
            if(inst->m_on_msg) {
                auto nmsg = std::make_shared<message>();
                nmsg->set_id(msg->mid);
                nmsg->set_topic(msg->topic);
                nmsg->set_qos(msg->qos);
                nmsg->set_retained(msg->retain);
                nmsg->set_duplicate(false);
                nmsg->set_payload(std::string(reinterpret_cast<const char*>(msg->payload), msg->payloadlen));
                if(inst->m_on_log)
                    inst->m_on_log(ttl::loglevel::DEBUG, std::string("Message \"") + msg->topic + "\": " + std::to_string(msg->payloadlen) + " bytes");
            }
        });
        auto lwt = m_connect_options->get_last_will();
        if(lwt) {
            auto err = mosquitto_will_set(m_inst, lwt->get_topic().c_str(), lwt->get_payload().size(), lwt->get_payload().data(), lwt->get_qos(), lwt->is_retained());
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
        }
        if(!m_connect_options->get_password().empty() || !m_connect_options->get_username().empty()) {
            auto err = mosquitto_username_pw_set(m_inst, m_connect_options->get_username().c_str(), m_connect_options->get_password().c_str());
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
        }
        if(m_connect_options->is_auto_reconnect()) {
            auto err = mosquitto_reconnect_delay_set(m_inst, 1, 60*5, true);
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
        }
        if(m_connect_options->get_version() != connect_options::version::auto_select) {
            if(m_connect_options->get_version() == connect_options::version::v5)
                throw std::logic_error("unsupported protocol version");
            int ver = m_connect_options->get_version() == connect_options::version::v31 ? MQTT_PROTOCOL_V31 : MQTT_PROTOCOL_V311;
            auto err = mosquitto_opts_set(m_inst, MOSQ_OPT_PROTOCOL_VERSION, &ver);
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
        }
        auto err = mosquitto_max_inflight_messages_set(m_inst, m_connect_options->get_max_in_flight());
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        for(auto& s : m_connect_options->get_serveruris()) {
            if(m_on_log)
                m_on_log(ttl::loglevel::DEBUG, "Connecting to " + s.first + ":" + std::to_string(s.second));
            err = mosquitto_connect(m_inst, s.first.c_str(), s.second, m_connect_options->get_keepalive_interval());
            if(err == MOSQ_ERR_ERRNO) {
                if(errno == ECONNREFUSED || errno == ENETUNREACH || errno == ETIMEDOUT) {
                    if(m_on_log)
                        m_on_log(ttl::loglevel::DEBUG, "Connection to " + s.first + ":" + std::to_string(s.second) + " failed: " + strerror(errno));
                    continue;
                }
                throw std::system_error(errno, std::system_category());
            }
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
            m_current_uri = s.first + ":" + std::to_string(s.second);
        }
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(errno, std::system_category());
        err = mosquitto_loop_start(m_inst);
        if(err != MOSQ_ERR_SUCCESS) throw std::system_error(err, get_mosq_error_category());
    }
    void client::disconnect() {
        auto err = mosquitto_disconnect(m_inst);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        mosquitto_loop_stop(m_inst, true);
        mosquitto_destroy(m_inst);
        m_inst = nullptr;
    }
    const std::string& client::get_clientid() const {
        return m_connect_options->get_clientid();
    }
    const std::string& client::get_serveruri() const {
        return m_current_uri;
    }
    bool client::is_connected() const {
        return m_connected;
    }
    void client::publish(std::string topic, std::string payload, int qos, bool retained) {
        std::unique_lock<std::mutex> lck(m_inflight_mtx);
        int mid;
        auto err = mosquitto_publish(m_inst, &mid,
            topic.c_str(),
            payload.size(),
            payload.data(),
            qos,
            retained);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        this->await_msg(mid, lck);
    }
    void client::publish(message_ptr msg) {
        std::unique_lock<std::mutex> lck(m_inflight_mtx);
        int mid;
        auto err = mosquitto_publish(m_inst, &mid,
            msg->get_topic().c_str(),
            msg->get_payload().size(),
            msg->get_payload().data(),
            msg->get_qos(),
            msg->is_retained());
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        this->await_msg(mid, lck);
    }
    void client::reconnect() {
        auto err = mosquitto_reconnect(m_inst);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
    }
    void client::subscribe(const std::string& topic) {
        this->subscribe({ topic });
    }
    void client::subscribe(const std::vector<std::string>& topics) {
        std::vector<std::pair<std::string, int>> res;
        for(auto& t : topics) res.push_back({ t, 1 });
        this->subscribe(res);
    }
    void client::subscribe(const std::string& topic, int qos) {
        std::vector<std::pair<std::string, int>> res;
        res.push_back(std::pair<std::string, int>(topic, qos));
        this->subscribe(res);
    }
    void client::subscribe(const std::vector<std::pair<std::string, int>>& topics) {
        for(auto& e: topics) {
            std::unique_lock<std::mutex> lck(m_inflight_mtx);
            int mid;
            auto err = mosquitto_subscribe(m_inst, &mid, e.first.c_str(), e.second);
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
            this->await_msg(mid, lck);
        }
    }
    void client::unsubscribe(const std::string& topic) {
        this->unsubscribe({ topic });
    }
    void client::unsubscribe(const std::vector<std::string>& topics) {
        for(auto& e: topics) {
            std::unique_lock<std::mutex> lck(m_inflight_mtx);
            int mid;
            auto err = mosquitto_unsubscribe(m_inst, &mid, e.c_str());
            if(err != MOSQ_ERR_SUCCESS)
                throw std::system_error(err, get_mosq_error_category());
            this->await_msg(mid, lck);
        }
    }
    void client::set_on_message(on_message_cb_t cb) {
        m_on_msg = cb;
    }
    void client::set_on_log(on_log_cb_t cb) {
        m_on_log = cb;
    }

    abstract_client_ptr create_client() {
        return std::make_shared<client>();
    }
}
}