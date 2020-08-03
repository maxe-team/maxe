#include "DoobAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

DoobAgent::DoobAgent(const Simulation* simulation)
	: Agent(simulation), m_tradeUnit(0), m_state(DoobAgentInventoryState::Empty), m_upcrossingsCount(0) { }

DoobAgent::DoobAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_tradeUnit(0), m_state(DoobAgentInventoryState::Empty), m_upcrossingsCount(0) { }

void DoobAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("a")).empty()) {
		m_a = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("b")).empty()) {
		m_b = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("tradeUnit")).empty()) {
		m_tradeUnit = std::stoul(simulation()->parameters().processString(att.as_string()));
	}
}

#include <iostream>

void DoobAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_LIMIT", std::make_shared<EmptyPayload>());
	} else if (msg->type == "RESPONSE_SUBSCRIBE_EVENT_ORDER_LIMIT") {
		// no op
	} else if (msg->type == "EVENT_ORDER_LIMIT") {
		auto payload = std::dynamic_pointer_cast<EventOrderLimitPayload>(msg->payload);
		// queue an L1 data request
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
	} else if (msg->type == "RESPONSE_RETRIEVE_L1") {
		auto l1payload = std::dynamic_pointer_cast<RetrieveL1ResponsePayload>(msg->payload);
		if (m_state == DoobAgentInventoryState::Empty && l1payload->bestAskPrice <= m_a) {
			auto mopayload = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Buy, m_tradeUnit, 10000);
			simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "PLACE_ORDER_LIMIT", mopayload);
			m_state = DoobAgentInventoryState::NonEmpty;
		} else if(m_state == DoobAgentInventoryState::NonEmpty && l1payload->bestBidPrice >= m_b) {
			auto mopayload = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Sell, m_tradeUnit, 0);
			simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "PLACE_ORDER_LIMIT", mopayload);
			m_state = DoobAgentInventoryState::Empty;

			++m_upcrossingsCount;
		}
	} else if(msg->type == "EVENT_SIMULATION_STOP") {
		std::cout << this->name() << ": Upcrossings registered: " << m_upcrossingsCount << std::endl;
	}
}
