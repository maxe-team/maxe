#include "TradeFactory.h"

TradeFactory::TradeFactory()
	: m_tradeCount(0) { }

TradePtr TradeFactory::makeRecord(Timestamp timestamp, OrderDirection direction, OrderID aggressingOrder, OrderID restingOrder, Volume volume, Money price) {
	++m_tradeCount;

	TradePtr ret = std::make_shared<Trade>(m_tradeCount, timestamp, direction, aggressingOrder, restingOrder, volume, price);

	return ret;
}