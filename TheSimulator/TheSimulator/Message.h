#pragma once

#include "Timestamp.h"
#include <string>
#include <vector>

#include "split.h"
#include <memory>

#include "MessagePayload.h"

struct Message {
public:
	Message(Timestamp occurrence, Timestamp arrival, const std::string& source, const std::string& target, const std::string& type, MessagePayloadPtr payload)
		: occurrence(occurrence), arrival(arrival), source(source), type(type), payload(payload) {
		this->targets = std::move(split(target, '|'));
	}

	Message(Timestamp occurrence, Timestamp arrival, const std::string& source, const std::vector<std::string>& targets, const std::string& type, MessagePayloadPtr payload)
		: occurrence(occurrence), arrival(arrival), source(source), targets(targets), type(type), payload(payload) { }
	
	~Message() = default;

	Timestamp occurrence;
	Timestamp arrival;

	std::string source;
	std::vector<std::string> targets;
	std::string type;

	MessagePayloadPtr payload;
};
using MessagePtr = std::shared_ptr<Message>;