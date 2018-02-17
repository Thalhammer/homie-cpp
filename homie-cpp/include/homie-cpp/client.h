#pragma once
#include "mqtt_connection.h"
#include "device.h"
#include "utils.h"
#include <set>

namespace homie {
	class client : private mqtt_event_handler {
		mqtt_connection& mqtt;
		std::string base_topic;
		std::set<device_ptr> devices;

		// Inherited by mqtt_event_handler
		virtual void on_connect(bool session_present) override {
			for (auto& e : devices) {
				this->publish_device_info(e);
			}
		}
		virtual void on_closing() override {
			for (auto& e : devices) {
				this->publish_device_attribute(e->get_id(), "$state", enum_to_string(device_state::disconnected));
			}
		}
		virtual void on_closed() override {}
		virtual void on_offline() override {}
		virtual void on_message(const std::string & topic, const std::string & payload) override {
			// Check basetopic
			if (topic.size() < base_topic.size())
				return;
			if (topic.compare(0, base_topic.size(), base_topic) != 0)
				return;

			auto parts = utils::split<std::string>(topic, "/", base_topic.size());
			if (parts.size() < 2)
				return;
			for (auto& e : parts) if (e.empty()) return;
			if (parts[0][0] == '$') {
				if (parts[0] == "$broadcast") {
					this->handle_broadcast(parts[1], payload);
				}
			}
			else {
				if (parts.size() != 4
					|| parts[3] != "set"
					|| parts[1][0] == '$'
					|| parts[2][0] == '$')
					return;
				this->handle_property_set(parts[0], parts[1], parts[2], payload);
			}
		}

		void handle_property_set(const std::string& sdevice, const std::string& snode, const std::string& sproperty, const std::string& payload) {
			if (sdevice.empty() || snode.empty() || sproperty.empty())
				return;

			int64_t id = 0;
			bool is_array_node = false;
			std::string rnode = snode;
			auto pos = rnode.find('_');
			if (pos != std::string::npos) {
				id = std::stoll(rnode.substr(pos + 1));
				is_array_node = true;
				rnode.resize(pos);
			}

			for (auto& dev : devices)
			{
				if (dev->get_id() != sdevice)
					continue;
				for (auto& node : dev->get_nodes())
				{
					if (node->is_array() != is_array_node || node->get_id() != rnode)
						continue;
					for (auto& property : node->get_properties())
					{
						if (property->get_id() != sproperty)
							continue;
						if (is_array_node)
							property->set_value(id, payload);
						else property->set_value(payload);
					}
				}
				return;
			}
		}

		void handle_broadcast(const std::string& level, const std::string& payload) {

		}

