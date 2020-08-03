#pragma once

#include <deque>

#include "Book.h"

class TickDeque : public TickContainer, public std::deque<LimitOrderPtr> {
public:
	TickDeque(Money price);
};

class PriceTimeBook : public Book {
public:
	PriceTimeBook(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr);
protected:
	void processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) override;
	void processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) override;
};

