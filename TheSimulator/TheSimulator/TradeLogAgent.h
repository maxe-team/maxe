#pragma once

#include "Agent.h"

class TradeLogAgent : public Agent {
public:
	TradeLogAgent(const Simulation* simulation);
	TradeLogAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;
};
