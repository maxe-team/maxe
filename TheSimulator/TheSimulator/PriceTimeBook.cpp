#include "PriceTimeBook.h"

PriceTimeBook::PriceTimeBook(OrderFactoryPtr orderFactory, TradeFactoryPtr tradeFactory)
	: Book(orderFactory, tradeFactory) { }

void PriceTimeBook::processAgainstTheBuyQueue(const OrderPtr& order, Money minPrice) {
	auto* bestBuyDeque = &m_buyQueue.back();
	while (order->volume() > 0 && bestBuyDeque->price() >= minPrice) {
		LimitOrderPtr iop = bestBuyDeque->front();
		const Volume usedVolume = std::min(iop->volume(), order->volume());
		order->removeVolume(usedVolume);
		iop->removeVolume(usedVolume);
		if(usedVolume > 0) {
			logTrade(OrderDirection::Sell, order->id(), iop->id(), usedVolume, bestBuyDeque->price());
		}
		if (iop->volume() == 0) {
			bestBuyDeque->pop_front();
			unregisterLimitOrder(iop);
		}

		if (bestBuyDeque->empty()) {
			m_buyQueue.pop_back();
			if (m_buyQueue.empty()) {
				break;
			}
			bestBuyDeque = &m_buyQueue.back();
		}
	}
}

void PriceTimeBook::processAgainstTheSellQueue(const OrderPtr& order, Money maxPrice) {
	auto* bestSellDeque = &m_sellQueue.front();
	while (order->volume() > 0 && bestSellDeque->price() <= maxPrice) {
		LimitOrderPtr iop = bestSellDeque->front();
		const Volume usedVolume = std::min(iop->volume(), order->volume());
		order->removeVolume(usedVolume);
		iop->removeVolume(usedVolume);
		if (usedVolume > 0) {
			logTrade(OrderDirection::Buy, order->id(), iop->id(), usedVolume, bestSellDeque->price());
		}
		if (iop->volume() == 0) {
			bestSellDeque->pop_front();
			unregisterLimitOrder(iop);
		}

		if (bestSellDeque->empty()) {
			m_sellQueue.pop_front();
			if (m_sellQueue.empty()) {
				break;
			}
			bestSellDeque = &m_sellQueue.front();
		}
	}
}

TickDeque::TickDeque(Money price)
	: TickContainer(price) {
	
}
