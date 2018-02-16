#pragma once
#include <string>
#include <memory>
#include "property.h"

namespace homie {
	struct node {
		virtual std::string get_id() const = 0;
		virtual std::string get_name() const = 0;
		virtual std::string get_name(int64_t node_idx) const = 0;
		virtual std::string get_type() const = 0;
		virtual bool is_array() const = 0;
		virtual std::pair<int64_t, int64_t> array_range() const = 0;
		virtual std::vector<const_property_ptr> get_properties() const = 0;
		virtual std::vector<property_ptr> get_properties() = 0;
	};
	typedef std::shared_ptr<node> node_ptr;
	typedef std::shared_ptr<const node> const_node_ptr;
}