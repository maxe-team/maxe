#pragma once

#pragma warning( suppress : 4250 )

#include <list>

#include "Book.h"
#include "PureProRataBook.h"

class TimeProRataBook : public PureProRataBook {
public:
	TimeProRataBook(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr);

protected:
	std::vector<std::pair<LimitOrderPtr, Volume>> computePartialVolumes(Volume incomingVolume, TickContainer* bestList) override;
};
