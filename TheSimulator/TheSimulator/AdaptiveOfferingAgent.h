#pragma once

#include <deque>

#include "Agent.h"
#include "Order.h"

struct WakeupForCancellationPayload : public MessagePayload {
	OrderID orderToCancelId;

	WakeupForCancellationPayload(OrderID orderToCancelId)
		: orderToCancelId(orderToCancelId) { }
};

struct AdaptiveOfferingAgentOrder {
	OrderID id;
	Volume offeredVolume;
	Volume currentVolume;
	unsigned int centDeltaFromBestPrice;
	Timestamp timeOfPlacement;
	Timestamp lifeTime;

	AdaptiveOfferingAgentOrder()
		: id(0), offeredVolume(0), currentVolume(0), centDeltaFromBestPrice(0), timeOfPlacement(0), lifeTime(0) { }
	AdaptiveOfferingAgentOrder(OrderID id, Volume offeredVolume)
		: id(id), offeredVolume(offeredVolume), currentVolume(offeredVolume), centDeltaFromBestPrice(0), timeOfPlacement(0), lifeTime(0) { }
};

class AdaptiveOfferingAgent : public Agent {
public:
	AdaptiveOfferingAgent(const Simulation* simulation);
	AdaptiveOfferingAgent(const Simulation* simulation, const std::string& name);

	void configure(const pugi::xml_node& node, const std::string& configurationPath) override;

	// Inherited via Agent
	void receiveMessage(const MessagePtr& msg) override;
private:
	std::string m_exchange;
	Volume m_volumeUnit;
	Timestamp m_orderMeanLifeTime;
	double m_marketOrderFraction;
	double m_priceScale;
	unsigned int m_memorySize;

	AdaptiveOfferingAgentOrder m_currentOrder;

	Timestamp computeOrderCancellationDelay();
	double computeTradingRateObservation(unsigned int priceCentsDeltaFromBest);
	Volume computeVolumeToOrder(unsigned int priceCentsDeltaFromBest, Timestamp lifeTime);

	std::vector<std::deque<std::pair<double, double>>> m_fulfillmentRates;
};