#include "Simulation.h"

#include "ExchangeAgent.h"
#include "TradeLogAgent.h"
#include "OrderLogAgent.h"
#include "L1LogAgent.h"
#include "BouchaudAgent.h"
#include "ImpactAgent.h"
#include "SetupAgent.h"
#include "AdaptiveOfferingAgent.h"
#include "RandomWalkMarketMakerAgent.h"
#include "DoobAgent.h"
#include "PythonAgent.h"

#include <algorithm>
#include <filesystem>

#include "SimulationException.h"
#include "ParameterStorage.h"

Simulation::Simulation(ParameterStorage* parameters)
	: Simulation(parameters, 0, 0, ".") {
	
}

Simulation::Simulation(ParameterStorage* parameters, Timestamp startTimestamp, Timestamp duration, const std::string& directory)
	: IMessageable(this, "SIMULATION"), m_parameters(parameters), m_startTimestamp(startTimestamp), m_currentTimestamp(startTimestamp), m_durationTimestamp(duration), m_messageQueue(std::make_unique <std::priority_queue<MessagePtr, std::vector<MessagePtr>, CompareArrival>>()), m_state(SimulationState::INACTIVE), m_randomDevice(), m_randomGenerator(std::make_unique<std::mt19937>(m_randomDevice())) {
}

void Simulation::simulate() {
	simulate(m_startTimestamp + m_durationTimestamp - m_currentTimestamp);
}

void Simulation::simulate(Timestamp howMuch) {
	if (m_state == SimulationState::STOPPED) { 
		return;
	}

	if (m_state == SimulationState::INACTIVE) {
		this->start();
	}

	Timestamp toSimulate = std::min(m_startTimestamp + m_durationTimestamp - m_currentTimestamp, howMuch);
	if (toSimulate > 0) {
		step(toSimulate);
	}

	this->stop();
}

void Simulation::deliverMessage(const MessagePtr& messagePtr) {
	for (const std::string& target : messagePtr->targets) {
		if (target == "*") {
			receiveMessage(messagePtr);

			for (const auto& agentPtr : m_agentList) {
				agentPtr->receiveMessage(messagePtr);
			}
		} else if (target == "SIMULATION") {
			receiveMessage(messagePtr);
		} else if (target.back() == '*') {
			std::string prefix = target;
			prefix.pop_back();
			
			auto cmpPredLb = [](const auto& agentPtr, const std::string& val) {
				return agentPtr->name().find(val, 0) != 0;
			};

			auto cmpPredUb = [](const std::string& val, const auto& agentPtr) {
				return agentPtr->name().find(val, 0) == 0;
			};

			auto lb = std::lower_bound(m_agentList.begin(), m_agentList.end(), prefix, cmpPredLb);
			auto ub = std::upper_bound(lb, m_agentList.end(), prefix, cmpPredUb);

			for (; lb != ub; ++lb) {
				(*lb)->receiveMessage(messagePtr);
			}
		} else {
			auto it = std::lower_bound(m_agentList.begin(), m_agentList.end(), target, [](const auto& agentPtr, const std::string& val) {
				return agentPtr->name() < val;
			});

			if (it != m_agentList.end()) { 
				(*it)->receiveMessage(messagePtr);
			} else {
				throw SimulationException("Simulation::deliverMessage(): unknown message target '" + target + "'");
			}
		}
	}
}

void Simulation::receiveMessage(const MessagePtr& msg) {
	// TODO: do something
}

void Simulation::start() {
	this->dispatchMessage(m_startTimestamp, 0, "SIMULATION", "*", "EVENT_SIMULATION_START", nullptr);
	this->dispatchMessage(m_startTimestamp, m_durationTimestamp-1, "SIMULATION", "*", "EVENT_SIMULATION_STOP", nullptr);

	m_state = SimulationState::STARTED;
}

