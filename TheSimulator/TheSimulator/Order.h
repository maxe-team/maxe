#pragma once

#include "Timestamp.h"
#include "Volume.h"
#include "Money.h"

#include "IHumanPrintable.h"
#include "ICSVPrintable.h"

#include <memory>

using OrderID = unsigned long int;
constexpr OrderID ORDERID_INVALID = 0;

enum class OrderDirection : unsigned int {
	Buy,
	Sell
};

class BasicOrder : public IHumanPrintable, public ICSVPrintable {
public:
	inline OrderID id() const { return m_id; }
	inline Timestamp timestamp() const { return m_timestamp;  }
	inline Volume volume() const { return m_volume; }

	void printHuman() const override;
	void printCSV() const override;

	void removeVolume(Volume decrease) { m_volume -= decrease; }
protected:
	BasicOrder(OrderID id, Timestamp timestamp, Volume orderVolume);

	void setVolume(Volume newVolume) { m_volume = newVolume; }

	friend class Book;
private:
	OrderID m_id;
	Timestamp m_timestamp;
	Volume m_volume;
};

class Order : public BasicOrder {
public:
	virtual ~Order() = default;
	inline OrderDirection direction() const { return m_direction; }

	Order(const Order& order);
	Order(Order&& order);

	void printHuman() const override;
	void printCSV() const override;

protected:
	Order(OrderID id, OrderDirection orderDirection, Timestamp timestamp, Volume orderVolume);
private:
	const OrderDirection m_direction;
};
using OrderPtr = std::shared_ptr<Order>;

class MarketOrder : public Order {
public:
	MarketOrder(const MarketOrder& order) = default;
	MarketOrder(MarketOrder&& order) = default;

	void printHuman() const override;
	void printCSV() const override;
protected:
	MarketOrder(OrderID id, OrderDirection direction, Timestamp timestamp, Volume volume);

	friend class OrderFactory;
};
using MarketOrderPtr = std::shared_ptr<MarketOrder>;

class LimitOrder : public Order {
public:
	LimitOrder(const LimitOrder& order) = default;
	LimitOrder(LimitOrder&& order) = default;

	inline Money price() const { return m_price; };

	void printHuman() const override;
	void printCSV() const override;

	LimitOrder& operator=(const LimitOrder& _) { 
		// TODO: WARNING: DANGER: BAD CODE, RETHINK THIS WHOLE HEADER FILE
		return *this;
	}
protected:
	LimitOrder(OrderID id, OrderDirection direction, Timestamp timestamp, Volume volume, const Money& price);

	friend class OrderFactory;
private:
	const Money m_price;
 };
using LimitOrderPtr = std::shared_ptr<LimitOrder>;