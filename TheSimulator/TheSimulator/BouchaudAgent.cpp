#include "BouchaudAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"
#include "ParameterStorage.h"

#include <random>
#include <cmath>

BouchaudAgent::BouchaudAgent(const Simulation* simulation)
	: Agent(simulation), m_exchange(""), m_volumeUnit(1), m_orderMeanArrivalTime(1000), m_orderMeanLifeTime(1000), m_marketOrderFraction(0.0), m_delta0(1.0), m_delta1(1.0), m_mu(0.6) { }

BouchaudAgent::BouchaudAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_exchange(""), m_volumeUnit(1), m_orderMeanArrivalTime(1000), m_orderMeanLifeTime(1000), m_marketOrderFraction(0.0), m_delta0(1.0), m_delta1(1.0), m_mu(0.6) { }

void BouchaudAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("volumeUnit")).empty()) {
		m_volumeUnit = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("orderMeanArrivalTime")).empty()) {
		m_orderMeanArrivalTime = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("orderMeanLifeTime")).empty()) {
		m_orderMeanLifeTime = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("marketOrderFraction")).empty()) {
		m_marketOrderFraction = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("delta0")).empty()) {
		m_delta0 = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("delta1")).empty()) {
		m_delta1 = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("mu")).empty()) {
		m_mu = std::stod(simulation()->parameters().processString(att.as_string()));
	}
}

void BouchaudAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		scheduleNextOrderPlacement();
		scheduleNextOrderCancellation();
	} else if (msg->type == "WAKEUP_FOR_PLACEMENT") {
		// queue an L1 data request
		simulation()->dispatchMessage(simulation()->currentTimestamp(), 0, name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
	} else if (msg->type == "RESPONSE_RETRIEVE_L1") {
		auto l1ptr = std::dynamic_pointer_cast<RetrieveL1ResponsePayload>(msg->payload);
		// place an order based on the current L1 status
		std::bernoulli_distribution orderTypeDistribution(m_marketOrderFraction);
		std::bernoulli_distribution orderDirectionDistribution(0.5);
		bool isMarketOrder = orderTypeDistribution(simulation()->randomGenerator());
		OrderDirection direction = orderDirectionDistribution(simulation()->randomGenerator()) ? OrderDirection::Buy : OrderDirection::Sell;
		if (isMarketOrder) {
			auto pptr = std::make_shared<PlaceOrderMarketPayload>(direction, m_volumeUnit);
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "PLACE_ORDER_MARKET", pptr);

			scheduleNextOrderPlacement();
		} else {
			std::uniform_real_distribution<> priceUniformDistribution(std::numeric_limits<double>::min(), 1.0);
			double randomUniformForPrice = priceUniformDistribution(simulation()->randomGenerator());
			Money priceDeltaFromBest = Money(std::pow(std::pow(m_delta0, m_mu) / randomUniformForPrice, 1+m_mu) - m_delta1);
			Money price;
			if (direction == OrderDirection::Buy) {
				price = l1ptr->bestAskPrice - priceDeltaFromBest.floorToCents();
			} else {
				price = l1ptr->bestBidPrice + priceDeltaFromBest.floorToCents();
			}

			auto pptr = std::make_shared<PlaceOrderLimitPayload>(direction, m_volumeUnit, price);
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "PLACE_ORDER_LIMIT", pptr);
		}
	} else if (msg->type == "RESPONSE_PLACE_ORDER_LIMIT") {
		auto responsepptr = std::dynamic_pointer_cast<PlaceOrderLimitResponsePayload>(msg->payload);
		auto orderIterator = std::upper_bound(m_ownedOrders.begin(), m_ownedOrders.end(), responsepptr->id, [](OrderID orderSought, const BouchaudAgentOrder& agentOrder) {
			return orderSought < agentOrder.id;
		});
		m_ownedOrders.insert(orderIterator, BouchaudAgentOrder(responsepptr->id, responsepptr->requestPayload->volume));

		auto pptr = std::make_shared<SubscribeEventTradeByOrderPayload>(responsepptr->id);
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_TRADE", pptr);

		scheduleNextOrderPlacement();
	} else if (msg->type == "EVENT_TRADE") {
		auto eventpptr = std::dynamic_pointer_cast<EventTradePayload>(msg->payload);
		auto orderIterator = std::lower_bound(m_ownedOrders.begin(), m_ownedOrders.end(), eventpptr->trade.restingOrderID(), [](const BouchaudAgentOrder& agentOrder, OrderID orderSought) {
			return agentOrder.id < orderSought;
		});
		if(orderIterator != m_ownedOrders.end()) {
			orderIterator->volume -= eventpptr->trade.volume();
			if (orderIterator->volume == 0) {
				m_ownedOrders.erase(orderIterator);
			}
		}
	} else if (msg->type == "WAKEUP_FOR_CANCELLATION") {
		if (!m_ownedOrders.empty()) { 
			// randomly cancel an order with the exchange, and remove it from ownedOrders
			std::uniform_int_distribution<size_t> discreteUniformDistribution(0, m_ownedOrders.size()-1);
			auto indexToKill = discreteUniformDistribution(simulation()->randomGenerator());
			auto it = m_ownedOrders.begin();
			std::advance(it, indexToKill);

			auto pptr = std::make_shared<CancelOrdersPayload>();
			pptr->cancellations.push_back(CancelOrdersCancellation(it->id, it->volume));
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "CANCEL_ORDER", pptr);

			m_ownedOrders.erase(it); // safe to erase here because everything else first checks whether an order with a given id exists
		}

		scheduleNextOrderCancellation();
	}
}

void BouchaudAgent::scheduleNextOrderPlacement() {
	double rate = 1.0 / m_orderMeanArrivalTime;

	// generate a random cancellation delay
	std::exponential_distribution<> exponentialDistribution(rate);
	Timestamp delay = (Timestamp)std::floor(exponentialDistribution(simulation()->randomGenerator()));

	// queue a placement
	simulation()->dispatchMessage(simulation()->currentTimestamp(), delay, name(), name(), "WAKEUP_FOR_PLACEMENT", std::make_shared<EmptyPayload>());
}

void BouchaudAgent::scheduleNextOrderCancellation() {
	// based on m_ownedOrders.size() and m_orderMeanLifetime, schedule an order cancellation
	Timestamp adjustedMeanOrderLifetime = (Timestamp)(m_orderMeanLifeTime * (1 + m_marketOrderFraction));
	double nextCancellationRate = !m_ownedOrders.empty() ? ((double)m_ownedOrders.size() / adjustedMeanOrderLifetime) : (1.0 / adjustedMeanOrderLifetime);

	// generate a random cancellation delay
	std::exponential_distribution<> exponentialDistribution(nextCancellationRate);
	Timestamp delay = (Timestamp)std::floor(exponentialDistribution(simulation()->randomGenerator()));

	// queue a cancellation
	simulation()->dispatchMessage(simulation()->currentTimestamp(), delay, name(), name(), "WAKEUP_FOR_CANCELLATION", std::make_shared<EmptyPayload>());
}
