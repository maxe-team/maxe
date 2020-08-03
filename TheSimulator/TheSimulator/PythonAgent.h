#pragma once
#include "Agent.h"
#include <map>

#include <pybind11/embed.h>
namespace py = pybind11;

class PythonAgent : public Agent {
public:
	PythonAgent(const Simulation* simulation, const std::string& pythonClass, const std::string& file);
	PythonAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_class;
	std::string m_file;
	std::map<std::string, std::string> m_parameters;

	py::object m_instance;
};
