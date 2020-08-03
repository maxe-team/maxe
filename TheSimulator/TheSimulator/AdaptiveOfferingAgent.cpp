#include "AdaptiveOfferingAgent.h"

#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

#include <fstream>

AdaptiveOfferingAgent::AdaptiveOfferingAgent(const Simulation* simulation)
	: Agent(simulation), m_exchange(""), m_volumeUnit(1), m_orderMeanLifeTime(1000), m_marketOrderFraction(0.0), m_priceScale(1.0), m_memorySize(5) { }

AdaptiveOfferingAgent::AdaptiveOfferingAgent(const Simulation* simulation, const std::string& name)
	: Agent(simulation, name), m_exchange(""), m_volumeUnit(1), m_orderMeanLifeTime(1000), m_marketOrderFraction(0.0), m_priceScale(1.0), m_memorySize(5) { }

void AdaptiveOfferingAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("exchange")).empty()) {
		m_exchange = simulation()->parameters().processString(att.as_string());
	}

	if (!(att = node.attribute("volumeUnit")).empty()) {
		m_volumeUnit = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("orderMeanLifeTime")).empty()) {
		m_orderMeanLifeTime = std::stoull(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("marketOrderFraction")).empty()) {
		m_marketOrderFraction = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("priceScale")).empty()) {
		m_priceScale = std::stod(simulation()->parameters().processString(att.as_string()));
	}

	if (!(att = node.attribute("memorySize")).empty()) {
		m_memorySize = std::stoul(simulation()->parameters().processString(att.as_string()));
	}
}

void AdaptiveOfferingAgent::receiveMessage(const MessagePtr& msg) {
	const Timestamp currentTimestamp = simulation()->currentTimestamp();

	if (msg->type == "EVENT_SIMULATION_START") {
		auto delay = computeOrderCancellationDelay();
		simulation()->dispatchMessage(simulation()->currentTimestamp(), delay, name(), name(), "WAKEUP_FOR_CANCELLATION", std::make_shared<WakeupForCancellationPayload>(m_currentOrder.id));
	} else if (msg->type == "WAKEUP_FOR_CANCELLATION") {
		auto pptr = std::dynamic_pointer_cast<WakeupForCancellationPayload>(msg->payload);
		if (pptr->orderToCancelId == m_currentOrder.id && m_currentOrder.id != 0) {
			auto cpptr = std::make_shared<CancelOrdersPayload>();
			cpptr->cancellations.push_back(CancelOrdersCancellation(m_currentOrder.id, m_currentOrder.offeredVolume));
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "CANCEL_ORDERS", cpptr);
		} else {
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
		}
	} else if (msg->type == "RESPONSE_CANCEL_ORDERS") {
		const Volume tradedDelta = m_currentOrder.offeredVolume - m_currentOrder.currentVolume;
		const Timestamp timeDelta = currentTimestamp - m_currentOrder.timeOfPlacement;
		if(timeDelta > 0) {
			auto& ffr = m_fulfillmentRates[m_currentOrder.centDeltaFromBestPrice];
			ffr.push_back(std::make_pair(timeDelta, (double)tradedDelta / m_currentOrder.offeredVolume));
			if (ffr.size() > m_memorySize) {
				ffr.pop_front();
			}
		}

		m_currentOrder.id = 0;
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "RETRIEVE_L1", std::make_shared<EmptyPayload>());
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
		} else {
			std::uniform_real_distribution<> priceUniformDistribution(std::numeric_limits<double>::min(), 1.0);
			double randomUniformForPrice = priceUniformDistribution(simulation()->randomGenerator());
			Money priceDeltaFromBest = Money(randomUniformForPrice * m_priceScale);
			const auto inCents = priceDeltaFromBest.floorToCents();

			Money price;
			if (direction == OrderDirection::Buy) {
				price = l1ptr->bestAskPrice - inCents;
			} else {
				price = l1ptr->bestBidPrice + inCents;
			}

			m_currentOrder.centDeltaFromBestPrice = inCents.cents();
			auto delay = computeOrderCancellationDelay();
			m_currentOrder.lifeTime = delay;
			const Volume volumeToOrder = computeVolumeToOrder(inCents.cents(), delay);
			auto pptr = std::make_shared<PlaceOrderLimitPayload>(direction, volumeToOrder, price);
			simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "PLACE_ORDER_LIMIT", pptr);
		}
	} else if (msg->type == "RESPONSE_PLACE_ORDER_LIMIT") {
		auto polptr = std::dynamic_pointer_cast<PlaceOrderLimitResponsePayload>(msg->payload);
		m_currentOrder.id = polptr->id;
		m_currentOrder.offeredVolume = m_currentOrder.currentVolume = polptr->requestPayload->volume;
		m_currentOrder.timeOfPlacement = currentTimestamp;

		auto pptr = std::make_shared<SubscribeEventTradeByOrderPayload>(polptr->id);
		simulation()->dispatchMessage(currentTimestamp, 0, this->name(), m_exchange, "SUBSCRIBE_EVENT_ORDER_TRADE", pptr);
		simulation()->dispatchMessage(simulation()->currentTimestamp(), m_currentOrder.lifeTime, name(), name(), "WAKEUP_FOR_CANCELLATION", std::make_shared<WakeupForCancellationPayload>(m_currentOrder.id));
	} else if (msg->type == "RESPONSE_PLACE_ORDER_MARKET") {
		auto delay = computeOrderCancellationDelay();
		simulation()->dispatchMessage(simulation()->currentTimestamp(), delay, name(), name(), "WAKEUP_FOR_CANCELLATION", std::make_shared<WakeupForCancellationPayload>(m_currentOrder.id));
	} else if (msg->type == "EVENT_TRADE") {
		auto eventpptr = std::dynamic_pointer_cast<EventTradePayload>(msg->payload);
		if(m_currentOrder.id == eventpptr->trade.restingOrderID()) {
			const Volume volumeToSubtract = eventpptr->trade.volume();
			m_currentOrder.currentVolume -= volumeToSubtract;

			if (m_currentOrder.currentVolume == 0 || (m_currentOrder.offeredVolume - m_currentOrder.currentVolume) >= m_volumeUnit) {
				simulation()->dispatchMessage(simulation()->currentTimestamp(), 0, name(), name(), "WAKEUP_FOR_CANCELLATION", std::make_shared<WakeupForCancellationPayload>(m_currentOrder.id));
			}
		}
	} else if (msg->type == "EVENT_SIMULATION_STOP") {
		for (unsigned int i = 0; i < m_fulfillmentRates.size(); ++i) {
			if(m_fulfillmentRates[i].size()  == m_memorySize) {
				std::ofstream statusfile("offering" + std::to_string(i) + ".csv", std::ios::app | std::ios::out);
				const double guess = computeTradingRateObservation(i) * m_orderMeanLifeTime;
				if(guess != 0.0) {
					statusfile << (guess <= 1.0 ? (1.0 / guess) * m_volumeUnit : m_volumeUnit) << ",";
				}
			}
		}
	}
}

