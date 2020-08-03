#include "PythonAgent.h"
#include "Simulation.h"

#include <pybind11/stl.h>

PythonAgent::PythonAgent(const Simulation* simulation, const std::string& pythonClass, const std::string& file)
	: Agent(simulation), m_class(pythonClass), m_file(file) { }

PythonAgent::PythonAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_class(""), m_file("") { }

void PythonAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	for (const pugi::xml_attribute& attr : node.attributes()) {
		if (std::string(attr.name()) != "file" && std::string(attr.name()) != "name") {
			m_parameters[std::string(attr.name())] = simulation()->parameters().processString(attr.as_string());
		}
	}

	py::object agentClass;
	if (m_file == "") {
		py::module m = py::module::import(m_class.c_str());
		agentClass = m.attr(m_class.c_str());
	} else {
		py::object result = py::eval_file(m_file);
		agentClass = result.attr(m_class.c_str());
	}

	py::cpp_function nameStringFunction = [this]() {
		return this->name();
	};
	//py::object nameStringObject = py::str(name());
	agentClass.attr("name") = nameStringFunction;
	m_instance = agentClass();

	py::function fun = py::reinterpret_borrow<py::function>(m_instance.attr("configure"));
	py::object _ret = fun(m_parameters);
}

#include <iostream>

void PythonAgent::receiveMessage(const MessagePtr& msg) {
	py::function receiveMessageFunction = py::reinterpret_borrow<py::function>(m_instance.attr("receiveMessage"));
	py::object _ret = receiveMessageFunction(simulation(), msg->type, py::dict()/*, msg.toDict()*/);
}
