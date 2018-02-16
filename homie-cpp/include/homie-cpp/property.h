#pragma once
#include <string>
#include <memory>
#include "datatype.h"

namespace homie {
	struct property {
		virtual std::string get_id() const = 0;
		virtual std::string get_name() const = 0;
		virtual bool is_settable() const = 0;
		virtual std::string get_unit() const = 0;
		virtual datatype get_datatype() const = 0;
		virtual std::string get_format() const = 0;

		virtual std::string get_value(int64_t node_idx) const = 0;
		virtual void set_value(int64_t node_idx, const std::string& value) = 0;
		virtual std::string get_value() const = 0;
		virtual void set_value(const std::string& value) = 0;
	};
	typedef std::shared_ptr<property> property_ptr;
	typedef std::shared_ptr<const property> const_property_ptr;
}