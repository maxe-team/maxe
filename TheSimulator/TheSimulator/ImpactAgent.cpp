#include "ImpactAgent.h"

#include "Simulation.h"
#include "SimulationException.h"
#include "ParameterStorage.h"

ImpactAgent::ImpactAgent(const Simulation* simulation)
	: Agent(simulation), m_impactTime(0), m_impactSide("ask"), m_greed(0.0) {}

ImpactAgent::ImpactAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_impactTime(0), m_impactSide("ask"), m_greed(0.0) { }

void ImpactAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("greed")).empty()) {
		m_greed = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("impactTime")).empty()) {
		m_impactTime = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("impactSide")).empty()) {
		m_impactSide = simulation()->parameters().processString(att.as_string());
		if (m_impactSide != "bid" && m_impactSide != "ask") {
			throw SimulationException("Unrecognized side: '" + m_impactSide + "', only 'bid'&'ask' are allowed");
		}
	}
}

void ImpactAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		simulation()->dispatchMessage(currentTimestamp, m_impactTime - currentTimestamp, name(), name(), "WAKEUP_FOR_IMPACT", std::make_shared<EmptyPayload>());
	} else if (msg->type == "WAKEUP_FOR_IMPACT") {
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
	} else if (msg->type == "RESPONSE_RETRIEVE_L1") {
		auto pptr = std::dynamic_pointer_cast<RetrieveL1ResponsePayload>(msg->payload);
		Volume relevantSideVolume = m_impactSide == "bid" ? pptr->bidTotalVolume : pptr->askTotalVolume;
		Volume amountToTrade = (Volume)std::floor(m_greed * relevantSideVolume);

		auto marketpayload = std::make_shared<PlaceOrderMarketPayload>(m_impactSide == "bid" ? OrderDirection::Sell : OrderDirection::Buy, amountToTrade);
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "PLACE_ORDER_MARKET", marketpayload);
	}
}
