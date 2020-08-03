from thesimulator import *

class BuyingAgent:
    def configure(self, params):
        # save locally the configuration params passed so that they are properly typed
        self.exchange = str(params['exchange'])
        self.quantity = int(params['quantity'])
    
    def receiveMessage(self, simulation, type, payload):
        currentTimestamp = simulation.currentTimestamp()

        if type == "EVENT_SIMULATION_START":
            # Subscribe to receive a message of type "EVENT_ORDER_LIMIT" whenever a limit order is submitted to the exchange `self.exchange`
            simulation.dispatchMessage(currentTimestamp, 0, self.name(), self.exchange, "SUBSCRIBE_EVENT_ORDER_LIMIT", EmptyPayload())
            return
        elif type != "EVENT_ORDER_LIMIT":
            # Ignore all messages that should not trigger buying (i.e. order placement confirmations, etc.
            return
        
        # Announce our intentions in the standard output
        print("%s:   Buying %d units, then going to sleep to wait for the next order to be submitted" % (self.name(), self.quantity))

        # Place a limit order to buy `self.quantity` units of an instrument at the price `self.price`
        marketOrderPayload = PlaceOrderMarketPayload(OrderDirection.Buy, self.quantity)
        simulation.dispatchMessage(currentTimestamp, 0, self.name(), self.exchange, "PLACE_ORDER_MARKET", marketOrderPayload)
    