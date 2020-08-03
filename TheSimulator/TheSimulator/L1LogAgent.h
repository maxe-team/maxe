#pragma once
#include "Agent.h"

#include <memory>
#include <fstream>
#include "ExchangeAgentMessagePayloads.h"

class L1LogAgent : public Agent {
public:
	L1LogAgent(const Simulation* simulation);
	L1LogAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;

	std::shared_ptr<RetrieveL1ResponsePayload> m_mostRecentPayload;
	std::ofstream m_outputFile;
	Timestamp m_aggregationPeriod;
	Timestamp computeNextAggregation(Timestamp current) const;
	void logData(std::shared_ptr<RetrieveL1ResponsePayload> l1data);
};
