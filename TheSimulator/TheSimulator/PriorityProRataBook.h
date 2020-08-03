#pragma once

#include "PureProRataBook.h"

class PriorityProRataBook : public PureProRataBook {
public:
	PriorityProRataBook(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr);
protected:
	void processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) override;
	void processAgainstTheSellQueue(const OrderPtr& order, Money minPrice) override;
};

