#pragma once
#include "Agent.h"

#include <memory>
#include <fstream>
#include "ExchangeAgentMessagePayloads.h"

class ImpactAgent : public Agent {
public:
	ImpactAgent(const Simulation* simulation);
	ImpactAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;

	double m_greed;
	Timestamp m_impactTime;
	std::string m_impactSide;
};