Timestamp AdaptiveOfferingAgent::computeOrderCancellationDelay() {
	Timestamp adjustedMeanOrderLifetime = (Timestamp)(m_orderMeanLifeTime * (1 + m_marketOrderFraction));
	double nextCancellationRate = 1.0 / adjustedMeanOrderLifetime;

	// generate a random cancellation delay
	std::exponential_distribution<> exponentialDistribution(nextCancellationRate);
	Timestamp delay = (Timestamp)std::floor(exponentialDistribution(simulation()->randomGenerator()));

	return delay;
}

double AdaptiveOfferingAgent::computeTradingRateObservation(unsigned int priceCentsDeltaFromBest) {
	double totalWeight = std::accumulate(m_fulfillmentRates[priceCentsDeltaFromBest].cbegin(), m_fulfillmentRates[priceCentsDeltaFromBest].cend(), (double)0, [](double acc, const auto& pair) {
		return acc + pair.first;
	});
	double totalSum = std::accumulate(m_fulfillmentRates[priceCentsDeltaFromBest].cbegin(), m_fulfillmentRates[priceCentsDeltaFromBest].cend(), (double)0, [](double acc, const auto& pair) {
		return acc + pair.second;
	});
	
	return totalSum / totalWeight;
}

Volume AdaptiveOfferingAgent::computeVolumeToOrder(unsigned int priceCentsDeltaFromBest, Timestamp lifeTime) {
	if (m_fulfillmentRates.size() <= priceCentsDeltaFromBest) {
		m_fulfillmentRates.resize(priceCentsDeltaFromBest+1, std::deque<std::pair<double, double>>());
		return m_volumeUnit;
	} else if (m_fulfillmentRates[priceCentsDeltaFromBest].size() < m_memorySize) {
		return m_volumeUnit;
	} else {
		const double fractionThatCanBeTradedGuess = lifeTime * computeTradingRateObservation(priceCentsDeltaFromBest);
		if (fractionThatCanBeTradedGuess == 0 || std::isinf(fractionThatCanBeTradedGuess) || fractionThatCanBeTradedGuess > 1.0) {
			return m_volumeUnit;
		} else {
			return (Volume)std::floor((1.0 / fractionThatCanBeTradedGuess) * m_volumeUnit);
		}
	}
}
