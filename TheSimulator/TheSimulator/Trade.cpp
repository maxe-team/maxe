#include "Trade.h"

Trade::Trade(TradeID id, Timestamp timestamp, OrderDirection direction, OrderID aggressingOrderID, OrderID restingOrderID, Volume volume, Money price)
	: m_id(id), m_timestamp(timestamp), m_direction(direction), m_aggressingOrderID(aggressingOrderID), m_restingOrderID(restingOrderID), m_volume(volume), m_price(price) { }

#include <iostream>

void Trade::printHuman() const {
	/*std::cout << std::to_string(m_id) << "\t"
		<< std::to_string(m_timestamp) << "\t"
		<< std::to_string(m_aggressingOrderID) << "\t"
		<< (m_direction == OrderDirection::Sell ? "SELL" : "BUY ") << "\t"
		<< std::to_string(m_restingOrderID) << "\t"
		<< std::to_string(m_volume) << "\t"
		<< m_price.toCentString();*/
	std::cout << "Trade " + std::to_string(m_id)
		<< " occurred at time " << std::to_string(m_timestamp)
		<< ", matching order " << std::to_string(m_aggressingOrderID) << " vs. " << std::to_string(m_restingOrderID) 
		<< " (written in the " << (m_direction == OrderDirection::Sell ? "SELL" : "BUY ") << " direction)"
		<< " with volume " << std::to_string(m_volume)
		<< " and price " << m_price.toCentString();
}

void Trade::printCSV() const {
	std::cout << std::to_string(m_id) << ","
		<< std::to_string(m_timestamp) << ","
		<< std::to_string(m_aggressingOrderID) << ","
		<< (m_direction == OrderDirection::Sell ? "SELL" : "BUY") << ","
		<< std::to_string(m_restingOrderID) << ","
		<< std::to_string(m_volume) << ","
		<< m_price.toFullString();
}