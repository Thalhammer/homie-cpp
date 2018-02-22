#include "mqtt_client.h"
#define WIN32 _WIN32
#define WIN64 _WIN64
#include <MQTTClient.h>

struct mqtt_client::simpl {
	homie::mqtt_event_handler *handler;
	MQTTClient client;
	std::string host;
	std::string user;
	std::string pass;
};

mqtt_client::mqtt_client(const std::string& host, const std::string& user, const std::string& pass, const std::string& clientid)
{
	impl = std::make_unique<simpl>();
	auto rc = MQTTClient_create(&impl->client, host.c_str(), clientid.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to create mqtt client");

	impl->host = host;
	impl->user = user;
	impl->pass = pass;

	if (MQTTClient_setCallbacks(impl->client, this, [](void* ctx, char* cause) {
		auto* that = reinterpret_cast<mqtt_client*>(ctx);
		that->impl->handler->on_offline();
	}, [](void* ctx, char* topic, int topiclen, MQTTClient_message* msg) {
		auto* that = reinterpret_cast<mqtt_client*>(ctx);
		std::string t = topiclen > 0 ? std::string(topic, topiclen) : std::string(topic);
		that->impl->handler->on_message(t, std::string((char*)msg->payload, msg->payloadlen));
		return int(1);
	}, nullptr) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to set callbacks");
}

mqtt_client::~mqtt_client()
{
	if(impl->handler)
		impl->handler->on_closing();
	MQTTClient_destroy(&impl->client);
	if (impl->handler)
		impl->handler->on_closed();
}

void mqtt_client::set_event_handler(homie::mqtt_event_handler * evt)
{
	impl->handler = evt;
}

void mqtt_client::open(const std::string & will_topic, const std::string & will_payload, int will_qos, bool will_retain)
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_willOptions will = MQTTClient_willOptions_initializer;
	conn_opts.username = impl->user.c_str();
	conn_opts.password = impl->pass.c_str();
	conn_opts.will = &will;
	will.payload.data = will_payload.data();
	will.payload.len = will_payload.size();
	will.qos = will_qos;
	will.retained = will_retain;
	will.topicName = will_topic.c_str();

	if (MQTTClient_connect(impl->client, &conn_opts) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to connect");

	impl->handler->on_connect(false, false);
}

void mqtt_client::open()
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_willOptions will = MQTTClient_willOptions_initializer;
	conn_opts.username = impl->user.c_str();
	conn_opts.password = impl->pass.c_str();
	conn_opts.will = nullptr;

	if (MQTTClient_connect(impl->client, &conn_opts) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to connect");

	impl->handler->on_connect(false, false);
}

void mqtt_client::publish(const std::string & topic, const std::string & payload, int qos, bool retain)
{
	if (MQTTClient_publish(impl->client, topic.c_str(), payload.size(), (void*)payload.data(), qos, retain ? 1 : 0, NULL) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to publish");
}

void mqtt_client::subscribe(const std::string & topic, int qos)
{
	if (MQTTClient_subscribe(impl->client, topic.c_str(), qos) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to subscribe");
}

void mqtt_client::unsubscribe(const std::string & topic)
{
	if (MQTTClient_unsubscribe(impl->client, topic.c_str()) != MQTTCLIENT_SUCCESS)
		throw std::runtime_error("Failed to unsubscribe");
}

bool mqtt_client::is_connected() const
{
	return MQTTClient_isConnected(impl->client);
}

void mqtt_client::loop()
{
	MQTTClient_yield();
}
