from thesimulator import *

class PrintingAgent:
    def configure(self, params):
        print(" --- Configuring with the following parameters --- ")
        print(params)
        print(" ------------------------------------------------- ")
    
    def receiveMessage(self, simulation, type, payload):
        currentTimestamp = simulation.currentTimestamp()
        print("Received a message of type '%s' at time %d, payload %s " % (type, currentTimestamp, payload))
    