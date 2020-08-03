#pragma once

#include "IHumanPrintable.h"
#include "ICSVPrintable.h"
#include "Order.h"

#include <memory>
#include <map>
#include <list>

class OrderFactory {
public:
	OrderFactory();
	OrderFactory(const OrderFactory& orderFactory) = default;
	OrderFactory(OrderFactory&& orderFactory) noexcept;
	~OrderFactory();

	MarketOrderPtr makeMarketOrder(OrderDirection direction, Timestamp timestamp, Volume volume);
	LimitOrderPtr makeLimitOrder(OrderDirection direction, Timestamp timestamp, Volume volume, Money price);

	// convenience methods
	MarketOrderPtr marketBuy(Timestamp timestamp, Volume volume);
	MarketOrderPtr marketSell(Timestamp timestamp, Volume volume);
	LimitOrderPtr limitBuy(Timestamp timestamp, Volume volume, Money price);
	LimitOrderPtr limitSell(Timestamp timestamp, Volume volume, Money price);
private:
	OrderID m_orderCount;
};
using OrderFactoryPtr = std::shared_ptr<OrderFactory>;

