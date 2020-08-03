#pragma once

#include <memory>
#include <map>
#include <string>

struct MessagePayload {
	virtual ~MessagePayload() = default; // for this type to be polymorphic
protected:
	MessagePayload() = default;
};
using MessagePayloadPtr = std::shared_ptr<MessagePayload>;

struct ErrorResponsePayload : public MessagePayload {
	std::string message;

	ErrorResponsePayload(const std::string& message) : message(message) { }
};

struct SuccessResponsePayload : public MessagePayload {
	std::string message;

	SuccessResponsePayload(const std::string& message) : message(message) { }
};


struct EmptyPayload : public MessagePayload {

};

struct GenericPayload : public MessagePayload, public std::map<std::string, std::string> {
	GenericPayload(const std::map<std::string, std::string>& initMap)
		: MessagePayload(), std::map<std::string, std::string>(initMap) { }
};