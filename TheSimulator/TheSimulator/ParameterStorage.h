#pragma once

#include <string>
#include <map>

class ParameterStorage {
public:
	void set(const std::string& name, const std::string& value);
	const std::string& get(const std::string& name);
	bool tryGet(const std::string& name, std::string& val);

	std::string processString(const std::string& stringToProcess);

	const std::string& operator[](const std::string& parameterName) const;
	std::string& operator[](const std::string& parameterName);
private:
	std::map<std::string, std::string> m_parameterMap;
};