#include <gtest/gtest.h>
#include <homie-cpp/client.h>

using namespace homie;

namespace {
	struct message_step {
		std::vector<std::pair<std::string, std::string>> expected_messages;

		bool is_ok(const std::string& topic, const std::string& payload) {
			for (auto it = expected_messages.begin(); it != expected_messages.end(); it++) {
				if (it->first == topic && it->second == payload) {
					expected_messages.erase(it);
					return true;
				}
			}
			return false;
		}

		bool done() {
			return expected_messages.empty();
		}

		message_step& add_message(const std::string& topic, const std::string& payload) {
			expected_messages.push_back({ topic, payload });
			return *this;
		}
	};

	struct test_mqtt_client : public homie::mqtt_client {
		homie::mqtt_event_handler* handler;

		std::vector<message_step> steps;

		std::set<std::string> expect_subscribe;
		std::set<std::string> expect_unsubscribe;

		bool open_called = false;

		// Geerbt über mqtt_connection
		virtual void set_event_handler(homie::mqtt_event_handler * evt) override
		{
			handler = evt;
		}

		virtual void open(const std::string& will_topic, const std::string& will_payload, int will_qos, bool will_retain) override {
			open_called = true;

			if (handler)
				handler->on_connect(false, false);
		}

		virtual void open() override {
			FAIL();
		}

		virtual void publish(const std::string & topic, const std::string & payload, int qos, bool retain) override
		{
			ASSERT_FALSE(steps.empty());
			ASSERT_TRUE(steps.begin()->is_ok(topic, payload));
			if (steps.begin()->done())
				steps.erase(steps.begin());
		}

		virtual void subscribe(const std::string & topic, int qos) override
		{
			ASSERT_TRUE(expect_subscribe.count(topic) != 0);
			expect_subscribe.erase(topic);
		}

		virtual void unsubscribe(const std::string & topic) override
		{
			ASSERT_TRUE(expect_unsubscribe.count(topic) != 0);
			expect_unsubscribe.erase(topic);
		}

		virtual bool is_connected() const override
		{
			return false;
		}

		message_step& add_step() {
			steps.push_back({});
			return steps.back();
		}
	};

	struct test_property : public homie::basic_property {
		std::string value = "100";
		std::map<std::string, std::string> attributes;
		std::weak_ptr<homie::node> node;

		test_property(std::weak_ptr<homie::node> ptr)
			: node(ptr)
		{
			attributes["name"] = "Intensity";
			attributes["settable"] = "true";
			attributes["unit"] = "%";
			attributes["datatype"] = "integer";
			attributes["format"] = "0:100";
		}

		virtual node_ptr get_node() { return node.lock(); }
		virtual const_node_ptr get_node() const { return node.lock(); }

		virtual std::string get_id() const {
			return "intensity";
		}

