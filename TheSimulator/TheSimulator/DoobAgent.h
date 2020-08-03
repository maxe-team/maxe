#pragma once

#include "Agent.h"
#include "Order.h"

enum class DoobAgentInventoryState {
	Empty,
	NonEmpty
};

class DoobAgent : public Agent {
public:
	DoobAgent(const Simulation* simulation);
	DoobAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	DoobAgentInventoryState m_state;
	std::string m_exchange;
	Money m_a, m_b;
	unsigned int m_tradeUnit;
	unsigned int m_upcrossingsCount;
};