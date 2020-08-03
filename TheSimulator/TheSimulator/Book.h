#pragma once

#include <memory>
#include <queue>
#include <map>
#include <functional>
#include <algorithm>
#include <numeric>

#include "OrderFactory.h"
#include "TradeFactory.h"

#include "ICSVPrintable.h"
#include "IHumanPrintable.h"

class TickContainer : public std::list<LimitOrderPtr> {
public:
	TickContainer(Money price);

	Money price() const { return m_price; }
	Volume volume() const {
		return std::accumulate(cbegin(), cend(), (Volume)0, [](Volume soFar, const LimitOrderPtr& lop) {
			return soFar + lop->volume();
		});
	};
private:
	Money m_price;
};

template<class TickContainer>
using OrderContainer = std::deque<TickContainer>;

using TradeLoggingCallback = std::function<void(TradePtr)>;

class Book : public IHumanPrintable, public ICSVPrintable {
public:
	Book(OrderFactoryPtr orderFactoryPtr, TradeFactoryPtr tradeFactoryPtr);
	virtual ~Book() = default;

	MarketOrderPtr placeMarketOrder(OrderDirection direction, Timestamp timestamp, Volume volume);
	LimitOrderPtr placeLimitOrder(OrderDirection direction, Timestamp timestamp, Volume volume, Money price);
	void cancelOrder(const OrderID orderId);
	Volume cancelOrder(const OrderID orderId, Volume volumeToCancel);

	bool tryGetOrder(OrderID id, LimitOrderPtr& orderPtr) const;

	const OrderContainer<TickContainer>& buyQueue() const { return m_buyQueue; }
	const OrderContainer<TickContainer>& sellQueue() const { return m_sellQueue; }

	void printHuman() const override;
	void printCSV() const override;
	void printHuman(unsigned int depth) const;
	void printCSV(unsigned int depth) const;

	using IHumanPrintable::print;

	const OrderFactoryPtr& orderFactory() const { return m_orderRecordPtr; }
	const TradeFactoryPtr& tradeFactory() const { return m_tradeRecordPtr; }

	void registerTradeLoggingCallback(TradeLoggingCallback tradeLogginCallbackToRegister);
protected:
	void placeOrder(const MarketOrderPtr& order);
	void placeOrder(const LimitOrderPtr& order);

	void registerLimitOrder(const LimitOrderPtr& order);
	void unregisterLimitOrder(const LimitOrderPtr& order);
	std::map<OrderID, LimitOrderPtr> m_orderIdMap;

	OrderContainer<TickContainer> m_buyQueue;
	LimitOrderPtr m_lastBetteringBuyOrder;
	OrderContainer<TickContainer> m_sellQueue;
	LimitOrderPtr m_lastBetteringSellOrder;

	virtual void processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) = 0; // you want to keep it this way
	virtual void processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) = 0;

	void logTrade(OrderDirection direction, OrderID aggressorId, OrderID restingId, Volume volume, Money execPrice);
private:
	OrderFactoryPtr m_orderRecordPtr;
	TradeFactoryPtr m_tradeRecordPtr;
	TradeLoggingCallback m_tradeLoggingCallback;
	
	template <class CIteratorType>
	void dumpHumanLOB(CIteratorType begin, CIteratorType end, unsigned int depth) const;
	template <class CIteratorType>
	void dumpCSVLOB(CIteratorType begin, CIteratorType end, unsigned int depth) const;
};
using BookPtr = std::shared_ptr<Book>;

#include <numeric>
#include <iostream>
#include <string>

template<class CIteratorType>
inline void Book::dumpHumanLOB(CIteratorType begin, CIteratorType end, unsigned int depth) const {
	while (depth > 0 && begin != end) {
		const Volume totalVolume = std::accumulate(begin->begin(), begin->end(), (Volume)0, [](Volume acc, const LimitOrderPtr& iop) {
			return acc + iop->volume();
		});

		std::cout << "\t" << ((Money)begin->price()).toCentString() << " (" + Money(totalVolume, 0).toPostfixedString(4) + ")";

		--depth;
		++begin;
	}
}

template<class CIteratorType>
void Book::dumpCSVLOB(CIteratorType begin, CIteratorType end, unsigned int depth) const {
	while (depth > 0 && begin != end) {
		const Volume totalVolume = std::accumulate(begin->begin(), begin->end(), (Volume)0, [](Volume acc, const LimitOrderPtr& iop) {
			return acc 
				+ iop->volume();
		});

		std::cout << "," << begin->price().toPostfixedString(3) << "," << std::to_string(totalVolume);

		--depth;
		++begin;
	}
}