		virtual std::string get_value(int64_t node_idx) const { return std::to_string(std::stoi(value) - node_idx); }
		virtual void set_value(int64_t node_idx, const std::string& value) { this->value = value; }
		virtual std::string get_value() const { return value; }
		virtual void set_value(const std::string& value) { this->value = value; }

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
		virtual device_ptr get_device() override {
			return device.lock();
		}
		virtual const_device_ptr get_device() const override {
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
		virtual property_ptr get_property(const std::string& id) override
		{
			return properties.count(id) ? properties.at(id) : nullptr;
		}
		virtual const_property_ptr get_property(const std::string& id) const override
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
				if (e.first.first == idx)
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

	struct test_node_array : public test_node {
		test_node_array(std::weak_ptr<homie::device> dev)
			:test_node(dev)
		{
			attributes["array"] = "1-3";
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
		virtual node_ptr get_node(const std::string& id) override
		{
			return nodes.count(id) ? nodes.at(id) : nullptr;
		}
		virtual const_node_ptr get_node(const std::string& id) const override
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

}

TEST(ClientTest, Init) {

	test_mqtt_client test_client;
	test_client.expect_subscribe.insert("homie/testdevice/+/+/set");
	test_client.expect_unsubscribe.insert("homie/testdevice/+/+/set");
	test_client.add_step().add_message("homie/testdevice/$state", "init");
	test_client.add_step()
		.add_message("homie/testdevice/$homie", "3.0.0")
		.add_message("homie/testdevice/$name", "Testdevice")
		.add_message("homie/testdevice/$localip", "10.0.0.1")
		.add_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF")
		.add_message("homie/testdevice/$fw/name", "Firmwarename")
		.add_message("homie/testdevice/$fw/version", "0.0.1")
		.add_message("homie/testdevice/$nodes", "")
		.add_message("homie/testdevice/$implementation", "homie-cpp")
		.add_message("homie/testdevice/$stats", "uptime")
		.add_message("homie/testdevice/$stats/interval", "60")
		.add_message("homie/testdevice/$stats/uptime", "0");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		homie::client client(test_client, std::make_shared<test_device>());
	}

	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(ClientTest, InitWithNode) {

	test_mqtt_client test_client;
	test_client.expect_subscribe.insert("homie/testdevice/+/+/set");
	test_client.expect_unsubscribe.insert("homie/testdevice/+/+/set");
	test_client.add_step().add_message("homie/testdevice/$state", "init");
	test_client.add_step()
		.add_message("homie/testdevice/$homie", "3.0.0")
		.add_message("homie/testdevice/$name", "Testdevice")
		.add_message("homie/testdevice/$localip", "10.0.0.1")
		.add_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF")
		.add_message("homie/testdevice/$fw/name", "Firmwarename")
		.add_message("homie/testdevice/$fw/version", "0.0.1")
		.add_message("homie/testdevice/$nodes", "testnode")
		.add_message("homie/testdevice/$implementation", "homie-cpp")
		.add_message("homie/testdevice/$stats", "uptime")
		.add_message("homie/testdevice/$stats/interval", "60")
		.add_message("homie/testdevice/$stats/uptime", "0")
		.add_message("homie/testdevice/testnode/$name", "Testnode")
		.add_message("homie/testdevice/testnode/$type", "light")
		.add_message("homie/testdevice/testnode/$properties", "");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		auto dev = std::make_shared<test_device>();
		dev->add_node(std::make_shared<test_node>(dev));
		homie::client client(test_client, dev);
	}

	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(ClientTest, InitWithNodeAndProperty) {

	test_mqtt_client test_client;
	test_client.expect_subscribe.insert("homie/testdevice/+/+/set");
	test_client.expect_unsubscribe.insert("homie/testdevice/+/+/set");
	test_client.add_step().add_message("homie/testdevice/$state", "init");
	test_client.add_step()
		.add_message("homie/testdevice/$homie", "3.0.0")
		.add_message("homie/testdevice/$name", "Testdevice")
		.add_message("homie/testdevice/$localip", "10.0.0.1")
		.add_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF")
		.add_message("homie/testdevice/$fw/name", "Firmwarename")
		.add_message("homie/testdevice/$fw/version", "0.0.1")
		.add_message("homie/testdevice/$nodes", "testnode")
		.add_message("homie/testdevice/$implementation", "homie-cpp")
		.add_message("homie/testdevice/$stats", "uptime")
		.add_message("homie/testdevice/$stats/interval", "60")
		.add_message("homie/testdevice/$stats/uptime", "0")
		.add_message("homie/testdevice/testnode/$name", "Testnode")
		.add_message("homie/testdevice/testnode/$type", "light")
		.add_message("homie/testdevice/testnode/$properties", "intensity")
		.add_message("homie/testdevice/testnode/intensity", "100")
		.add_message("homie/testdevice/testnode/intensity/$name", "Intensity")
		.add_message("homie/testdevice/testnode/intensity/$settable", "true")
		.add_message("homie/testdevice/testnode/intensity/$unit", "%")
		.add_message("homie/testdevice/testnode/intensity/$datatype", "integer")
		.add_message("homie/testdevice/testnode/intensity/$format", "0:100");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		auto dev = std::make_shared<test_device>();
		auto node = std::make_shared<test_node>(dev);
		dev->add_node(node);
		node->add_property(std::make_shared<test_property>(node));
		homie::client client(test_client, dev);
	}

	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(ClientTest, InitWithNodeArrayAndProperty) {

	test_mqtt_client test_client;
	test_client.expect_subscribe.insert("homie/testdevice/+/+/set");
	test_client.expect_unsubscribe.insert("homie/testdevice/+/+/set");
	test_client.add_step().add_message("homie/testdevice/$state", "init");
	test_client.add_step()
		.add_message("homie/testdevice/$homie", "3.0.0")
		.add_message("homie/testdevice/$name", "Testdevice")
		.add_message("homie/testdevice/$localip", "10.0.0.1")
		.add_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF")
		.add_message("homie/testdevice/$fw/name", "Firmwarename")
		.add_message("homie/testdevice/$fw/version", "0.0.1")
		.add_message("homie/testdevice/$nodes", "testnode[]")
		.add_message("homie/testdevice/$implementation", "homie-cpp")
		.add_message("homie/testdevice/$stats", "uptime")
		.add_message("homie/testdevice/$stats/interval", "60")
		.add_message("homie/testdevice/$stats/uptime", "0")
		.add_message("homie/testdevice/testnode/$name", "Testnode")
		.add_message("homie/testdevice/testnode/$type", "light")
		.add_message("homie/testdevice/testnode/$properties", "intensity")
		.add_message("homie/testdevice/testnode/$array", "1-3")
		.add_message("homie/testdevice/testnode/intensity/$name", "Intensity")
		.add_message("homie/testdevice/testnode/intensity/$settable", "true")
		.add_message("homie/testdevice/testnode/intensity/$unit", "%")
		.add_message("homie/testdevice/testnode/intensity/$datatype", "integer")
		.add_message("homie/testdevice/testnode/intensity/$format", "0:100")
		.add_message("homie/testdevice/testnode_1/intensity", "99")
		.add_message("homie/testdevice/testnode_2/intensity", "98")
		.add_message("homie/testdevice/testnode_3/intensity", "97");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		auto dev = std::make_shared<test_device>();
		auto node = std::make_shared<test_node_array>(dev);
		dev->add_node(node);
		node->add_property(std::make_shared<test_property>(node));
		homie::client client(test_client, dev);
	}

	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(ClientTest, ChangesGetPublished) {

	test_mqtt_client test_client;
	test_client.expect_subscribe.insert("homie/testdevice/+/+/set");
	test_client.expect_unsubscribe.insert("homie/testdevice/+/+/set");
	test_client.add_step().add_message("homie/testdevice/$state", "init");
	test_client.add_step()
		.add_message("homie/testdevice/$homie", "3.0.0")
		.add_message("homie/testdevice/$name", "Testdevice")
		.add_message("homie/testdevice/$localip", "10.0.0.1")
		.add_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF")
		.add_message("homie/testdevice/$fw/name", "Firmwarename")
		.add_message("homie/testdevice/$fw/version", "0.0.1")
		.add_message("homie/testdevice/$nodes", "testnode[]")
		.add_message("homie/testdevice/$implementation", "homie-cpp")
		.add_message("homie/testdevice/$stats", "uptime")
		.add_message("homie/testdevice/$stats/interval", "60")
		.add_message("homie/testdevice/$stats/uptime", "0")
		.add_message("homie/testdevice/testnode/$name", "Testnode")
		.add_message("homie/testdevice/testnode/$type", "light")
		.add_message("homie/testdevice/testnode/$properties", "intensity")
		.add_message("homie/testdevice/testnode/$array", "1-3")
		.add_message("homie/testdevice/testnode/intensity/$name", "Intensity")
		.add_message("homie/testdevice/testnode/intensity/$settable", "true")
		.add_message("homie/testdevice/testnode/intensity/$unit", "%")
		.add_message("homie/testdevice/testnode/intensity/$datatype", "integer")
		.add_message("homie/testdevice/testnode/intensity/$format", "0:100")
		.add_message("homie/testdevice/testnode_1/intensity", "99")
		.add_message("homie/testdevice/testnode_2/intensity", "98")
		.add_message("homie/testdevice/testnode_3/intensity", "97");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step()
		.add_message("homie/testdevice/testnode_1/intensity", "19")
		.add_message("homie/testdevice/testnode_2/intensity", "18")
		.add_message("homie/testdevice/testnode_3/intensity", "17");
	test_client.add_step().add_message("homie/testdevice/testnode_1/intensity", "8");
	test_client.add_step().add_message("homie/testdevice/testnode_2/intensity", "6");
	test_client.add_step().add_message("homie/testdevice/testnode_3/intensity", "4");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		auto dev = std::make_shared<test_device>();
		auto node = std::make_shared<test_node_array>(dev);
		dev->add_node(node);
		node->add_property(std::make_shared<test_property>(node));
		homie::client client(test_client, dev);

		node->properties.begin()->second->set_value("20");
		client.notify_property_changed(node->get_id(), "intensity");

		node->properties.begin()->second->set_value("9");
		client.notify_property_changed(node->get_id(), "intensity", 1);

		node->properties.begin()->second->set_value("8");
		client.notify_property_changed(node->get_id(), "intensity", 2);

		node->properties.begin()->second->set_value("7");
		client.notify_property_changed(node->get_id(), "intensity", 3);
	}

	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}