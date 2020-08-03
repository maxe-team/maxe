#pragma once

#include "Agent.h"
#include "Book.h"

#include <list>
#include <map>

class ExchangeAgent : public Agent {
public:
	ExchangeAgent(const Simulation* simulation);
	ExchangeAgent(const Simulation* simulation, const std::string& name, const BookPtr& bookPtr, Timestamp processingDelay = 0);
	virtual ~ExchangeAgent() = default;

	void receiveMessage(const MessagePtr& msg) override;

	Timestamp processingDelay() const { return m_processingDelay; }

	void configure(const pugi::xml_node& node, const std::string& configurationPath) override;
private:
	Timestamp m_processingDelay;
	BookPtr m_bookPtr;

	std::list<std::string> m_marketOrderSubscribers;
	std::list<std::string> m_limitOrderSubscribers;
	std::list<std::string> m_tradeSubscribers;
	std::map<OrderID, std::vector<std::string>> m_tradeByOrderSubscribers;

	void notifyMarketOrderSubscribers(MarketOrderPtr ptr);
	void notifyLimitOrderSubscribers(LimitOrderPtr ptr);
	void notifyTradeSubscribers(TradePtr tradePtr);
	void notifyTradeSubscribersByOrderID(TradePtr tradePtr, OrderID orderId);
};
