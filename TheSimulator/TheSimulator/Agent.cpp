#include "Agent.h"
#include "ParameterStorage.h"

#include "Simulation.h"

void Agent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	pugi::xml_attribute att;
	if (!(att = node.attribute("name")).empty()) {
		setName(simulation()->parameters().processString(att.as_string()) + configurationPath);
	}
}
