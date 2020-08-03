#include "IMessageable.h"

#include "Simulation.h"

void IMessageable::respondToMessage(const MessagePtr& msg, const std::string& type, MessagePayloadPtr payload, Timestamp processingDelay) const {
	const Timestamp diff = msg->arrival - msg->occurrence;
	const Timestamp replyTime = msg->arrival + processingDelay;

	m_simulation->dispatchMessage(replyTime, diff, this->m_name, msg->source, type, payload);
}

void IMessageable::respondToMessage(const MessagePtr& msg, MessagePayloadPtr payload, Timestamp processingDelay) const {
	this->respondToMessage(msg, "RESPONSE_" + msg->type, payload, processingDelay);
}

void IMessageable::fastRespondToMessage(const MessagePtr& msg, const std::string& type, MessagePayloadPtr payload, Timestamp processingDelay) const {
	const Timestamp replyTime = msg->arrival + processingDelay;
	m_simulation->dispatchMessage(replyTime, 0, this->m_name, msg->source, type, payload);
}

void IMessageable::fastRespondToMessage(const MessagePtr& msg, MessagePayloadPtr payload, Timestamp processingDelay) const {
	this->fastRespondToMessage( msg, "RESPONSE_" + msg->type, payload, processingDelay);
}

IMessageable::IMessageable(const Simulation* simulation, const std::string& name)
	: m_simulation(simulation), m_name(name) { }
