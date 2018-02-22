#pragma once
#include <map>
#include <iostream>
#include "..\homie-cpp\include\homie-cpp\client.h"

struct test_property : public homie::basic_property {
	std::string value = "100";
	std::map<std::string, std::string> attributes;
	std::weak_ptr<homie::node> node;
	homie::client* client;

	test_property(std::weak_ptr<homie::node> ptr)
		: node(ptr), client(nullptr)
	{
		attributes["name"] = "Testproperty";
		attributes["settable"] = "true";
		attributes["datatype"] = "string";
	}

	virtual homie::node_ptr get_node() { return node.lock(); }
	virtual homie::const_node_ptr get_node() const { return node.lock(); }

	virtual std::string get_id() const {
		return "testproperty";
	}

	virtual std::string get_value(int64_t node_idx) const { return ""; }
	virtual void set_value(int64_t node_idx, const std::string& value) { }
	virtual std::string get_value() const { return value; }
	virtual void set_value(const std::string& value) {
		this->value = value;
		std::cout << "Testproperty set to value:" << value << std::endl;
		if(client != nullptr)
			client->notify_property_changed(get_node()->get_id(), get_id());
	}

	virtual std::set<std::string> get_attributes() const override {
		std::set<std::string> res;
		for (auto& e : attributes) res.insert(e.first);
		return res;
	}
	virtual std::string get_attribute(const std::string& id) const override {
		auto it = attributes.find(id);
		if (it != attributes.cend()) return it->second;
		return "";
	}
	virtual void set_attribute(const std::string& id, const std::string& value) override {
		attributes[id] = value;
	}

};

struct test_node : public homie::basic_node {
	std::map<std::string, homie::property_ptr> properties;
	std::map<std::string, std::string> attributes;
	std::map<std::pair<int64_t, std::string>, std::string> attributes_array;
	std::weak_ptr<homie::device> device;

	test_node(std::weak_ptr<homie::device> dev)
		: device(dev)
	{
		attributes["name"] = "Testnode";
		attributes["type"] = "light";
	}

	void add_property(homie::property_ptr ptr) {
		properties.insert({ ptr->get_id(), ptr });
	}

	// Geerbt über node
	virtual homie::device_ptr get_device() override {
		return device.lock();
	}
	virtual homie::const_device_ptr get_device() const override {
		return device.lock();
	}
	virtual std::string get_id() const override
	{
		return "testnode";
	}
	virtual std::set<std::string> get_properties() const override
	{
		std::set<std::string> res;
		for (auto& e : properties) res.insert(e.first);
		return res;
	}
	virtual homie::property_ptr get_property(const std::string& id) override
	{
		return properties.count(id) ? properties.at(id) : nullptr;
	}
	virtual homie::const_property_ptr get_property(const std::string& id) const override
	{
		return properties.count(id) ? properties.at(id) : nullptr;
	}

	virtual std::set<std::string> get_attributes() const override {
		std::set<std::string> res;
		for (auto& e : attributes) res.insert(e.first);
		return res;
	}
	virtual std::set<std::string> get_attributes(int64_t idx) const override {
		std::set<std::string> res;
		for (auto& e : attributes_array)
			if(e.first.first == idx)
				res.insert(e.first.second);
		return res;
	}
	virtual std::string get_attribute(const std::string& id) const override {
		auto it = attributes.find(id);
		if (it != attributes.cend()) return it->second;
		return "";
	}
	virtual void set_attribute(const std::string& id, const std::string& value) override {
		attributes[id] = value;
	}
	virtual std::string get_attribute(const std::string& id, int64_t idx) const override {
		auto it = attributes_array.find({ idx, id });
		if (it != attributes_array.cend()) return it->second;
		return "";
	}
	virtual void set_attribute(const std::string& id, const std::string& value, int64_t idx) override {
		attributes_array[{idx, id}] = value;
	}
};

struct test_device : public homie::basic_device {
	std::map<std::string, homie::node_ptr> nodes;
	std::map<std::string, std::string> attributes;

	test_device() {
		attributes["name"] = "Testdevice";
		attributes["state"] = "ready";
		attributes["localip"] = "10.0.0.1";
		attributes["mac"] = "AA:BB:CC:DD:EE:FF";
		attributes["fw/name"] = "Firmwarename";
		attributes["fw/version"] = "0.0.1";
		attributes["implementation"] = "homie-cpp";
		attributes["stats"] = "uptime";
		attributes["stats/uptime"] = "0";
		attributes["stats/interval"] = "60";
	}

	void add_node(homie::node_ptr ptr) {
		nodes.insert({ ptr->get_id(), ptr });
	}

	// Geerbt über device
	virtual std::string get_id() const override
	{
		return "testdevice";
	}
	virtual std::set<std::string> get_nodes() const override
	{
		std::set<std::string> res;
		for (auto& e : nodes) res.insert(e.first);
		return res;
	}
	virtual homie::node_ptr get_node(const std::string& id) override
	{
		return nodes.count(id) ? nodes.at(id) : nullptr;
	}
	virtual homie::const_node_ptr get_node(const std::string& id) const override
	{
		return nodes.count(id) ? nodes.at(id) : nullptr;
	}

	virtual std::set<std::string> get_attributes() const override {
		std::set<std::string> res;
		for (auto& e : attributes) res.insert(e.first);
		return res;
	}
	virtual std::string get_attribute(const std::string& id) const {
		auto it = attributes.find(id);
		if (it != attributes.cend()) return it->second;
		return "";
	}
	virtual void set_attribute(const std::string& id, const std::string& value) {
		attributes[id] = value;
	}
};