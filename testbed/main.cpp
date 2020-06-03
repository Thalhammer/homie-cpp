#include <iostream>
#include <string>
#include <cstring>
#include <mosquitto.h>
#include "mqtt.h"

int main(int, char**) try {
	ttl::streamlogger log(std::cout);
	log.set_loglevel(ttl::loglevel::TRACE);
	auto client = homiecpp::mqtt::create_client();
	client->set_on_log([&log](ttl::loglevel lvl, std::string str){
		log(lvl, "mqtt") << str;
	});
	client->set_on_message([&log](homiecpp::mqtt::const_message_ptr msg){
		log(ttl::loglevel::INFO, "mqtt") << "Message(" << msg->get_topic() << "): " << msg->get_payload();
	});
	client->connect("127.0.0.1");
	log(ttl::loglevel::INFO, "main") << "Test";
	//client->publish("test/topic", "Hello World");
	//client->subscribe("test/#");

	std::cin.get();
}
catch (const std::exception& e) {
	std::cerr << "Error:" << e.what() << std::endl;
}