#include "PriorityProRataBook.h"

PriorityProRataBook::PriorityProRataBook(OrderFactoryPtr orderFactory, TradeFactoryPtr makeRecord)
	: PureProRataBook(orderFactory, makeRecord) { }

void PriorityProRataBook::processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) {
	const auto& bestBuyList = m_buyQueue.back();
	if (order->volume() > 0 && bestBuyList.price() >= minPrice && m_lastBetteringBuyOrder->volume() > 0) {
		const Volume effectiveVolume = std::min(order->volume(), m_lastBetteringBuyOrder->volume());
		order->removeVolume(effectiveVolume);
		m_lastBetteringBuyOrder->removeVolume(effectiveVolume);
		if(effectiveVolume > 0) {
			logTrade(OrderDirection::Sell, order->id(), m_lastBetteringBuyOrder->id(), effectiveVolume, bestBuyList.price());
		}

		if (m_lastBetteringBuyOrder->volume() == 0) {
			m_buyQueue.back().remove(m_lastBetteringBuyOrder);
		}
	}

	this->PureProRataBook::processAgainstTheBuyQueue(order, minPrice);
}

void PriorityProRataBook::processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) {
	const auto& bestSellList = m_sellQueue.front();
	if (order->volume() > 0 && bestSellList.price() <= maxPrice && m_lastBetteringSellOrder->volume() > 0) {
		const Volume effectiveVolume = std::min(order->volume(), m_lastBetteringSellOrder->volume());
		order->removeVolume(effectiveVolume);
		m_lastBetteringSellOrder->removeVolume(effectiveVolume);
		if (effectiveVolume > 0) {
			logTrade(OrderDirection::Buy, order->id(), m_lastBetteringSellOrder->id(), effectiveVolume, bestSellList.price());
		}

		if (m_lastBetteringSellOrder->volume() == 0) {
			m_sellQueue.back().remove(m_lastBetteringSellOrder);
		}
	}

	this->PureProRataBook::processAgainstTheSellQueue(order, maxPrice);
}