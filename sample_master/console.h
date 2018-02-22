#pragma once
#include "..\homie-cpp\include\homie-cpp\master.h"

class console {
	homie::device_ptr device;
	homie::node_ptr node;
	homie::property_ptr property;
	homie::master& master;

	int handle_cmd(const std::string& cmd);

	int handle_cmd(const std::string& cmd, const std::string& arg);

	int handle_cmd(const std::string& cmd, const std::string& arg1, const std::string& arg2);

public:
	console(homie::master& m)
		:master(m)
	{}

	void run();
};