#include "Simulation.h"
#include "RandomWalkMarketMakerAgent.h"

#include "ExchangeAgentMessagePayloads.h"

RandomWalkMarketMakerAgent::RandomWalkMarketMakerAgent(const Simulation* simulation)
	: Agent(simulation), m_exchange(""), m_p(0.5), m_halfSpread(0.01), m_depth(0), m_priceStep(0.01), m_timeStep(1), m_currentMidPrice(1), m_lb(1), m_ub(1), m_outstandingBuyOrder(0), m_outstandingSellOrder(0) { }

RandomWalkMarketMakerAgent::RandomWalkMarketMakerAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_exchange(""), m_p(0.5), m_halfSpread(0.01), m_depth(0), m_priceStep(0.01), m_timeStep(1), m_currentMidPrice(1), m_lb(1), m_ub(1), m_outstandingBuyOrder(0), m_outstandingSellOrder(0) { }

void RandomWalkMarketMakerAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("p")).empty()) {
		m_p = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("halfSpread")).empty()) {
		m_halfSpread = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("depth")).empty()) {
		m_depth = std::stoul(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("priceStep")).empty()) {
		m_priceStep = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("timeStep")).empty()) {
		m_timeStep = std::stoul(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("init")).empty()) {
		m_currentMidPrice = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("lb")).empty()) {
		m_lb = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}

	if (!(att = node.attribute("ub")).empty()) {
		m_ub = Money(std::stod(simulation()->parameters().processString(att.as_string())));
	}
}

void RandomWalkMarketMakerAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		// trigger immediate market making
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), this->name(), "WAKEUP_FOR_MARKETMAKING", std::make_shared<EmptyPayload>());
	} else if (msg->type == "WAKEUP_FOR_MARKETMAKING") {
		// cancel the outstanding orders
		auto cpptr = std::make_shared<CancelOrdersPayload>();
		if (m_outstandingBuyOrder != 0) {
			cpptr->cancellations.push_back(CancelOrdersCancellation(m_outstandingBuyOrder, m_depth));
		}
		if (m_outstandingSellOrder != 0) {
			cpptr->cancellations.push_back(CancelOrdersCancellation(m_outstandingSellOrder, m_depth));
		}
		if (cpptr->cancellations.size() > 0) {
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "CANCEL_ORDERS", cpptr);
		}

		// walk a step
		std::bernoulli_distribution stepTypeDistribution(m_p);
		Money step = stepTypeDistribution(simulation()->randomGenerator()) ? m_priceStep : -m_priceStep;
		m_currentMidPrice += step;
		if (m_currentMidPrice < m_lb) {
			m_currentMidPrice = m_lb;
		}
		if (m_currentMidPrice > m_ub) {
			m_currentMidPrice = m_ub;
		}

		// place new orders
		Money newSellPrice = m_currentMidPrice + m_halfSpread;
		Money newBuyPrice = m_currentMidPrice - m_halfSpread;

		auto pptr = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Sell, m_depth, newSellPrice);
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "PLACE_ORDER_LIMIT", pptr);

		pptr = std::make_shared<PlaceOrderLimitPayload>(OrderDirection::Buy, m_depth, newBuyPrice);
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "PLACE_ORDER_LIMIT", pptr);

		// schedule next marketMaking
		scheduleMarketMaking();
	} else if (msg->type == "RESPONSE_PLACE_ORDER_LIMIT") {
		auto payload = std::dynamic_pointer_cast<PlaceOrderLimitResponsePayload>(msg->payload);
		if (payload->requestPayload->direction == OrderDirection::Buy) {
			m_outstandingBuyOrder = payload->id;
		} else {
			m_outstandingSellOrder = payload->id;
		}
	}
}

void RandomWalkMarketMakerAgent::scheduleMarketMaking() {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();
	simulation()->dispatchMessage(currentTimestamp, m_timeStep, this->name(), this->name(), "WAKEUP_FOR_MARKETMAKING", std::make_shared<EmptyPayload>());
}
