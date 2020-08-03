#pragma once

#include "Agent.h"
#include "Order.h"

struct BouchaudAgentOrder {
	OrderID id;
	Volume volume;

	BouchaudAgentOrder(OrderID id, Volume volume) : id(id), volume(volume) { }
};

class BouchaudAgent : public Agent {
public:
	BouchaudAgent(const Simulation* simulation);
	BouchaudAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath);

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;
	Volume m_volumeUnit;
	Timestamp m_orderMeanArrivalTime;
	Timestamp m_orderMeanLifeTime;
	double m_marketOrderFraction;
	double m_delta0;
	double m_delta1;
	double m_mu;

	void scheduleNextOrderPlacement();
	void scheduleNextOrderCancellation();

	std::vector<BouchaudAgentOrder> m_ownedOrders;
};