#pragma once
#include <string>
#include <memory>

namespace homie {
	struct firmware_info {
		virtual std::string get_name() const = 0;
		virtual std::string get_version() const = 0;
	};
	typedef std::shared_ptr<firmware_info> firmware_info_ptr;
	typedef std::shared_ptr<const firmware_info> const_firmware_info_ptr;
}