#pragma once

#include "MessagePayload.h"

#include "Order.h"
#include "Trade.h"
#include "Book.h"

#include <vector>
#include <string>

struct PlaceOrderMarketPayload : public MessagePayload {
	OrderDirection direction;
	Volume volume;

	PlaceOrderMarketPayload(OrderDirection direction, Volume volume) : direction(direction), volume(volume) { }
};

struct PlaceOrderMarketResponsePayload : public MessagePayload {
	OrderID id;
	std::shared_ptr<PlaceOrderMarketPayload> requestPayload;

	PlaceOrderMarketResponsePayload(OrderID id, const std::shared_ptr<PlaceOrderMarketPayload>& requestPayload)
		: id(id), requestPayload(requestPayload) { }
};

struct PlaceOrderLimitPayload : public MessagePayload {
	OrderDirection direction;
	Volume volume;
	Money price;

	PlaceOrderLimitPayload(OrderDirection direction, Volume volume, Money price) : direction(direction), volume(volume), price(price) { }
};

struct PlaceOrderLimitResponsePayload : public MessagePayload {
	OrderID id;
	std::shared_ptr<PlaceOrderLimitPayload> requestPayload;

	PlaceOrderLimitResponsePayload(OrderID id, const std::shared_ptr<PlaceOrderLimitPayload>& requestPayload)
		: id(id), requestPayload(requestPayload) { }
};

struct RetrieveOrdersPayload : public MessagePayload {
	std::vector<OrderID> ids;

	RetrieveOrdersPayload(const std::vector<OrderID>& ids)
		: ids(ids) {}
};

struct RetrieveOrdersResponsePayload : public MessagePayload {
	std::vector<LimitOrder> orders;

	RetrieveOrdersResponsePayload() = default;
};

struct CancelOrdersCancellation {
	OrderID id;
	Volume volume;

	CancelOrdersCancellation(OrderID id, Volume volume) : id(id), volume(volume) { }
};

struct CancelOrdersPayload : public MessagePayload {
	std::vector<CancelOrdersCancellation> cancellations;

	CancelOrdersPayload()
		: cancellations() { }
	CancelOrdersPayload(const std::vector<CancelOrdersCancellation>& cancellations)
		: cancellations(cancellations) { }
};

struct RetrieveBookPayload : public MessagePayload {
	unsigned int depth;

	RetrieveBookPayload(unsigned int _)
		: depth(_) { }
};

struct RetrieveBookResponsePayload : public MessagePayload {
	Timestamp time;
	std::vector<TickContainer> tickContainers;

	RetrieveBookResponsePayload(Timestamp time)
		: RetrieveBookResponsePayload(time, std::vector<TickContainer>()) { }
	RetrieveBookResponsePayload(Timestamp time, const std::vector<TickContainer>& tickContainers)
		: time(time), tickContainers(tickContainers) { }
};

struct RetrieveL1Payload : public MessagePayload { };

struct RetrieveL1ResponsePayload : public MessagePayload {
	Timestamp time;

	Money bestAskPrice;
	Volume bestAskVolume;
	Volume askTotalVolume;
	Money bestBidPrice;
	Volume bestBidVolume;
	Volume bidTotalVolume;

	RetrieveL1ResponsePayload() = default;
	RetrieveL1ResponsePayload(Timestamp time, Money bestAskPrice, Volume bestAskVolume, Volume askTotalVolume, Money bestBidPrice, Volume bestBidVolume, Volume bidTotalVolume)
		: time(time), bestAskPrice(bestAskPrice), bestAskVolume(bestAskVolume), askTotalVolume(askTotalVolume), bestBidPrice(bestBidPrice), bestBidVolume(bestBidVolume), bidTotalVolume(bidTotalVolume) { }
};

struct SubscribeEventTradeByOrderPayload : public MessagePayload {
	OrderID id;

	SubscribeEventTradeByOrderPayload(OrderID id) : id(id) { }
};

struct EventOrderMarketPayload : public MessagePayload {
	MarketOrder order;

	EventOrderMarketPayload(const MarketOrder& order) : order(order) { }
};

struct EventOrderLimitPayload : public MessagePayload {
	LimitOrder order;

	EventOrderLimitPayload(const LimitOrder& order) : order(order) { }
};

struct EventTradePayload : public MessagePayload {
	Trade trade;

	EventTradePayload(const Trade& trade) : trade(trade) { }
};