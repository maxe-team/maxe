#include "ExchangeAgent.h"
#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

#include <memory>
#include <algorithm>
#include <functional>
#include <numeric>

#include <iostream>

ExchangeAgent::ExchangeAgent(const Simulation* simulation)
	: Agent(simulation), m_processingDelay(0), m_bookPtr(nullptr) { }

ExchangeAgent::ExchangeAgent(const Simulation* simulation, const std::string& name, const BookPtr& bookPtr, Timestamp processingDelay)
	: Agent(simulation, name), m_processingDelay(processingDelay), m_bookPtr(bookPtr) {

	std::function<void(TradePtr)> loggingCallbackBound = std::bind(&ExchangeAgent::notifyTradeSubscribers, this, std::placeholders::_1);
	bookPtr->registerTradeLoggingCallback(loggingCallbackBound);
}

void ExchangeAgent::receiveMessage(const MessagePtr& msg) {
	if (msg->type == "PLACE_ORDER_MARKET") {
		auto ptr = std::dynamic_pointer_cast<PlaceOrderMarketPayload>(msg->payload);
		auto mop = m_bookPtr->placeMarketOrder(ptr->direction, msg->arrival, ptr->volume);
		
		PlaceOrderMarketResponsePayload retpay(mop->id(), ptr);
		auto retpayptr = std::make_shared<PlaceOrderMarketResponsePayload>(retpay);

		respondToMessage(msg, retpayptr, m_processingDelay);

		notifyMarketOrderSubscribers(mop);
	} else if (msg->type == "PLACE_ORDER_LIMIT") {
		auto ptr = std::dynamic_pointer_cast<PlaceOrderLimitPayload>(msg->payload);
		auto lop = m_bookPtr->placeLimitOrder(ptr->direction, msg->arrival, ptr->volume, ptr->price);

		PlaceOrderLimitResponsePayload retpay(lop->id(), ptr);
		auto retpayptr = std::make_shared<PlaceOrderLimitResponsePayload>(retpay);

		respondToMessage(msg, retpayptr, m_processingDelay);

		notifyLimitOrderSubscribers(lop);
	} else if (msg->type == "RETRIEVE_ORDERS") {
		auto pptr = std::dynamic_pointer_cast<RetrieveOrdersPayload>(msg->payload);
		auto retpptr = std::make_shared<RetrieveOrdersResponsePayload>();
		for (OrderID id : pptr->ids) {
			LimitOrderPtr lop;
			if (m_bookPtr->tryGetOrder(id, lop)) {
				retpptr->orders.push_back(*lop);
			}
		}

		respondToMessage(msg, retpptr);
	} else if (msg->type == "CANCEL_ORDERS") {
		auto pptr = std::dynamic_pointer_cast<CancelOrdersPayload>(msg->payload);
		auto retpptr = std::make_shared<CancelOrdersPayload>();
		
		for (const auto& cancellation : pptr->cancellations) {
			auto cancellationCopy = cancellation;
			cancellationCopy.volume = m_bookPtr->cancelOrder(cancellation.id, cancellation.volume);
			retpptr->cancellations.push_back(cancellationCopy);
		}

		// NOTE: event [orderId no longer exists in the book] is a no-op
		// NOTE: might be woth implementing the processing delay as well, in one way or another (think about the error message about)
		respondToMessage(msg, retpptr, m_processingDelay);
	} else if (msg->type == "RETRIEVE_L1") {
		auto retpptr = std::make_shared<RetrieveL1ResponsePayload>();
		retpptr->time = simulation()->currentTimestamp();

		if (m_bookPtr->sellQueue().empty()) {
			retpptr->bestAskPrice = 0;
			retpptr->bestAskVolume = 0;
			retpptr->askTotalVolume = 0;
		} else {
			const auto& bestSellLevel = m_bookPtr->sellQueue().front();
			retpptr->bestAskPrice = bestSellLevel.price();
			retpptr->bestAskVolume = bestSellLevel.volume();
			retpptr->askTotalVolume = std::accumulate(m_bookPtr->sellQueue().begin(), m_bookPtr->sellQueue().end(), (Volume)0, [](Volume acc, const TickContainer& cont) {
				return acc + cont.volume();
			});
		}

		if (m_bookPtr->buyQueue().empty()) {
			retpptr->bestBidPrice = 0;
			retpptr->bestBidVolume = 0;
			retpptr->bidTotalVolume = 0;
		} else {
			const auto& bestBuyLevel = m_bookPtr->buyQueue().back();
			retpptr->bestBidPrice = bestBuyLevel.price();
			retpptr->bestBidVolume = bestBuyLevel.volume();
			retpptr->bidTotalVolume = std::accumulate(m_bookPtr->buyQueue().begin(), m_bookPtr->buyQueue().end(), (Volume)0, [](Volume acc, const TickContainer& cont) {
				return acc + cont.volume();
			});
		}

		respondToMessage(msg, retpptr);
	} else if (msg->type == "RETRIEVE_BOOK_ASK") {
		auto pptr = std::dynamic_pointer_cast<RetrieveBookPayload>(msg->payload);
		auto retpptr = std::make_shared<RetrieveBookResponsePayload>(simulation()->currentTimestamp());
		
		unsigned int actualDepth = (unsigned int)std::min((size_t)pptr->depth, m_bookPtr->sellQueue().size());
		const auto beg = m_bookPtr->sellQueue().cbegin();
		auto end = beg;
		std::advance(end, actualDepth);
		retpptr->tickContainers.reserve(actualDepth);
		std::copy(beg, end, std::back_inserter(retpptr->tickContainers));

		respondToMessage(msg, retpptr);
	} else if (msg->type == "RETRIEVE_BOOK_BID") {
		auto pptr = std::dynamic_pointer_cast<RetrieveBookPayload>(msg->payload);
		auto retpptr = std::make_shared<RetrieveBookResponsePayload>(simulation()->currentTimestamp());

		unsigned int actualDepth = (unsigned int)std::min((size_t)pptr->depth, m_bookPtr->buyQueue().size());
		const auto beg = m_bookPtr->buyQueue().crbegin();
		auto end = beg;
		std::advance(end, actualDepth);
		retpptr->tickContainers.reserve(actualDepth);
		std::copy(beg, end, std::back_inserter(retpptr->tickContainers));

		respondToMessage(msg, retpptr);
	} else if (msg->type == "SUBSCRIBE_EVENT_ORDER_MARKET") { 
		if (std::binary_search(m_marketOrderSubscribers.begin(), m_marketOrderSubscribers.end(), msg->source)) {
			auto eretpptr = std::make_shared<ErrorResponsePayload>("The agent is already subscribed to order events: " + msg->source);
			fastRespondToMessage(msg, eretpptr);
		} else {
			auto iit = std::upper_bound(m_marketOrderSubscribers.begin(), m_marketOrderSubscribers.end(), msg->source);
			m_marketOrderSubscribers.insert(iit, msg->source);

			auto sretpptr = std::make_shared<SuccessResponsePayload>("Agent subscribed successfully to order events: " + msg->source);
			fastRespondToMessage(msg, sretpptr);
		}
	} else if (msg->type == "SUBSCRIBE_EVENT_ORDER_LIMIT") {
		if (std::binary_search(m_limitOrderSubscribers.begin(), m_limitOrderSubscribers.end(), msg->source)) {
			auto eretpptr = std::make_shared<ErrorResponsePayload>("The agent is already subscribed to order events: " + msg->source);
			fastRespondToMessage(msg, eretpptr);
		} else {
			auto iit = std::upper_bound(m_limitOrderSubscribers.begin(), m_limitOrderSubscribers.end(), msg->source);
			m_limitOrderSubscribers.insert(iit, msg->source);

			auto sretpptr = std::make_shared<SuccessResponsePayload>("Agent subscribed successfully to order events: " + msg->source);
			fastRespondToMessage(msg, sretpptr);
		}
	} else if (msg->type == "SUBSCRIBE_EVENT_TRADE") {
		if (std::binary_search(m_tradeSubscribers.begin(), m_tradeSubscribers.end(), msg->source)) {
			auto eretpptr = std::make_shared<ErrorResponsePayload>("The agent is already subscribed to trade events: " + msg->source);
			fastRespondToMessage(msg, eretpptr);
		} else {
			auto iit = std::upper_bound(m_tradeSubscribers.begin(), m_tradeSubscribers.end(), msg->source);
			m_tradeSubscribers.insert(iit, msg->source);

			auto sretpptr = std::make_shared<SuccessResponsePayload>("Agent subscribed successfully to trade events: " + msg->source);
			fastRespondToMessage(msg, sretpptr);
		}
	} else if (msg->type == "SUBSCRIBE_EVENT_ORDER_TRADE") {
		auto pptr = std::dynamic_pointer_cast<SubscribeEventTradeByOrderPayload>(msg->payload);
		if (m_tradeByOrderSubscribers.count(pptr->id) == 0) {
			m_tradeByOrderSubscribers[pptr->id] = std::vector<std::string>();
		}

		auto& subscribers = m_tradeByOrderSubscribers[pptr->id];
		if (std::binary_search(subscribers.begin(), subscribers.end(), msg->source)) {
			auto eretpptr = std::make_shared<ErrorResponsePayload>("The agent is already subscribed to trade events for order " + std::to_string(pptr->id) + ":" + msg->source);
			fastRespondToMessage(msg, eretpptr);
		} else {
			auto iit = std::upper_bound(subscribers.begin(), subscribers.end(), msg->source);
			subscribers.insert(iit, msg->source);

			auto sretpptr = std::make_shared<SuccessResponsePayload>("Agent subscribed to trade events for order " + std::to_string(pptr->id) + ":" + msg->source);
			fastRespondToMessage(msg, sretpptr);
		}
	} else {
		auto retpptr = std::make_shared<ErrorResponsePayload>("Unrecognized request type: " + msg->type);

		fastRespondToMessage(msg, retpptr);
	}
}

