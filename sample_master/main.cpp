#include <iostream>
#include <string>
#include "mqtt_client.h"
#include <atomic>
#include <thread>
#include "..\homie-cpp\include\homie-cpp\master.h"
#include "console.h"

int main(int argc, char** argv) try {
	std::string brokerip = "127.0.0.1";
	std::string username;
	std::string password;
	std::string basetopic = "test/";
	std::string clientid = "sample_master";

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			if (i == argc - 1) throw std::runtime_error("Missing argument to -h");
			brokerip = argv[++i];
		}
		else if (strcmp(argv[i], "-u") == 0) {
			if (i == argc - 1) throw std::runtime_error("Missing argument to -u");
			username = argv[++i];
		}
		else if (strcmp(argv[i], "-p") == 0) {
			if (i == argc - 1) throw std::runtime_error("Missing argument to -p");
			password = argv[++i];
		}
		else if (strcmp(argv[i], "-t") == 0) {
			if (i == argc - 1) throw std::runtime_error("Missing argument to -t");
			basetopic = argv[++i];
		}
		else if (strcmp(argv[i], "-c") == 0) {
			if (i == argc - 1) throw std::runtime_error("Missing argument to -c");
			clientid = argv[++i];
		}
	}

	mqtt_client c(brokerip, username, password, clientid);
	homie::master hc(c, basetopic);

	std::atomic<bool> exit = false;
	std::thread th([&]() {
		console con(hc);
		con.run();
		exit = true;
	});
	while (!exit) {
		c.loop();
	}
	if (th.joinable())
		th.join();
}
catch (const std::exception& e) {
	std::cerr << "Error:" << e.what() << std::endl;
}