#pragma once
#include "..\homie-cpp\include\homie-cpp\mqtt_client.h"
#include <memory>

class mqtt_client : public homie::mqtt_client {
	struct simpl;
	std::unique_ptr<simpl> impl;
public:
	mqtt_client(const std::string& host, const std::string& user, const std::string& pass, const std::string& clientid);
	~mqtt_client();
	// Geerbt über mqtt_client
	virtual void set_event_handler(homie::mqtt_event_handler * evt) override;
	virtual void open(const std::string & will_topic, const std::string & will_payload, int will_qos, bool will_retain) override;
	virtual void open() override;
	virtual void publish(const std::string & topic, const std::string & payload, int qos, bool retain) override;
	virtual void subscribe(const std::string & topic, int qos) override;
	virtual void unsubscribe(const std::string & topic) override;
	virtual bool is_connected() const override;

	void loop();
};