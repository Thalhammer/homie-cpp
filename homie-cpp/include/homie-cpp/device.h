#pragma once
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include "device_state.h"
#include "firmware_info.h"
#include "status.h"
#include "node.h"

namespace homie {
	struct device {
		virtual std::string get_id() const = 0;
		virtual std::string get_name() const = 0;
		virtual device_state get_state() const = 0;
		virtual std::string get_localip() const = 0;
		virtual std::string get_mac() const = 0;
		virtual const_firmware_info_ptr get_firmware() const = 0;
		virtual std::set<const_node_ptr> get_nodes() const = 0;
		virtual std::set<node_ptr> get_nodes() = 0;
		virtual std::string get_implementation() const = 0;
		virtual std::set<const_status_ptr> get_stats() const = 0;
		virtual std::chrono::milliseconds get_stats_interval() const = 0;
	};
	typedef std::shared_ptr<device> device_ptr;
	typedef std::shared_ptr<const device> const_device_ptr;
}