#pragma once

#pragma warning( suppress : 4250 )

#include <list>

#include "Book.h"

class PureProRataBook : public Book {
public:
	PureProRataBook(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr);

protected:
	void processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) override;
	void processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) override;

	virtual std::vector<std::pair<LimitOrderPtr, Volume>> computePartialVolumes(Volume incomingVolume, TickContainer* bestList);
};

