#include "Book.h"

TickContainer::TickContainer(Money price)
	: m_price(price), list() { }

Book::Book(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr)
	: m_orderRecordPtr(orderRecordPtr), m_tradeRecordPtr(tradeRecordPtr), m_tradeLoggingCallback([] (TradePtr) { }), m_buyQueue(), m_sellQueue(), m_orderIdMap(), m_lastBetteringBuyOrder(nullptr), m_lastBetteringSellOrder(nullptr) { }

void Book::placeOrder(const LimitOrderPtr& order) {
	if (order->direction() == OrderDirection::Sell) {
		if (m_buyQueue.empty() || order->price() > this->m_buyQueue.back().price()) {
			auto firstGreaterThan = m_sellQueue.end();
			for (auto it = m_sellQueue.begin(); it != m_sellQueue.end(); ++it) {
				if (it->price() >= order->price()) {
					firstGreaterThan = it;
					break;
				}
			}

			if (firstGreaterThan != m_sellQueue.end() && firstGreaterThan->price() == order->price()) {
				registerLimitOrder(order);
				firstGreaterThan->push_back(order);
			} else {
				TickContainer tov = TickContainer(order->price());
				registerLimitOrder(order);
				tov.push_back(order);
				m_sellQueue.insert(firstGreaterThan, tov);

				m_lastBetteringSellOrder = order;
			}
		} else {
			processAgainstTheBuyQueue(order, order->price());

			if (order->volume() > 0) {
				this->placeOrder(order);
			}
		}
	} else {
		if (m_sellQueue.empty() || order->price() < this->m_sellQueue.front().price()) {
			auto firstLessThan = m_buyQueue.rend();
			for (auto rit = m_buyQueue.rbegin(); rit != m_buyQueue.rend(); ++rit) {
				if (rit->price() <= order->price()) {
					firstLessThan = rit;
					break;
				}
			}

			if (firstLessThan != m_buyQueue.rend() && firstLessThan->price() == order->price()) {
				registerLimitOrder(order);
				firstLessThan->push_back(order);
			} else {
				TickContainer tov = TickContainer(order->price());
				registerLimitOrder(order);
				tov.push_back(order);
				m_buyQueue.insert(firstLessThan.base(), tov);

				m_lastBetteringBuyOrder = order;
			}
		} else {
			processAgainstTheSellQueue(order, order->price());

			if (order->volume() > 0) {
				this->placeOrder(order);
			}
		}
	}
}

void Book::placeOrder(const MarketOrderPtr& order) {
	if (order->direction() == OrderDirection::Sell) {
		if(!m_buyQueue.empty()) {
			processAgainstTheBuyQueue(order, -1e9); // don't ask
		} else {
			// auto p = placeLimitOrder(OrderDirection::Sell, order->timestamp(), order->volume(), m_lastBetteringSellOrder->price());  // we need setup agents to guarantee this is sensible
			// I think that the above line was only introduced to deal with the zero intelligence simulations. I do now strongly believe this case should be a no-op.
		}
	} else {
		if (!m_sellQueue.empty()) {
			processAgainstTheSellQueue(order, 1e9); // assuming nothing trades at 1BN per lot
		} else {
			// auto p = placeLimitOrder(OrderDirection::Buy, order->timestamp(), order->volume(), m_lastBetteringBuyOrder->price()); // we need setup agents to guarantee this is sensible
			// I think that the above line was only introduced to deal with the zero intelligence simulations. I do now strongly believe this case should be a no-op.
		}
	}
}

MarketOrderPtr Book::placeMarketOrder(OrderDirection direction, Timestamp timestamp, Volume volume) {
	auto ret = m_orderRecordPtr->makeMarketOrder(direction, timestamp, volume);
	placeOrder(ret);

	return ret;
}

LimitOrderPtr Book::placeLimitOrder(OrderDirection direction, Timestamp timestamp, Volume volume, Money price) {
	auto ret = m_orderRecordPtr->makeLimitOrder(direction, timestamp, volume, price);
	placeOrder(ret);

	return ret;
}

void Book::cancelOrder(const OrderID orderId) {
	// POLICY: even the filled and cancelled orders still survive in this hashmap, for future analysis
	// POLICY: action requested on a non-existing orderId is a no-op

	if (m_orderIdMap.count(orderId) > 0) {
		m_orderIdMap[orderId]->setVolume(0);
		m_orderIdMap.erase(orderId);
	}
}

Volume Book::cancelOrder(const OrderID orderId, Volume volumeToCancel) {
	// POLICY: even the filled and cancelled orders still survive in this hashmap, for future analysis
	// POLICY: action requested on a non-existing orderId is a no-op

	// returns remaining volume

	Volume remainingVolume = 0;
	if(m_orderIdMap.count(orderId) > 0) {
		const Volume originalVolume = m_orderIdMap[orderId]->volume();
		remainingVolume = std::min((Volume)0, originalVolume - volumeToCancel);
		m_orderIdMap[orderId]->setVolume(remainingVolume);
		if (remainingVolume == 0) {
			m_orderIdMap.erase(orderId);
		}
	}

	return remainingVolume;
}

bool Book::tryGetOrder(OrderID id, LimitOrderPtr& orderPtr) const {
	decltype(m_orderIdMap)::const_iterator it;
	if ((it = m_orderIdMap.find(id)) != m_orderIdMap.end()) {
		orderPtr = it->second;
		return true;
	} else {
		return false;
	}
}

void Book::printHuman() const {
	this->printHuman(5);
}

void Book::printCSV() const {
	this->printCSV(5);
}

void Book::printHuman(unsigned int depth) const {
	std::cout << "----------------" << std::endl;

	std::cout << "ask:";
	dumpHumanLOB(m_sellQueue.cbegin(), m_sellQueue.cend(), depth);
	std::cout << std::endl;

	std::cout << "bid:";
	dumpHumanLOB(m_buyQueue.crbegin(), m_buyQueue.crend(), depth);
	std::cout << std::endl << std::endl;
}

void Book::printCSV(unsigned int depth) const {
	std::cout << "ask";
	dumpCSVLOB(m_sellQueue.cbegin(), m_sellQueue.cend(), depth);
	std::cout << std::endl;

	std::cout << "bid";
	dumpCSVLOB(m_buyQueue.crbegin(), m_buyQueue.crend(), depth);
	std::cout << std::endl;

}

void Book::registerLimitOrder(const LimitOrderPtr& order) {
	m_orderIdMap[order->id()] = order;
}

void Book::unregisterLimitOrder(const LimitOrderPtr& order) {
	m_orderIdMap.erase(order->id());
}

void Book::logTrade(OrderDirection direction, OrderID aggressorId, OrderID restingId, Volume volume, Money execPrice) {
	TradePtr tradePtr = tradeFactory()->makeRecord(TIMESTAMP_INVALID, direction, aggressorId, restingId, volume, execPrice);
	m_tradeLoggingCallback(tradePtr);
}

void Book::registerTradeLoggingCallback(TradeLoggingCallback tradeLogginCallbackToRegister) {
	m_tradeLoggingCallback = tradeLogginCallbackToRegister;
}
