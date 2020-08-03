#pragma once

#include "Timestamp.h"
#include "IMessageable.h"
#include "IConfigurable.h"
#include <string>

class Agent : public IMessageable, public IConfigurable {
public:
	virtual ~Agent() = default;

	// Inherited via IConfigurable
	virtual void configure(const pugi::xml_node& node, const std::string& configurationPath) override;
protected:
	Agent(const Simulation* simulation)
		: Agent(simulation, "") { }
	Agent(const Simulation* simulation, const std::string& name)
		: IMessageable(simulation, name) { }
};