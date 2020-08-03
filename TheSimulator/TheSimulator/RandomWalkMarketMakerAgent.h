#pragma once

#include "Agent.h"
#include "Order.h"

class RandomWalkMarketMakerAgent : public Agent {
public:
	RandomWalkMarketMakerAgent(const Simulation* simulation);
	RandomWalkMarketMakerAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;
	double m_p;
	
	Money m_halfSpread;
	unsigned int m_depth;
	
	Money m_priceStep;
	Timestamp m_timeStep;
	Money m_currentMidPrice;
	Money m_lb;
	Money m_ub;

	void scheduleMarketMaking();
	OrderID m_outstandingBuyOrder;
	OrderID m_outstandingSellOrder;
};