#pragma once

#include "pugi/pugixml.hpp"

#include <string>

using ConfigurationIndex = unsigned int;
constexpr ConfigurationIndex CONFIGURATIONINDEX_INVALID = 0;

class IConfigurable {
public:
	virtual ~IConfigurable() = default;

	virtual void configure(const pugi::xml_node& node, const std::string& configurationPath) = 0;
	
protected:
	IConfigurable() = default;
};