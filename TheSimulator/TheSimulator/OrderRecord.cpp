#include "OrderFactory.h"
#include "Order.h"

OrderFactory::OrderFactory()
	: m_orderCount(0) { }

OrderFactory::OrderFactory(OrderFactory&& orderFactory) noexcept
	: m_orderCount(orderFactory.m_orderCount) { }

OrderFactory::~OrderFactory() {
	
}

MarketOrderPtr OrderFactory::makeMarketOrder(OrderDirection direction, Timestamp timestamp, Volume volume) {
	++m_orderCount;

	MarketOrderPtr op = MarketOrderPtr(new MarketOrder(m_orderCount, direction, timestamp, volume)); // has to be explicit because make_shared can't make use of friendships

	return op;
}

LimitOrderPtr OrderFactory::makeLimitOrder(OrderDirection direction, Timestamp timestamp, Volume volume, Money price) {
	++m_orderCount;

	LimitOrderPtr op = LimitOrderPtr(new LimitOrder(m_orderCount, direction, timestamp, volume, price)); // has to be explicit because make_shared can't make use of friendships

	return op;
}

MarketOrderPtr OrderFactory::marketBuy(Timestamp timestamp, Volume volume) {
	return makeMarketOrder(OrderDirection::Buy, timestamp, volume);
}

MarketOrderPtr OrderFactory::marketSell(Timestamp timestamp, Volume volume) {
	return makeMarketOrder(OrderDirection::Sell, timestamp, volume);
}

LimitOrderPtr OrderFactory::limitBuy(Timestamp timestamp, Volume volume, Money price) {
	return makeLimitOrder(OrderDirection::Buy, timestamp, volume, price);
}

LimitOrderPtr OrderFactory::limitSell(Timestamp timestamp, Volume volume, Money price) {
	return makeLimitOrder(OrderDirection::Sell, timestamp, volume, price);
}

#include <iostream>