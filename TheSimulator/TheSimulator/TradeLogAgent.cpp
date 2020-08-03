#include "TradeLogAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

#include <iostream>

TradeLogAgent::TradeLogAgent(const Simulation* simulation)
	: Agent(simulation) { }

TradeLogAgent::TradeLogAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name) { }

void TradeLogAgent::receiveMessage(const MessagePtr& messagePtr) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();
	
	if (messagePtr->type == "EVENT_SIMULATION_START") {
		simulation()->dispatchMessage(currentTimestamp, currentTimestamp, name(), m_exchange, "SUBSCRIBE_EVENT_TRADE", std::make_shared<EmptyPayload>());
	} else if (messagePtr->type == "EVENT_TRADE") {
		auto pptr = std::dynamic_pointer_cast<EventTradePayload>(messagePtr->payload);
		const auto& trade = pptr->trade;
		
		std::cout << name() << ": ";
		trade.printHuman();
		std::cout << std::endl;
	}
}

#include "ParameterStorage.h"

void TradeLogAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}
}