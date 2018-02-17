#include <gtest/gtest.h>
#include <homie-cpp/client.h>

using namespace homie;

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

struct test_mqtt_client : public homie::mqtt_connection {
	homie::mqtt_event_handler* handler;

	std::vector<message_step> steps;

	std::set<std::string> expect_subscribe;
	std::set<std::string> expect_unsubscribe;

	// Geerbt über mqtt_connection
	virtual void set_event_handler(homie::mqtt_event_handler * evt) override
	{
		handler = evt;
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

struct test_property : public homie::property {
	std::string value = "100";

	virtual std::string get_id() const {
		return "intensity";
	}
	virtual std::string get_name() const {
		return "Intensity";
	}
	virtual bool is_settable() const {
		return true;
	}
	virtual std::string get_unit() const {
		return "%";
	}
	virtual datatype get_datatype() const {
		return datatype::integer;
	}
	virtual std::string get_format() const {
		return "0:100";
	}

	virtual std::string get_value(int64_t node_idx) const { return std::to_string(std::stoi(value)-node_idx); }
	virtual void set_value(int64_t node_idx, const std::string& value) { this->value = value; }
	virtual std::string get_value() const { return value; }
	virtual void set_value(const std::string& value) { this->value = value; }
};

struct test_node : public homie::node {
	std::set<property_ptr> properties;

	// Geerbt über node
	virtual std::string get_id() const override
	{
		return "testnode";
	}
	virtual std::string get_name() const override
	{
		return "Testnode";
	}
	virtual std::string get_name(int64_t node_idx) const override
	{
		return "Testnode";
	}
	virtual std::string get_type() const override
	{
		return "light";
	}
	virtual bool is_array() const override
	{
		return false;
	}
	virtual std::pair<int64_t, int64_t> array_range() const override
	{
		return{ 0,0 };
	}
	virtual std::set<const_property_ptr> get_properties() const override
	{
		return std::set<const_property_ptr>(properties.cbegin(), properties.cend());
	}
	virtual std::set<property_ptr> get_properties() override
	{
		return properties;
	}
};

struct test_node_array : public test_node {
	virtual std::string get_name(int64_t node_idx) const override
	{
		return "Testnode" + std::to_string(node_idx);
	}
	virtual bool is_array() const override
	{
		return true;
	}
	virtual std::pair<int64_t, int64_t> array_range() const override
	{
		return{ 1,3 };
	}
};

struct test_device : public homie::device {
	std::set<homie::node_ptr> nodes;

	// Geerbt über device
	virtual std::string get_id() const override
	{
		return "testdevice";
	}
	virtual std::string get_name() const override
	{
		return "Testdevice";
	}
	virtual device_state get_state() const override
	{
		return device_state::ready;
	}
	virtual std::string get_localip() const override
	{
		return "10.0.0.1";
	}
	virtual std::string get_mac() const override
	{
		return "AA:BB:CC:DD:EE:FF";
	}
	virtual const_firmware_info_ptr get_firmware() const override
	{
		struct test_fw : public homie::firmware_info {
			virtual std::string get_name() const {
				return "Firmwarename";
			}
			virtual std::string get_version() const {
				return "0.0.1";
			}
		};
		return std::make_shared<test_fw>();
	}
	virtual std::set<const_node_ptr> get_nodes() const override
	{
		return std::set<const_node_ptr>(nodes.cbegin(), nodes.cend());
	}
	virtual std::set<node_ptr> get_nodes() override
	{
		return nodes;
	}
	virtual std::string get_implementation() const override
	{
		return "homie-cpp";
	}
	virtual std::set<const_status_ptr> get_stats() const override
	{
		struct uptime_status : public homie::status {
			virtual std::string get_id() const {
				return "uptime";
			}
			virtual std::string get_value() const {
				return "0";
			}
		};
		return{ std::make_shared<uptime_status>() };
	}
	virtual std::chrono::milliseconds get_stats_interval() const override
	{
		return std::chrono::milliseconds(60000);
	}
};

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
		homie::client client(test_client);
		client.add_device(std::make_shared<test_device>());
	}

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
		dev->nodes.insert(std::make_shared<test_node>());
		homie::client client(test_client);
		client.add_device(dev);
	}

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
		auto node = std::make_shared<test_node>();
		dev->nodes.insert(node);
		node->properties.insert(std::make_shared<test_property>());
		homie::client client(test_client);
		client.add_device(dev);
	}

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
		.add_message("homie/testdevice/testnode_3/intensity", "97")
		.add_message("homie/testdevice/testnode_1/$name", "Testnode1")
		.add_message("homie/testdevice/testnode_2/$name", "Testnode2")
		.add_message("homie/testdevice/testnode_3/$name", "Testnode3");
	test_client.add_step().add_message("homie/testdevice/$state", "ready");
	test_client.add_step().add_message("homie/testdevice/$state", "disconnected");

	{
		auto dev = std::make_shared<test_device>();
		auto node = std::make_shared<test_node_array>();
		dev->nodes.insert(node);
		node->properties.insert(std::make_shared<test_property>());
		homie::client client(test_client);
		client.add_device(dev);
	}

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
		.add_message("homie/testdevice/testnode_3/intensity", "97")
		.add_message("homie/testdevice/testnode_1/$name", "Testnode1")
		.add_message("homie/testdevice/testnode_2/$name", "Testnode2")
		.add_message("homie/testdevice/testnode_3/$name", "Testnode3");
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
		auto node = std::make_shared<test_node_array>();
		dev->nodes.insert(node);
		node->properties.insert(std::make_shared<test_property>());
		homie::client client(test_client);
		client.add_device(dev);

		(*node->properties.begin())->set_value("20");
		client.notify_property_changed(dev->get_id(), node->get_id(), "intensity");

		(*node->properties.begin())->set_value("9");
		client.notify_property_changed(dev->get_id(), node->get_id(), "intensity", 1);

		(*node->properties.begin())->set_value("8");
		client.notify_property_changed(dev->get_id(), node->get_id(), "intensity", 2);

		(*node->properties.begin())->set_value("7");
		client.notify_property_changed(dev->get_id(), node->get_id(), "intensity", 3);
	}

	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}