#pragma once
#include "Agent.h"
#include "Volume.h"

#include <memory>
#include <fstream>

#include "ParameterStorage.h"

class SetupAgent : public Agent {
public:
	SetupAgent(const Simulation* simulation);
	SetupAgent(const Simulation* simulation, const std::string& name);

	// Inherited via IConfigurable
	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;

	Timestamp m_setupTime;
	Volume m_bidVolume;
	int m_bidPrice;
	Volume m_askVolume;
	int m_askPrice;
};
