#include "OrderLogAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

OrderLogAgent::OrderLogAgent(const Simulation* simulation)
	: Agent(simulation) { }

OrderLogAgent::OrderLogAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name) { }

void OrderLogAgent::receiveMessage(const MessagePtr& messagePtr) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (messagePtr->type == "EVENT_SIMULATION_START") {
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_LIMIT", std::make_shared<EmptyPayload>());
		simulation()->dispatchMessage(currentTimestamp, 0, name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_MARKET", std::make_shared<EmptyPayload>());
	} else if (messagePtr->type == "EVENT_ORDER_MARKET") {
		auto pptr = std::dynamic_pointer_cast<EventOrderMarketPayload>(messagePtr->payload);
		const auto& order = pptr->order;

		std::cout << name() << ": ";
		order.printHuman();
	} else if (messagePtr->type == "EVENT_ORDER_LIMIT") {
		auto pptr = std::dynamic_pointer_cast<EventOrderLimitPayload>(messagePtr->payload);
		const auto& order = pptr->order;

		std::cout << name() << ": ";
		order.printHuman();
		std::cout << std::endl;
	}
}

#include "ParameterStorage.h"

void OrderLogAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) { 
		m_exchange = simulation()->parameters().processString(att.as_string());
	}
}