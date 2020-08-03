#include "TimeProRataBook.h"

TimeProRataBook::TimeProRataBook(OrderFactoryPtr orderRecordPtr, TradeFactoryPtr tradeRecordPtr)
	: PureProRataBook(orderRecordPtr, tradeRecordPtr) { }

std::vector<std::pair<LimitOrderPtr, Volume>> TimeProRataBook::computePartialVolumes(Volume incomingVolume, TickContainer* bestList) {
	const Volume availableVolume = std::accumulate(bestList->begin(), bestList->end(), (Volume)0, [](Volume v, const LimitOrderPtr& iop) {
		return v + iop->volume();
	});

	std::vector<std::pair<LimitOrderPtr, Volume>> partialVolumes;
	partialVolumes.reserve(bestList->size()); // list size complextiy is constant from c++11

	Volume volumePreceeding = 0;
	std::transform(bestList->cbegin(), bestList->cend(), std::back_inserter(partialVolumes), [incomingVolume, availableVolume, &volumePreceeding](const LimitOrderPtr& iop) {
		const Volume vOfThisOrder = iop->volume();
		const Volume v1 = availableVolume - volumePreceeding;
		const Volume v2 = v1 - vOfThisOrder;
		const float timeProRataFactor = ((float)(v1 * v1 - v2 * v2)) / (availableVolume * availableVolume);

		volumePreceeding += vOfThisOrder;

		return std::pair<LimitOrderPtr, Volume>(iop,
			std::min(vOfThisOrder, (Volume)std::floor(timeProRataFactor * incomingVolume))
		);
	});

	return partialVolumes;
}
