#include "console.h"
#include <iostream>

int console::handle_cmd(const std::string & cmd)
{
	if (cmd == "q" || cmd == "quit" || cmd == "exit" || cmd == ".quit" || cmd == ".q")
		return -1;
	if (cmd == "devices") {
		auto devs = master.get_discovered_devices();
		for (auto& e : devs) {
			std::cout << e->get_id() << " (" << e->get_name() << ")" << std::endl;
		}
		return 1;
	}
	else if (cmd == "dump") {
		if (property) {
			if (node->is_array()) {
				auto range = node->array_range();
				if (range.second - range.first < 10) {
					for (auto i = range.first; i <= range.second; i++)
						std::cout << "Value[" << i << "]: " << property->get_value(i) << std::endl;
				}
			}
			else
				std::cout << "Value: " << property->get_value() << std::endl;
			std::cout << "Attributes:" << std::endl;
			for (auto& att : property->get_attributes())
				std::cout << "\t" << att << " = " << property->get_attribute(att) << std::endl;
		}
		else if (node) {
			std::cout << "Properties:" << std::endl;
			for (auto& e : node->get_properties()) {
				auto p = node->get_property(e);
				std::cout << "\t" << p->get_id() << " (" << p->get_name() << ")" << std::endl;
			}
			std::cout << "Attributes:" << std::endl;
			for (auto& att : node->get_attributes())
				std::cout << "\t" << att << " = " << node->get_attribute(att) << std::endl;
		}
		else if (device) {
			std::cout << "Nodes:" << std::endl;
			for (auto& e : device->get_nodes()) {
				auto n = device->get_node(e);
				std::cout << "\t" << n->get_id() << " (" << n->get_name() << ")" << std::endl;
			}
			std::cout << "Attributes:" << std::endl;
			for (auto& att : device->get_attributes())
				std::cout << "\t" << att << " = " << device->get_attribute(att) << std::endl;
		}
		else {
			std::cout << "No device selected" << std::endl;
		}
		return 1;
	}
	else if (cmd == "back") {
		if (property) {
			property = nullptr;
			return 1;
		}
		if (node) {
			node = nullptr;
			return 1;
		}
		if (device) {
			device = nullptr;
			return 1;
		}
	}
	else if (cmd == "help") {
		std::cout << "Global commands:" << std::endl;
		std::cout << "devices               Display discovered devices" << std::endl;
		std::cout << "device <dev>          Change to device <dev>" << std::endl;
		std::cout << "dump                  Dump information about current device/node/prop" << std::endl;
		std::cout << "back                  Move one level up (i.e. property => node, node => device)" << std::endl;
		std::cout << "help                  Display this help dialog" << std::endl;
		std::cout << "exit                  Exit application" << std::endl;
		std::cout << std::endl;
		std::cout << "Device commands:" << std::endl;
		std::cout << "nodes                 Display this devices nodes" << std::endl;
		std::cout << "node <node>           Change to node <node>" << std::endl;
		std::cout << std::endl;
		std::cout << "Node commands:" << std::endl;
		std::cout << "properties            Display this nodes properties" << std::endl;
		std::cout << "property <property>   Change to property <property>" << std::endl;
		std::cout << "prop <property>       Shorthand for property" << std::endl;
		std::cout << "set <prop> <value>    Set property <prop> to <value>" << std::endl;
		std::cout << std::endl;
		std::cout << "Property commands:" << std::endl;
		std::cout << "set                   Set property value" << std::endl;
		std::cout << "property <property>   Change to property <property>" << std::endl;
		std::cout << "prop <property>       Shorthand for property" << std::endl;
		return 1;
	}
	else if (device) {
		if (cmd == "nodes") {
			for (auto& e : device->get_nodes()) {
				auto n = device->get_node(e);
				std::cout << n->get_id() << " (" << n->get_name() << ")" << std::endl;
			}
			return 1;
		}
		else if (node && cmd == "properties") {
			for (auto& e : node->get_properties()) {
				auto p = node->get_property(e);
				std::cout << p->get_id() << " (" << p->get_name() << ")" << std::endl;
			}
			return 1;
		}
	}
	return 0;
}

int console::handle_cmd(const std::string & cmd, const std::string & arg)
{
	if (cmd == "device") {
		auto t = master.get_discovered_device(arg);
		if (t == nullptr) {
			std::cout << "Device not found" << std::endl;
		}
		else {
			device = t;
			node = nullptr;
			property = nullptr;
		}
		return 1;
	}
	else if (device && cmd == "node") {
		auto n = device->get_node(arg);
		if (n == nullptr) {
			std::cout << "Node not found" << std::endl;
		}
		else {
			node = n;
			property = nullptr;
		}
		return 1;
	}
	else if (node && (cmd == "property" || cmd == "prop")) {
		auto p = node->get_property(arg);
		if (p == nullptr) {
			std::cout << "Property not found" << std::endl;
		}
		else {
			property = p;
		}
		return 1;
	}
	else if (property && (cmd == "set")) {
		property->set_value(arg);
		std::cout << "Sent value set:" << arg << std::endl;
		return 1;
	}
	return 0;
}

int console::handle_cmd(const std::string & cmd, const std::string & arg1, const std::string & arg2)
{
	if (node && cmd == "set") {
		auto p = node->get_property(arg1);
		if (p == nullptr) {
			std::cout << "Property not found" << std::endl;
		}
		else {
			p->set_value(arg2);
			std::cout << "Sent value set:" << arg2 << std::endl;
		}
		return 1;
	}
	return 0;
}

void console::run()
{

	std::string line;
	std::cout << " > ";
	while (std::getline(std::cin, line)) {
		if (!line.empty()) {
			auto parts = homie::utils::split<std::string>(line, " ");
			int res = 0;
			if (parts.size() == 1) {
				res = handle_cmd(parts[0]);
			}
			else if (parts.size() == 2) {
				res = handle_cmd(parts[0], parts[1]);
			}
			else if (parts.size() == 3) {
				res = handle_cmd(parts[0], parts[1], parts[2]);
			}
			if (res < 0) break;
			if (res == 0) {
				std::cout << "Command not found" << std::endl;
			}
		}
		if (device)
			std::cout << "/" << device->get_id();
		if (node)
			std::cout << "/" << node->get_id();
		if (property)
			std::cout << "/" << property->get_id();

		std::cout << " > ";
		std::cout.flush();
	}
}
