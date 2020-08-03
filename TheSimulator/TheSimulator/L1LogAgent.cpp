#include "L1LogAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

#include <iostream>

L1LogAgent::L1LogAgent(const Simulation* simulation)
	: Agent(simulation), m_outputFile(), m_mostRecentPayload(nullptr), m_aggregationPeriod(0) { }

L1LogAgent::L1LogAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_outputFile(), m_mostRecentPayload(nullptr), m_aggregationPeriod(0) { }

void L1LogAgent::receiveMessage(const MessagePtr& messagePtr) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (messagePtr->type == "EVENT_SIMULATION_START") {
		if(!m_aggregationPeriod) {
			simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_LIMIT", std::make_shared<EmptyPayload>());
			simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_MARKET", std::make_shared<EmptyPayload>());
		} else {
			Timestamp nextAggregation = computeNextAggregation(currentTimestamp);
			simulation()->dispatchMessage(currentTimestamp, nextAggregation - currentTimestamp, name(), name(), "WAKEUP_FOR_AGGREGATION", std::make_shared<EmptyPayload>());
		}
	} else if (messagePtr->type == "EVENT_ORDER_LIMIT" || messagePtr->type == "EVENT_ORDER_MARKET" || messagePtr->type == "WAKEUP_FOR_AGGREGATION") {
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
	} else if (messagePtr->type == "RESPONSE_RETRIEVE_L1") {
		auto pptr = std::dynamic_pointer_cast<RetrieveL1ResponsePayload>(messagePtr->payload);

		if(!m_aggregationPeriod) {
			if (m_mostRecentPayload != nullptr) {
				if (pptr->bestAskPrice != m_mostRecentPayload->bestAskPrice || pptr->bestBidPrice != m_mostRecentPayload->bestBidPrice) {
					logData(pptr);
					m_mostRecentPayload = pptr;
				}
			} else {
				m_mostRecentPayload = pptr;
			}
		} else {
			logData(pptr);
			
			Timestamp nextAggregation = computeNextAggregation(currentTimestamp);
			simulation()->dispatchMessage(currentTimestamp, nextAggregation - currentTimestamp, name(), name(), "WAKEUP_FOR_AGGREGATION", std::make_shared<EmptyPayload>());
		}
	}
}

Timestamp L1LogAgent::computeNextAggregation(Timestamp current) const {
	Timestamp nextAggregation;
	Timestamp nextAggregationDelta = current % m_aggregationPeriod;
	if (nextAggregationDelta == 0) {
		nextAggregation = current + m_aggregationPeriod;
	} else {
		nextAggregation = current + m_aggregationPeriod - nextAggregationDelta;
	}

	return nextAggregation;
}

void L1LogAgent::logData(std::shared_ptr<RetrieveL1ResponsePayload> pptr) {
	m_outputFile << std::to_string(pptr->time) << "," << pptr->bestBidPrice.toCentString() << "," << pptr->bestAskPrice.toCentString() << std::endl;
	// std::cout << std::to_string(pptr->time) << ": BID " << pptr->bestBidPrice.toCentString() << " ASK " << pptr->bestAskPrice.toCentString() << " SPREAD " << ((Money)(pptr->bestAskPrice - pptr->bestBidPrice)).toCentString() << std::endl;
}

#include "ParameterStorage.h"

void L1LogAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("outputFile")).empty()) {
		m_outputFile.open(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("aggregationPeriod")).empty()) {
		m_aggregationPeriod = att.as_ullong();
	}
}