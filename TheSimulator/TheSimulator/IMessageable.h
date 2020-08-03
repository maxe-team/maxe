#pragma once

#include "Message.h"

class Simulation;

class IMessageable {
public:
	const std::string& name() const { return m_name; }
	const Simulation* simulation() const { return m_simulation; }
	
	virtual void receiveMessage(const MessagePtr& msg) = 0;
	virtual void respondToMessage(const MessagePtr& msg, const std::string& type, MessagePayloadPtr payload, Timestamp processingDelay = 0) const;
	virtual void respondToMessage(const MessagePtr& msg, MessagePayloadPtr payload, Timestamp processingDelay = 0) const;
	virtual void fastRespondToMessage(const MessagePtr& msg, const std::string& type, MessagePayloadPtr payload, Timestamp processingDelay = 0) const;
	virtual void fastRespondToMessage(const MessagePtr& msg, MessagePayloadPtr payload, Timestamp processingDelay = 0) const;
protected:
	IMessageable(const Simulation* simulation, const std::string& name);
	virtual ~IMessageable() = default;

	void setName(const std::string& name) { m_name = name; }
private:
	const Simulation* const m_simulation;
	std::string m_name;
};