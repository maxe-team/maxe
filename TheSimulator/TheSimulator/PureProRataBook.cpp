#include "PureProRataBook.h"

#include <numeric>
#include <algorithm>

PureProRataBook::PureProRataBook(OrderFactoryPtr orderFactory, TradeFactoryPtr makeRecord)
	: Book(orderFactory, makeRecord) { }

void PureProRataBook::processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) {
	auto* bestBuyList = &m_buyQueue.back();
	while (order->volume() > 0 && bestBuyList->price() >= minPrice) {
		auto partialVolumes = this->computePartialVolumes(order->volume(), bestBuyList);

		for (const auto& orderVolumePair : partialVolumes) {
			orderVolumePair.first->removeVolume(orderVolumePair.second);
			order->removeVolume(orderVolumePair.second);
			if (orderVolumePair.second > 0) {
				logTrade(OrderDirection::Sell, order->id(), orderVolumePair.first->id(), orderVolumePair.second, bestBuyList->price());
			}
		}

		// FIFO on the cummulative remainder from rounding down
		auto it = bestBuyList->begin();
		while (it != bestBuyList->end()) {
			const Volume applicableVolume = std::min((*it)->volume(), order->volume());
			(*it)->removeVolume(applicableVolume);
			order->removeVolume(applicableVolume);
			if (applicableVolume > 0) {
				logTrade(OrderDirection::Sell, order->id(), (*it)->id(), applicableVolume, bestBuyList->price());
			}

			if ((*it)->volume() == 0) {
				unregisterLimitOrder(*it);
				bestBuyList->erase(it);
				it = bestBuyList->begin();
			} else {
				++it;
			}
		}

		if (bestBuyList->empty()) {
			m_buyQueue.pop_back();
			if (m_buyQueue.empty()) {
				break;
			}
			bestBuyList = &m_buyQueue.back();
		}
	}
}

void PureProRataBook::processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) {
	auto* bestSellList = &m_sellQueue.front();
	while (order->volume() > 0 && bestSellList->price() <= maxPrice) {
		std::vector<std::pair<LimitOrderPtr, Volume>> partialVolumes = computePartialVolumes(order->volume(), bestSellList);

		for (const auto& orderVolumePair : partialVolumes) {
			orderVolumePair.first->removeVolume(orderVolumePair.second);
			order->removeVolume(orderVolumePair.second);
			if (orderVolumePair.second > 0) {
				logTrade(OrderDirection::Buy, order->id(), orderVolumePair.first->id(), orderVolumePair.second, bestSellList->price());
			}
		}

		// FIFO on the cummulative remainder from rounding down
		auto it = bestSellList->begin();
		while(it != bestSellList->end()) {
			const Volume applicableVolume = std::min((*it)->volume(), order->volume());
			(*it)->removeVolume(applicableVolume);
			order->removeVolume(applicableVolume);
			if(applicableVolume > 0) {
				logTrade(OrderDirection::Sell, order->id(), (*it)->id(), applicableVolume, bestSellList->price());
			}

			if ((*it)->volume() == 0) {
				unregisterLimitOrder(*it);
				bestSellList->erase(it);
				it = bestSellList->begin();
			} else {
				++it;
			}
		}

		if (bestSellList->empty()) {
			m_sellQueue.pop_front();
			if (m_sellQueue.empty()) {
				break;
			}
			bestSellList = &m_sellQueue.front();
		}
	}
}

std::vector<std::pair<LimitOrderPtr, Volume>> PureProRataBook::computePartialVolumes(Volume incomingVolume, TickContainer* bestList) {
	const Volume availableVolume = std::accumulate(bestList->begin(), bestList->end(), (Volume)0, [](Volume v, const LimitOrderPtr& iop) {
		return v + iop->volume();
	});

	std::vector<std::pair<LimitOrderPtr, Volume>> partialVolumes;
	partialVolumes.reserve(bestList->size()); // list size complextiy is constant from c++11

	float orderFraction = std::min((float)incomingVolume / availableVolume, 1.f);
	std::transform(bestList->cbegin(), bestList->cend(), std::back_inserter(partialVolumes), [orderFraction](const LimitOrderPtr& iop) {
		return std::pair<LimitOrderPtr, Volume>(iop, (Volume)std::floor(orderFraction * iop->volume()));
	});
	
	return partialVolumes;
}