#include "PriceTimeBook.h"
#include "PureProRataBook.h"
#include "PriorityProRataBook.h"
#include "TimeProRataBook.h"
#include "SimulationException.h"
#include "ParameterStorage.h"

void ExchangeAgent::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	Agent::configure(node, configurationPath);

	pugi::xml_attribute att;
	if (!(att = node.attribute("algorithm")).empty()) {
		std::string algorithm = simulation()->parameters().processString(att.as_string());
		std::function<void(TradePtr)> loggingCallbackBound = std::bind(&ExchangeAgent::notifyTradeSubscribers, this, std::placeholders::_1);
		
		auto orderFactoryPtr = std::make_shared<OrderFactory>();
		auto tradeFactoryPtr = std::make_shared<TradeFactory>();
		if (algorithm == "PriceTime") {
			m_bookPtr = std::make_shared<PriceTimeBook>(orderFactoryPtr, tradeFactoryPtr);
			m_bookPtr->registerTradeLoggingCallback(loggingCallbackBound);
		} else if (algorithm == "PureProRata") {
			m_bookPtr = std::make_shared<PureProRataBook>(orderFactoryPtr, tradeFactoryPtr);
			m_bookPtr->registerTradeLoggingCallback(loggingCallbackBound);
		} else if (algorithm == "PriorityProRata") {
			m_bookPtr = std::make_shared<PriorityProRataBook>(orderFactoryPtr, tradeFactoryPtr);
			m_bookPtr->registerTradeLoggingCallback(loggingCallbackBound);
		} else if (algorithm == "TimeProRata") {
			m_bookPtr = std::make_shared<TimeProRataBook>(orderFactoryPtr, tradeFactoryPtr);
			m_bookPtr->registerTradeLoggingCallback(loggingCallbackBound);
		} else {
			throw SimulationException("ExchangeAgent::configure(): unknown algorithm '" + algorithm + "'");
		}
	}

	if (!(att = node.attribute("processingDelay")).empty()) {
		std::string pd = simulation()->parameters().processString(att.as_string());
		m_processingDelay = std::stoull(pd);
	}
}

