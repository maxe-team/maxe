#include "SetupAgent.h"

#include "ExchangeAgentMessagePayloads.h"
#include "Simulation.h"

SetupAgent::SetupAgent(const Simulation* simulation)
	: Agent(simulation), m_setupTime(0), m_askVolume(0), m_askPrice(0), m_bidVolume(0), m_bidPrice(0) {}

SetupAgent::SetupAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_setupTime(0), m_askVolume(0), m_askPrice(0), m_bidVolume(0), m_bidPrice(0) { }

void SetupAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("setupTime")).empty()) {
		m_setupTime = att.as_ullong();
	}

	if (!(att = node.attribute("bidVolume")).empty()) {
		m_bidVolume = att.as_ullong();
	}

	if (!(att = node.attribute("askVolume")).empty()) {
		m_askVolume = att.as_ullong();
	}

	if (!(att = node.attribute("bidPrice")).empty()) {
		m_bidPrice = att.as_int();
	}

	if (!(att = node.attribute("askPrice")).empty()) {
		m_askPrice = att.as_int();
	}
}

void SetupAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		auto bidPayload = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Buy, m_bidVolume, Money(0, m_bidPrice));
		auto askPayload = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Sell, m_askVolume, Money(0, m_askPrice));
		simulation()->dispatchMessage(currentTimestamp, m_setupTime - currentTimestamp, name(), m_exchange, "PLACE_ORDER_LIMIT", bidPayload);
		simulation()->dispatchMessage(currentTimestamp, m_setupTime - currentTimestamp, name(), m_exchange, "PLACE_ORDER_LIMIT", askPayload);
	}
}
