#include <iostream>
#include <thread>

#include "Simulation.h"
#include "SimulationException.h"
#include "ParameterStorage.h"

#include "pugi/pugixml.hpp"
#include "dimcli/cli.h"

#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/stl.h> // MUST BE
namespace py = pybind11;

static bool silent = false;
void trace(const std::string& msg);
void traceLine(const std::string& msg);
void etrace(const std::string& msg);
void etraceLine(const std::string& msg);

void invokeInteractiveMode(Simulation* simulation);
void runSimulations(std::pair<unsigned int, unsigned int> runIndexRange, bool interactive, pugi::xml_node configurationNode, const ParameterStorage& parameterBase);

int main(int argc, char* argv[]) {
	// start the interpreter and keep it alive
	py::scoped_interpreter guard {};

	// handle the command line argument parsing
	Dim::Cli cli;
	auto& simulationFile = cli.opt<std::string>("f file [file]", "Simulation.xml").desc("the simulation file to be used");
	auto& interactive = cli.opt<bool>("i interactive", false).desc("runs the simulation in the interactive mode");
	auto& silencio = cli.opt<bool>("s silent", false).desc("supresses all verbose trace output, error traces remain enabled");
	auto& runCount = cli.opt<unsigned int>("r runs", 1).desc("Number of times the simulation is to be run");
	auto& threadCount = cli.opt<unsigned int>("t threads", 1).desc("The maximum number of threads to use for evaluating different runs");
	auto& simParameters = cli.optVec<std::string>("[params]").desc("Parameters to be passed to the simulation configuration & the simulation itself");
	if (!cli.parse(std::cerr, argc, argv)) {
		return cli.exitCode();
	}
	silent = *silencio;

	if (*threadCount == 0) {
		etraceLine("Error: can not run the simulation on 0 threads, specify 1 to use the main thread");
		return 1;
	}

	if (*threadCount > 1 && *interactive) {
		etraceLine("Error: can not do multithreading in the interactive mode");
		return 1;
	}

	ParameterStorage parameterBase;
	for (const std::string& simParamPair : *simParameters) {
		auto pos = simParamPair.find('=');
		if (pos == std::string::npos) {
			etraceLine("Error: invalid param pair '" + simParamPair + "', expected 'name=value'");
			return 1;
		}

		const std::string name(simParamPair, 0, pos);
		const std::string value(simParamPair, pos+1, std::string::npos);
		if (name.empty()) {
			etraceLine("Error: invalid param pair '" + simParamPair + "', param name can not be empty");
			return 1;
		}

		parameterBase.set(name, value);
	}

	// say hello world, if not in silent mode
	traceLine("ExchangeSimulator v2.0");

	// parse the simulation configuration file
	pugi::xml_document doc;
	auto parse_result = doc.load_file(simulationFile->c_str());
	if (!parse_result) {
		etraceLine(" - error: could not parse the file '" + (*simulationFile) + "'");
		return 1;
	}
	traceLine(" - '" + *simulationFile + "' loaded successfully");

	// find the root node
	auto node = doc.child("Simulation");
	if (node.empty()) {
		etraceLine(" - error: when parsing '" + *simulationFile + "' - the 'Simulation' element was not found");
		return 1;
	}

	// distribute the loads among all threads
	std::vector<std::pair<unsigned int, unsigned int>> loads;
	loads.reserve(*threadCount);
	unsigned int unitShare = *runCount / *threadCount;
	unsigned int remainder = *runCount % *threadCount;
	unsigned int runIndex = 0;
	for (unsigned int threadIndex = 0; threadIndex < *threadCount; ++threadIndex) {
		const auto endRunIndex = runIndex + unitShare + (threadIndex < remainder ? 1 : 0);
		loads.push_back(std::make_pair(runIndex, endRunIndex));
		runIndex = endRunIndex;
	}
	
	try {
		// catch any SimulationException that may occur
		try {
			traceLine(" - starting the simulations");
			if (*interactive) {
				traceLine(" - entering the interactive mode, type 'help' to retrieve the list of available commands");
			}

			if (loads.size() == 1) {
				runSimulations(loads.front(), *interactive, node, parameterBase);
			} else {
				std::vector<std::unique_ptr<std::thread>> threads;
				for (const auto& load : loads) {
					threads.push_back(std::make_unique<std::thread>(runSimulations, load, false, node, parameterBase));
				}

				for (const auto& threadptr : threads) {
					if (threadptr->joinable()) {
						threadptr->join();
					}
				}
			}
		
			traceLine(" - all simulations finished, exiting");
		} catch (const SimulationException& ex) {
			std::cout << ex.what() << std::endl;
		}
	} catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}

	return 0;
}

#include <sstream>

void invokeInteractiveMode(Simulation* simulation) { 
	while (true) {
		trace("r" + simulation->parameters()["runId"] + ":" + std::to_string(simulation->currentTimestamp()) + "> ");
		std::string commandLine;
		std::getline(std::cin, commandLine);

		std::stringstream ss(commandLine);
		std::string command;
		std::getline(ss, command, ' ');
		if (command == "help") { 
			traceLine("\thelp\t\t\tdisplays this information");
			traceLine("\tstop, exit\t\tstops the simulation and exits the program");
			traceLine("\trun \t\t\tcontinues the simulation until it finishes");
			traceLine("\tstep <step>\t\tsimulates over <step> time units");
		} else if (command == "stop" || command == "exit") {
			traceLine(" - simulation stopped, exiting");
			break;
		} else if (command == "run") { 
			simulation->simulate();
		} else if (command == "step") {
			Timestamp step = 0;
			ss >> step;
			simulation->simulate(step);
		}
	}
}

void runSimulations(std::pair<unsigned int, unsigned int> runIndexRange, bool interactive, pugi::xml_node configurationNode, const ParameterStorage& parameterBase) {
	for(unsigned int runIndex = runIndexRange.first;runIndex < runIndexRange.second;++runIndex) {
		ParameterStorage* parameters = new ParameterStorage(parameterBase);
		parameters->set("runIndex", std::to_string(runIndex));
		Simulation* simulation = new Simulation(parameters);
		simulation->configure(configurationNode, "");

		if (interactive) {
			invokeInteractiveMode(simulation);
		} else {
			simulation->simulate();
		}

		delete simulation;
		delete parameters;
	}
}

void trace(const std::string& msg) {
	if (silent) {
		return;
	}
	std::cout << msg;
}
void traceLine(const std::string& msg) {
	if (silent) {
		return;
	}
	std::cout << msg << std::endl;
}
void etrace(const std::string& msg) {
	std::cerr << msg;
}
void etraceLine(const std::string& msg) {
	std::cerr << msg << std::endl;
}