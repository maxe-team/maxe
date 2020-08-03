#pragma once

#include "IHumanPrintable.h"
#include "ICSVPrintable.h"

#include "Timestamp.h"
#include "Order.h"

#include <memory>

using TradeID = unsigned int;

class Trade : public IHumanPrintable, public ICSVPrintable {
public:
	Trade(TradeID id, Timestamp timestamp, OrderDirection direction, OrderID aggressingOrderID, OrderID restingOrderID, Volume volume, Money price);
	/*Trade(const Trade& trade)
		: m_id(trade.m_id), m_timestamp(trade.m_timestamp), m_direction(trade.m_direction), m_aggressingOrderID(trade.m_aggressingOrderID), m_restingOrderID(trade.m_restingOrderID), m_volume(trade.m_volume), m_price(trade.m_price) { }*/
	Trade(const Trade& trade) = default;
	Trade(Trade&& trade) = default;

	inline TradeID id() const { return m_id; }
	inline Timestamp timestamp() const { return m_timestamp; }
	inline void setTimestamp(Timestamp timestamp) { m_timestamp = timestamp; }
	inline OrderID aggressingOrderID() const { return m_aggressingOrderID; }
	inline OrderID restingOrderID() const { return m_restingOrderID; }
	inline Volume volume() const { return m_volume; }
	inline Money price() const { return m_price; }

	void printHuman() const override;
	void printCSV() const override;
private:
	TradeID m_id;
	OrderDirection m_direction;
	Timestamp m_timestamp;
	OrderID m_aggressingOrderID;
	OrderID m_restingOrderID;
	Volume m_volume;
	Money m_price;
};

using TradePtr = std::shared_ptr<Trade>;