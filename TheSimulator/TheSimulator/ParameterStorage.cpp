#include "ParameterStorage.h"

#include "SimulationException.h"

#include <algorithm>

void ParameterStorage::set(const std::string& name, const std::string& value) {
	m_parameterMap[name] = value;
}

const std::string& ParameterStorage::get(const std::string& name) {
	auto fit = m_parameterMap.find(name);
	if (fit != m_parameterMap.end()) {
		return fit->second;
	} else {
		throw SimulationException("ParameterStorage::get(): no parameter with name '" + name + "' is currently in the parameter storage");
	}
}

bool ParameterStorage::tryGet(const std::string& name, std::string& val) {
	auto fit = m_parameterMap.find(name);
	if (fit != m_parameterMap.end()) {
		val = fit->second;
		return true;
	} else {
		return false;
	}
}

std::string ParameterStorage::processString(const std::string& stringToProcess) {
	std::string ret;

	std::string::const_iterator it = stringToProcess.cbegin();
	const std::string::const_iterator endIt = stringToProcess.cend();
	do {
		const auto dollarIt = std::find(it, endIt, '$');
		ret.append(it, dollarIt);
		it = dollarIt;
		if (dollarIt != endIt) {
			++it;

			if (it != endIt) {
				if (*it == '{') {
					++it;
					const auto paramEndIt = std::find(it, endIt, '}');
					if (paramEndIt != endIt) {
						const std::string paramName = std::string(it, paramEndIt);
						std::string paramValue;
						if (tryGet(paramName, paramValue)) {
							ret.append(paramValue);
						} else {
							throw SimulationException("ParameterStorage::processString(): unknown parameter name '" + paramName + "' encountered in the string '" + stringToProcess + "'");
						}
						it = paramEndIt;
						++it;
					} else {
						throw SimulationException("ParameterStorage::processString(): parameter reference opening '${' encountered but no matching closing bracket '}' found in the string '" + stringToProcess + "'");
					}
				} else {
					--it;
				}
			}
		}
	} while (it != endIt);

	return ret;
}

const std::string& ParameterStorage::operator[](const std::string& parameterName) const {
	auto fit = m_parameterMap.find(parameterName);
	if (fit != m_parameterMap.end()) {
		return fit->second;
	} else {
		throw SimulationException("ParameterStorage::operator[](): no parameter with name '" + parameterName + "' is currently in the parameter storage");
	}
}

std::string& ParameterStorage::operator[](const std::string& parameterName) {
	auto fit = m_parameterMap.find(parameterName);
	if (fit != m_parameterMap.end()) {
		return fit->second;
	} else {
		return m_parameterMap[parameterName];
	}
}