void ExchangeAgent::notifyMarketOrderSubscribers(MarketOrderPtr ptr) {
	auto currentTimestamp = simulation()->currentTimestamp();
	for(const std::string& subscriber : m_marketOrderSubscribers) {
		auto pptr = std::make_shared<EventOrderMarketPayload>(*ptr);
		simulation()->dispatchMessage(currentTimestamp, m_processingDelay, name(), subscriber, "EVENT_ORDER_MARKET", pptr);
	}
}

void ExchangeAgent::notifyLimitOrderSubscribers(LimitOrderPtr ptr) {
	auto currentTimestamp = simulation()->currentTimestamp();
	for (const std::string& subscriber : m_limitOrderSubscribers) {
		auto pptr = std::make_shared<EventOrderLimitPayload>(*ptr);
		simulation()->dispatchMessage(currentTimestamp, m_processingDelay, name(), subscriber, "EVENT_ORDER_LIMIT", pptr);
	}
}

void ExchangeAgent::notifyTradeSubscribers(TradePtr tradePtr) {
	const auto currentTimestamp = simulation()->currentTimestamp();
	tradePtr->setTimestamp(currentTimestamp); // the trade happens exactly on the receipt of the aggressing order, no processing delay there; the processing delay only kicks in sending out a response and events related to the matching

	for (const std::string& subscriber : m_tradeSubscribers) {
		auto pptr = std::make_shared<EventTradePayload>(*tradePtr);
		simulation()->dispatchMessage(currentTimestamp, m_processingDelay, name(), subscriber, "EVENT_TRADE", pptr);
	}

	notifyTradeSubscribersByOrderID(tradePtr, tradePtr->aggressingOrderID());
	notifyTradeSubscribersByOrderID(tradePtr, tradePtr->restingOrderID());
}

void ExchangeAgent::notifyTradeSubscribersByOrderID(TradePtr tradePtr, OrderID orderId) {
	const auto currentTimestamp = simulation()->currentTimestamp();
	if (m_tradeByOrderSubscribers.count(orderId) > 0) {
		const auto& subscribers = m_tradeByOrderSubscribers[orderId];
		for (const std::string& subscriber : subscribers) {
			auto pptr = std::make_shared<EventTradePayload>(*tradePtr);
			simulation()->dispatchMessage(currentTimestamp, m_processingDelay, name(), subscriber, "EVENT_TRADE", pptr);
		}
	}
}
