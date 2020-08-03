#pragma once

#include <stdexcept>

class SimulationException : public std::runtime_error {
public:
	SimulationException(const std::string& message) : std::runtime_error(message) { }
	SimulationException(const SimulationException& exception) = default;
	SimulationException(SimulationException&& exception) = default;
};