void Simulation::step(Timestamp step) {
	Timestamp cutoff = m_currentTimestamp + step;

	Timestamp topMessageTimestamp;
	while (!m_messageQueue->empty() && (topMessageTimestamp = m_messageQueue->top()->arrival) < cutoff) {
		m_currentTimestamp = topMessageTimestamp;

		MessagePtr topMessage = m_messageQueue->top();
		m_messageQueue->pop(); // ordering intentional
		deliverMessage(topMessage);
	}
}

void Simulation::stop() {
	m_state = SimulationState::STOPPED;
}

void Simulation::setupChildConfiguration(const pugi::xml_node& node, const std::string& configurationPath) {
	for (pugi::xml_node_iterator nit = node.begin(); nit != node.end(); ++nit) {
		std::string nodeName = nit->name();
		if (nodeName == "Generator") {
			pugi::xml_attribute att;
			std::string forwardPath = configurationPath;
			if (!(att = nit->attribute("count")).empty()) {
				ConfigurationIndex maxIndex = (ConfigurationIndex)att.as_uint();
				for (ConfigurationIndex index = 1; index <= maxIndex; ++index) {
					setupChildConfiguration(*nit, forwardPath + std::to_string(index));
				}
			}
		} else if (nodeName == "ExchangeAgent") {
			auto eaptr = std::make_unique<ExchangeAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "TradeLogAgent") {
			auto eaptr = std::make_unique<TradeLogAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "OrderLogAgent") {
			auto eaptr = std::make_unique<OrderLogAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "L1LogAgent") {
			auto eaptr = std::make_unique<L1LogAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "BouchaudAgent") {
			auto eaptr = std::make_unique<BouchaudAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "ImpactAgent") {
			auto eaptr = std::make_unique<ImpactAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "SetupAgent") {
			auto eaptr = std::make_unique<SetupAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "AdaptiveOfferingAgent") {
			auto eaptr = std::make_unique<AdaptiveOfferingAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "RandomWalkMarketMakerAgent") {
			auto eaptr = std::make_unique<RandomWalkMarketMakerAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else if (nodeName == "DoobAgent") {
			auto eaptr = std::make_unique<DoobAgent>(this);
			eaptr->configure(*nit, configurationPath);
			m_agentList.push_back(std::move(eaptr));
		} else {
			pugi::xml_attribute att = node.attribute("file");
			if (!att.empty()) {
				std::string filePath = att.as_string();
				if (!std::filesystem::exists(filePath)) {
					throw SimulationException("Simulation::configure(): unrecognized node '" 
						+ nodeName 
						+ "', tried looking into the file '"
						+ filePath
						+ "', but it does not exist"
					);
				} else {
					auto eaptr = std::make_unique<PythonAgent>(this, nodeName, filePath);
					eaptr->configure(*nit, configurationPath);
					m_agentList.push_back(std::move(eaptr));
				}
			} else {
				std::string filePath = nodeName + ".py";
				if (!std::filesystem::exists(filePath)) {
					throw SimulationException("Simulation::configure(): unrecognized node '"
						+ nodeName
						+ "', tried looking into the file '"
						+ filePath
						+ "', but it does not exist"
					);
				} else {
					auto eaptr = std::make_unique<PythonAgent>(this, nodeName, "");
					eaptr->configure(*nit, configurationPath);
					m_agentList.push_back(std::move(eaptr));
				}
			}
		}
	}

	std::sort(m_agentList.begin(), m_agentList.end(), [](const auto& agentAPtr, const auto& agentBPtr) {
		return agentAPtr->name() < agentBPtr->name();
	});
}

#include <iostream>

void Simulation::configure(const pugi::xml_node& node, const std::string& configurationPath) {
	pugi::xml_attribute att;
	if (!(att = node.attribute("start")).empty()) {
		m_startTimestamp = (Timestamp)att.as_ullong();
	}

	if (!(att = node.attribute("duration")).empty()) {
		m_durationTimestamp = (Timestamp)att.as_ullong();
	}

	setupChildConfiguration(node, configurationPath);
}
