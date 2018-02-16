#pragma once
#include <string>
#include <memory>

namespace homie {
	struct status {
		virtual std::string get_id() const = 0;
		virtual std::string get_value() const = 0;
	};
	typedef std::shared_ptr<status> status_ptr;
	typedef std::shared_ptr<const status> const_status_ptr;
}