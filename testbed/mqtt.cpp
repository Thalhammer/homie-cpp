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
    std::string connect_options::generate_random_clientid(size_t maxlen) {
        static constexpr const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::string res("random");
        res.resize(maxlen);
        for (size_t i = 6; i < maxlen; i++) {
            res[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        return res;
    }

    class async_client : public abstract_async_client {
        mosquitto* m_inst = nullptr;
        connect_options_ptr m_connect_options = nullptr;
        std::string m_current_uri;

        std::mutex m_mtx;
        std::condition_variable m_cv;

        std::recursive_mutex m_cb_mtx;
        int m_connect_result = -1;
        on_message_cb_t m_on_message;
        on_log_cb_t m_on_log;
        on_connect_cb_t m_on_connect;
        on_disconnect_cb_t m_on_disconnect;
        on_publish_cb_t m_on_publish;
        on_subscribe_cb_t m_on_subscribe;
        on_unsubscribe_cb_t m_on_unsubscribe;

        std::set<int> m_inflight_list;

        void await_msg(int mid, std::unique_lock<std::mutex>& lck);
    public:
        async_client();
        ~async_client();
        void connect(const std::string& uri, bool blocking = false) override;
        void connect(const_connect_options_ptr opts, bool blocking = false) override;
        void disconnect() override;
        const std::string& get_clientid() const override;
        const std::string& get_serveruri() const override;
        bool is_connected() const override;
        void publish(std::string topic, std::string payload, int qos, bool retained, int* mid = nullptr) override;
        void publish(message_ptr msg, bool wait) override;
        void reconnect() override;
        void subscribe(const std::string& topic, int* mid = nullptr) override;
        void subscribe(const std::string& topic, int qos, int* mid = nullptr) override;
        void unsubscribe(const std::string& topic, int* mid = nullptr) override;
        
        void set_on_log(on_log_cb_t cb) override;
        void set_on_connect(on_connect_cb_t) override;
        void set_on_disconnect(on_disconnect_cb_t) override;
        void set_on_publish(on_publish_cb_t) override;
        void set_on_message(on_message_cb_t) override;
        void set_on_subscribe(on_subscribe_cb_t) override;
        void set_on_unsubscribe(on_unsubscribe_cb_t) override;
    };

    class client : public abstract_client {
        async_client m_client;
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

    void async_client::await_msg(int mid, std::unique_lock<std::mutex>& lck) {
        m_inflight_list.insert(mid);
        while(m_inflight_list.count(mid) != 0) {
            m_cv.wait(lck);
        }
        m_inflight_list.erase(mid);
    }
    async_client::async_client() {
        if(mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
            throw std::runtime_error("mosquitto_lib_init failed");
    }
    async_client::~async_client() {
        if(m_inst) {
            mosquitto_disconnect(m_inst);
            mosquitto_loop_stop(m_inst, true);
            mosquitto_destroy(m_inst);
            m_inst = nullptr;
        }
        mosquitto_lib_cleanup();
    }
    void async_client::connect(const std::string& uri, bool blocking) {
        auto opts = std::make_shared<connect_options>();
        opts->set_serveruris({{uri, 1883}});
        opts->set_clientid(connect_options::generate_random_clientid());
        this->connect(opts, blocking);
    }
    void async_client::connect(const_connect_options_ptr opts, bool blocking) {
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
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
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
            auto inst = reinterpret_cast<async_client*>(arg);
            inst->m_connect_result = -1;
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Disconnected " + std::to_string(rc));
            if(inst->m_on_disconnect)
                inst->m_on_disconnect(rc);
        });
        mosquitto_connect_callback_set(m_inst, [](mosquitto*, void* arg, int rc){
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            std::unique_lock<std::mutex> lck(inst->m_mtx);
            inst->m_connect_result = rc;
            inst->m_cv.notify_all();
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, std::string("Connect callback ") + mosquitto_connack_string(rc));
            if(inst->m_on_connect)
                inst->m_on_connect(rc);
        });
        mosquitto_publish_callback_set(m_inst, [](mosquitto*, void* arg, int mid){
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_cv.notify_all();
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Published " + std::to_string(mid));
            if(inst->m_on_publish)
                inst->m_on_publish(mid);
        });
        mosquitto_subscribe_callback_set(m_inst, [](mosquitto*, void* arg, int mid, int qos_count, const int* granted_qos){
            (void)qos_count;
            (void)granted_qos;
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_cv.notify_all();
            if(qos_count < 1) return;
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Subscribed " + std::to_string(mid) + " granted_qos=" + std::to_string(*granted_qos));
            if(inst->m_on_subscribe)
                inst->m_on_subscribe(mid, *granted_qos);
        });
        mosquitto_unsubscribe_callback_set(m_inst, [](mosquitto*, void* arg, int mid){
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::mutex> lck(inst->m_mtx);
            inst->m_inflight_list.erase(mid);
            inst->m_cv.notify_all();
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, "Unsubscribed " + std::to_string(mid));
            if(inst->m_on_unsubscribe)
                inst->m_on_unsubscribe(mid);
        });
        mosquitto_message_callback_set(m_inst, [](mosquitto*, void* arg, const struct mosquitto_message* msg){
            auto inst = reinterpret_cast<async_client*>(arg);
            std::unique_lock<std::recursive_mutex> cb_lck(inst->m_cb_mtx);
            if(inst->m_on_log)
                inst->m_on_log(ttl::loglevel::DEBUG, std::string("Message \"") + msg->topic + "\": " + std::to_string(msg->payloadlen) + " bytes");
            if(inst->m_on_message) {
                auto nmsg = std::make_shared<message>();
                nmsg->set_id(msg->mid);
                nmsg->set_topic(msg->topic);
                nmsg->set_qos(msg->qos);
                nmsg->set_retained(msg->retain);
                nmsg->set_duplicate(false);
                nmsg->set_payload(std::string(reinterpret_cast<const char*>(msg->payload), msg->payloadlen));
                inst->m_on_message(nmsg);
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
            if(blocking) err = mosquitto_connect(m_inst, s.first.c_str(), s.second, m_connect_options->get_keepalive_interval());
            else err = mosquitto_connect_async(m_inst, s.first.c_str(), s.second, m_connect_options->get_keepalive_interval());
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
        {
            std::unique_lock<std::mutex> lck(m_mtx);
            while(m_connect_result == -1) {
                m_cv.wait(lck);
            }
        }
    }
    void async_client::disconnect() {
        auto err = mosquitto_disconnect(m_inst);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        mosquitto_loop_stop(m_inst, true);
        mosquitto_destroy(m_inst);
        m_inst = nullptr;
    }
    const std::string& async_client::get_clientid() const {
        return m_connect_options->get_clientid();
    }
    const std::string& async_client::get_serveruri() const {
        return m_current_uri;
    }
    bool async_client::is_connected() const {
        return m_connect_result == 0;
    }
    void async_client::publish(std::string topic, std::string payload, int qos, bool retained, int* mid) {
        std::unique_lock<std::mutex> lck(m_mtx);
        int pmid;
        auto err = mosquitto_publish(m_inst, &pmid,
            topic.c_str(),
            payload.size(),
            payload.data(),
            qos,
            retained);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        if(mid) *mid = pmid;
        else this->await_msg(pmid, lck);
    }
    void async_client::publish(message_ptr msg, bool wait) {
        std::unique_lock<std::mutex> lck(m_mtx);
        int mid;
        auto err = mosquitto_publish(m_inst, &mid,
            msg->get_topic().c_str(),
            msg->get_payload().size(),
            msg->get_payload().data(),
            msg->get_qos(),
            msg->is_retained());
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        msg->set_id(mid);
        if(wait) this->await_msg(mid, lck);
    }
    void async_client::reconnect() {
        auto err = mosquitto_reconnect(m_inst);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
    }
    void async_client::subscribe(const std::string& topic, int* mid) {
        this->subscribe(topic, 1, mid);
    }
    void async_client::subscribe(const std::string& topic, int qos, int* mid) {
        std::unique_lock<std::mutex> lck(m_mtx);
        int pmid;
        auto err = mosquitto_subscribe(m_inst, &pmid, topic.c_str(), qos);
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        if(mid) *mid = pmid;
        else this->await_msg(pmid, lck);
    }
    void async_client::unsubscribe(const std::string& topic, int* mid) {
        std::unique_lock<std::mutex> lck(m_mtx);
        int pmid;
        auto err = mosquitto_unsubscribe(m_inst, &pmid, topic.c_str());
        if(err != MOSQ_ERR_SUCCESS)
            throw std::system_error(err, get_mosq_error_category());
        if(mid) *mid = pmid;
        else this->await_msg(pmid, lck);
    }
    void async_client::set_on_message(on_message_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_message = cb;
    }
    void async_client::set_on_log(on_log_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_log = cb;
    }
    void async_client::set_on_connect(on_connect_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_connect = cb;
    }
    void async_client::set_on_disconnect(on_disconnect_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_disconnect = cb;
    }
    void async_client::set_on_publish(on_publish_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_publish = cb;
    }
    void async_client::set_on_subscribe(on_subscribe_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_subscribe = cb;
    }
    void async_client::set_on_unsubscribe(on_unsubscribe_cb_t cb) {
        std::unique_lock<std::recursive_mutex> cb_lck(m_cb_mtx);
        m_on_unsubscribe = cb;
    }

    client::client() {
    }
    client::~client() {
    }
    void client::connect(const std::string& uri) {
        m_client.connect(uri, true);
    }
    void client::connect(const_connect_options_ptr opts) {
        m_client.connect(opts, true);
    }
    void client::disconnect() {
        m_client.disconnect();
    }
    const std::string& client::get_clientid() const {
        return m_client.get_clientid();
    }
    const std::string& client::get_serveruri() const {
        return m_client.get_serveruri();
    }
    bool client::is_connected() const {
        return m_client.is_connected();
    }
    void client::publish(std::string topic, std::string payload, int qos, bool retained) {
        m_client.publish(topic, payload, qos, retained);
    }
    void client::publish(message_ptr msg) {
        m_client.publish(msg, true);
    }
    void client::reconnect() {
        m_client.reconnect();
    }
    void client::subscribe(const std::string& topic) {
        m_client.subscribe(topic);
    }
    void client::subscribe(const std::vector<std::string>& topics) {
        for(auto& t : topics) {
            m_client.subscribe(t);
        }
    }
    void client::subscribe(const std::string& topic, int qos) {
        m_client.subscribe(topic, qos);
    }
    void client::subscribe(const std::vector<std::pair<std::string, int>>& topics) {
        for(auto& e: topics) {
            m_client.subscribe(e.first, e.second);
        }
    }
    void client::unsubscribe(const std::string& topic) {
        m_client.unsubscribe(topic);
    }
    void client::unsubscribe(const std::vector<std::string>& topics) {
        for(auto& e: topics) {
            m_client.unsubscribe(e);
        }
    }
    void client::set_on_message(on_message_cb_t cb) {
        m_client.set_on_message(cb);
    }
    void client::set_on_log(on_log_cb_t cb) {
        m_client.set_on_log(cb);
    }

    abstract_client_ptr create_client() {
        return std::make_shared<client>();
    }
    abstract_async_client_ptr create_async_client() {
        return std::make_shared<async_client>();
    }
}
}