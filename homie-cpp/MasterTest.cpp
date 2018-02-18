#include <gtest/gtest.h>
#include <homie-cpp/master.h>

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
		bool is_manager = false;

		// Geerbt über mqtt_connection
		virtual void set_event_handler(homie::mqtt_event_handler * evt) override
		{
			handler = evt;
		}

		virtual void open(const std::string& will_topic, const std::string& will_payload, int will_qos, bool will_retain) override {
			FAIL();
		}

		virtual void open() override {
			open_called = true;
			if (handler)
				handler->on_connect(false, false);
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
			return true;
		}

		message_step& add_step() {
			steps.push_back({});
			return steps.back();
		}
	};
}

TEST(MasterTest, Init) {
	test_mqtt_client test_client;
	test_client.is_manager = true;
	test_client.expect_subscribe.insert("homie/#");
	test_client.expect_unsubscribe.insert("homie/#");

	{
		master m(test_client);
	}
	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(MasterTest, DevicePublished) {
	test_mqtt_client test_client;
	test_client.is_manager = true;
	test_client.expect_subscribe.insert("homie/#");
	test_client.expect_unsubscribe.insert("homie/#");

	{
		master m(test_client);
		test_client.handler->on_message("homie/testdevice/$state", "init");
		test_client.handler->on_message("homie/testdevice/$homie", "3.0.0");
		test_client.handler->on_message("homie/testdevice/$name", "Testdevice");
		test_client.handler->on_message("homie/testdevice/$localip", "10.0.0.1");
		test_client.handler->on_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF");
		test_client.handler->on_message("homie/testdevice/$fw/name", "Firmwarename");
		test_client.handler->on_message("homie/testdevice/$fw/version", "0.0.1");
		test_client.handler->on_message("homie/testdevice/$nodes", "");
		test_client.handler->on_message("homie/testdevice/$implementation", "homie-cpp");
		test_client.handler->on_message("homie/testdevice/$stats", "uptime");
		test_client.handler->on_message("homie/testdevice/$stats/interval", "60");
		test_client.handler->on_message("homie/testdevice/$stats/uptime", "0");
		test_client.handler->on_message("homie/testdevice/$state", "ready");

		ASSERT_EQ(1, m.get_discovered_devices().size());
		auto dev = *m.get_discovered_devices().begin();
		ASSERT_EQ(dev->get_id(), "testdevice");
		ASSERT_EQ(dev->get_name(), "Testdevice");
		ASSERT_EQ(dev->get_localip(), "10.0.0.1");
		ASSERT_EQ(dev->get_mac(), "AA:BB:CC:DD:EE:FF");
		ASSERT_EQ(dev->get_firmware_name(), "Firmwarename");
		ASSERT_EQ(dev->get_firmware_version(), "0.0.1");
		ASSERT_EQ(dev->get_implementation(), "homie-cpp");
		ASSERT_EQ(dev->get_stats().size(), 1);
		ASSERT_EQ(*dev->get_stats().begin(), "uptime");
		ASSERT_EQ(dev->get_stat("uptime"), "0");
		ASSERT_EQ(dev->get_stats_interval().count(), 60);
		ASSERT_EQ(dev->get_state(), device_state::ready);
		ASSERT_TRUE(dev->get_nodes().empty());
	}
	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(MasterTest, DeviceNodesPublished) {
	test_mqtt_client test_client;
	test_client.is_manager = true;
	test_client.expect_subscribe.insert("homie/#");
	test_client.expect_unsubscribe.insert("homie/#");

	{
		master m(test_client);
		test_client.handler->on_message("homie/testdevice/$state", "init");
		test_client.handler->on_message("homie/testdevice/$homie", "3.0.0");
		test_client.handler->on_message("homie/testdevice/$name", "Testdevice");
		test_client.handler->on_message("homie/testdevice/$localip", "10.0.0.1");
		test_client.handler->on_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF");
		test_client.handler->on_message("homie/testdevice/$fw/name", "Firmwarename");
		test_client.handler->on_message("homie/testdevice/$fw/version", "0.0.1");
		test_client.handler->on_message("homie/testdevice/$nodes", "testnode");
		test_client.handler->on_message("homie/testdevice/$implementation", "homie-cpp");
		test_client.handler->on_message("homie/testdevice/$stats", "uptime");
		test_client.handler->on_message("homie/testdevice/$stats/interval", "60");
		test_client.handler->on_message("homie/testdevice/$stats/uptime", "0");
		test_client.handler->on_message("homie/testdevice/testnode/$name", "Testnode");
		test_client.handler->on_message("homie/testdevice/testnode/$type", "light");
		test_client.handler->on_message("homie/testdevice/testnode/$properties", "");
		test_client.handler->on_message("homie/testdevice/$state", "ready");

		ASSERT_EQ(1, m.get_discovered_devices().size());
		auto dev = *m.get_discovered_devices().begin();
		ASSERT_EQ(dev->get_id(), "testdevice");
		ASSERT_EQ(dev->get_name(), "Testdevice");
		ASSERT_EQ(dev->get_localip(), "10.0.0.1");
		ASSERT_EQ(dev->get_mac(), "AA:BB:CC:DD:EE:FF");
		ASSERT_EQ(dev->get_firmware_name(), "Firmwarename");
		ASSERT_EQ(dev->get_firmware_version(), "0.0.1");
		ASSERT_EQ(dev->get_implementation(), "homie-cpp");
		ASSERT_EQ(dev->get_stats().size(), 1);
		ASSERT_EQ(*dev->get_stats().begin(), "uptime");
		ASSERT_EQ(dev->get_stat("uptime"), "0");
		ASSERT_EQ(dev->get_stats_interval().count(), 60);
		ASSERT_EQ(dev->get_state(), device_state::ready);
		ASSERT_EQ(dev->get_nodes().size(), 1);
		ASSERT_EQ(*dev->get_nodes().begin(), "testnode");
		auto node = dev->get_node("testnode");
		ASSERT_EQ(node->get_id(), "testnode");
		ASSERT_EQ(node->get_type(), "light");
		ASSERT_EQ(node->get_name(), "Testnode");
		ASSERT_TRUE(node->get_properties().empty());
		ASSERT_EQ(node->get_device(), dev);
	}
	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}

TEST(MasterTest, DeviceNodePropertyPublished) {
	test_mqtt_client test_client;
	test_client.is_manager = true;
	test_client.expect_subscribe.insert("homie/#");
	test_client.expect_unsubscribe.insert("homie/#");

	{
		master m(test_client);
		test_client.handler->on_message("homie/testdevice/$state", "init");
		test_client.handler->on_message("homie/testdevice/$homie", "3.0.0");
		test_client.handler->on_message("homie/testdevice/$name", "Testdevice");
		test_client.handler->on_message("homie/testdevice/$localip", "10.0.0.1");
		test_client.handler->on_message("homie/testdevice/$mac", "AA:BB:CC:DD:EE:FF");
		test_client.handler->on_message("homie/testdevice/$fw/name", "Firmwarename");
		test_client.handler->on_message("homie/testdevice/$fw/version", "0.0.1");
		test_client.handler->on_message("homie/testdevice/$nodes", "testnode");
		test_client.handler->on_message("homie/testdevice/$implementation", "homie-cpp");
		test_client.handler->on_message("homie/testdevice/$stats", "uptime");
		test_client.handler->on_message("homie/testdevice/$stats/interval", "60");
		test_client.handler->on_message("homie/testdevice/$stats/uptime", "0");
		test_client.handler->on_message("homie/testdevice/testnode/$name", "Testnode");
		test_client.handler->on_message("homie/testdevice/testnode/$type", "light");
		test_client.handler->on_message("homie/testdevice/testnode/$properties", "intensity");
		test_client.handler->on_message("homie/testdevice/testnode/intensity", "100");
		test_client.handler->on_message("homie/testdevice/testnode/intensity/$name", "Intensity");
		test_client.handler->on_message("homie/testdevice/testnode/intensity/$settable", "true");
		test_client.handler->on_message("homie/testdevice/testnode/intensity/$unit", "%");
		test_client.handler->on_message("homie/testdevice/testnode/intensity/$datatype", "integer");
		test_client.handler->on_message("homie/testdevice/testnode/intensity/$format", "0:100");
		test_client.handler->on_message("homie/testdevice/$state", "ready");

		ASSERT_EQ(1, m.get_discovered_devices().size());
		auto dev = *m.get_discovered_devices().begin();
		ASSERT_EQ(dev->get_id(), "testdevice");
		ASSERT_EQ(dev->get_name(), "Testdevice");
		ASSERT_EQ(dev->get_localip(), "10.0.0.1");
		ASSERT_EQ(dev->get_mac(), "AA:BB:CC:DD:EE:FF");
		ASSERT_EQ(dev->get_firmware_name(), "Firmwarename");
		ASSERT_EQ(dev->get_firmware_version(), "0.0.1");
		ASSERT_EQ(dev->get_implementation(), "homie-cpp");
		ASSERT_EQ(dev->get_stats().size(), 1);
		ASSERT_EQ(*dev->get_stats().begin(), "uptime");
		ASSERT_EQ(dev->get_stat("uptime"), "0");
		ASSERT_EQ(dev->get_stats_interval().count(), 60);
		ASSERT_EQ(dev->get_state(), device_state::ready);
		ASSERT_EQ(dev->get_nodes().size(), 1);
		ASSERT_EQ(*dev->get_nodes().begin(), "testnode");
		auto node = dev->get_node("testnode");
		ASSERT_EQ(node->get_id(), "testnode");
		ASSERT_EQ(node->get_type(), "light");
		ASSERT_EQ(node->get_name(), "Testnode");
		ASSERT_EQ(node->get_device(), dev);
		ASSERT_EQ(node->get_properties().size(), 1);
		ASSERT_EQ(*node->get_properties().begin(), "intensity");
		auto prop = node->get_property("intensity");
		ASSERT_EQ(prop->get_id(), "intensity");
		ASSERT_EQ(prop->get_name(), "Intensity");
		ASSERT_EQ(prop->is_settable(), true);
		ASSERT_EQ(prop->get_unit(), "%");
		ASSERT_EQ(prop->get_datatype(), datatype::integer);
		ASSERT_EQ(prop->get_format(), "0:100");
		ASSERT_EQ(prop->get_node(), node);
	}
	ASSERT_TRUE(test_client.open_called);
	ASSERT_TRUE(test_client.steps.empty());
	ASSERT_TRUE(test_client.expect_subscribe.empty());
	ASSERT_TRUE(test_client.expect_unsubscribe.empty());
}