		void publish_device_info(device_ptr dev) {
			// Signal initialisation phase
			this->publish_device_attribute(dev->get_id(), "$state", enum_to_string(device_state::init));

			// Public device properties
			this->publish_device_attribute(dev->get_id(), "$homie", "3.0.0");
			this->publish_device_attribute(dev->get_id(), "$name", dev->get_name());
			this->publish_device_attribute(dev->get_id(), "$localip", dev->get_localip());
			this->publish_device_attribute(dev->get_id(), "$mac", dev->get_mac());
			this->publish_device_attribute(dev->get_id(), "$fw/name", dev->get_firmware()->get_name());
			this->publish_device_attribute(dev->get_id(), "$fw/version", dev->get_firmware()->get_version());
			this->publish_device_attribute(dev->get_id(), "$implementation", dev->get_implementation());
			this->publish_device_attribute(dev->get_id(), "$stats/interval", std::to_string(dev->get_stats_interval().count() / 1000));

			// Publish nodes
			std::string nodes = "";
			for (auto& node : dev->get_nodes()) {
				if (node->is_array()) {
					nodes += node->get_id() + "[],";
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/$array", std::to_string(node->array_range().first) + "-" + std::to_string(node->array_range().second));
					for (int64_t i = node->array_range().first; i <= node->array_range().second; i++) {
						this->publish_device_attribute(dev->get_id(), node->get_id() + "_" + std::to_string(i) + "/$name", node->get_name(i));
					}
				}
				else {
					nodes += node->get_id() + ",";
				}
				this->publish_device_attribute(dev->get_id(), node->get_id() + "/$name", node->get_name());
				this->publish_device_attribute(dev->get_id(), node->get_id() + "/$type", node->get_type());

				// Publish node properties
				std::string properties = "";
				for (auto& property : node->get_properties()) {
					properties += property->get_id() + ",";
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id() + "/$name", property->get_name());
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id() + "/$settable", property->is_settable() ? "true" : "false");
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id() + "/$unit", property->get_unit());
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id() + "/$datatype", enum_to_string(property->get_datatype()));
					this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id() + "/$format", property->get_format());
					if (!node->is_array()) {
						this->publish_device_attribute(dev->get_id(), node->get_id() + "/" + property->get_id(), property->get_value());
					}
					else {
						for (int64_t i = node->array_range().first; i <= node->array_range().second; i++) {
							this->publish_device_attribute(dev->get_id(), node->get_id() + "_" + std::to_string(i) + "/" + property->get_id(), property->get_value(i));
						}
					}
				}
				if (!properties.empty())
					properties.resize(properties.size() - 1);
				this->publish_device_attribute(dev->get_id(), node->get_id() + "/$properties", properties);
			}
			if (!nodes.empty())
				nodes.resize(nodes.size() - 1);
			this->publish_device_attribute(dev->get_id(), "$nodes", nodes);

			// Publish stats
			std::string stats = "";
			for (auto& stat : dev->get_stats()) {
				stats += stat->get_id() + ",";
				this->publish_device_status(dev->get_id(), stat);
			}
			if (!stats.empty())
				stats.resize(stats.size() - 1);
			this->publish_device_attribute(dev->get_id(), "$stats", stats);

			// Subscribe to set topics
			this->mqtt.subscribe(base_topic + dev->get_id() + "/+/+/set", 1);

			// Everything done, set device to real state
			this->publish_device_attribute(dev->get_id(), "$state", enum_to_string(dev->get_state()));
		}

		void publish_device_status(const std::string& devid, const_status_ptr stat) {
			this->publish_device_attribute(devid, "$stats/" + stat->get_id(), stat->get_value());
		}

		void publish_device_attribute(const std::string& devid, const std::string& attribute, const std::string& value) {
			mqtt.publish(base_topic + devid + "/" + attribute, value, 1, true);
		}

		void notify_property_changed_impl(const std::string& sdevice, const std::string& snode, const std::string& sproperty, const int64_t* idx) {
			if (sdevice.empty() || snode.empty() || sproperty.empty())
				return;

			auto device = find_device_by_name(sdevice);
			if (!device) return;
			auto node = find_node_by_name(device, snode);
			if (!node) return;
			auto prop = find_property_by_name(node, sproperty);
			if (!prop) return;
			if (node->is_array()) {
				if (idx != nullptr) {
					this->publish_device_attribute(device->get_id(), node->get_id() + "_" + std::to_string(*idx) + "/" + prop->get_id(), prop->get_value(*idx));
				}
				else {
					auto range = node->array_range();
					for (auto i = range.first; i <= range.second; i++) {
						this->publish_device_attribute(device->get_id(), node->get_id() + "_" + std::to_string(i) + "/" + prop->get_id(), prop->get_value(i));
					}
				}
			}
			else {
				this->publish_device_attribute(device->get_id(), node->get_id() + "/" + prop->get_id(), prop->get_value());
			}
		}

		device_ptr find_device_by_name(const std::string& d) {
			for (auto& dev : devices)
			{
				if (dev->get_id() != d)
					continue;
				return dev;
			}
			return nullptr;
		}

		static node_ptr find_node_by_name(device_ptr dev, const std::string& n) {
			for (auto& node : dev->get_nodes())
			{
				if (node->get_id() != n)
					continue;
				return node;
			}
			return nullptr;
		}

		static property_ptr find_property_by_name(node_ptr node, const std::string& p) {
			for (auto& prop : node->get_properties())
			{
				if (prop->get_id() != p)
					continue;
				return prop;
			}
			return nullptr;
		}
	public:
		client(mqtt_connection& con, std::string basetopic = "homie/")
			: mqtt(con), base_topic(basetopic)
		{
			mqtt.set_event_handler(this);
		}

		~client() {
			for (auto& dev : devices) {
				this->publish_device_attribute(dev->get_id(), "$state", enum_to_string(device_state::disconnected));
				this->mqtt.unsubscribe(base_topic + dev->get_id() + "/+/+/set");
			}

			mqtt.set_event_handler(nullptr);
		}

		void add_device(device_ptr dev) {
			if (devices.count(dev) != 0) return;
			devices.insert(dev);
			this->publish_device_info(dev);
		}

		void notify_property_changed(const std::string& sdevice, const std::string& snode, const std::string& sproperty) {
			notify_property_changed_impl(sdevice, snode, sproperty, nullptr);
		}

		void notify_property_changed(const std::string& sdevice, const std::string& snode, const std::string& sproperty, int64_t idx) {
			notify_property_changed_impl(sdevice, snode, sproperty, &idx);
		}
	};
}