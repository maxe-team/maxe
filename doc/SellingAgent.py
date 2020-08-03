from thesimulator import *

class SellingAgent:
    def configure(self, params):
        # save locally the configuration params passed so that they are properly typed
        self.exchange = str(params['exchange'])
        self.price = Money(float(params['price']))
        self.quantity = int(params['quantity'])
        self.interval = int(params['interval'])
    
    def receiveMessage(self, simulation, type, payload):
        # Firstly, ignore all messages that should not trigger selling (i.e. order placement confirmations, etc.)
        if type != "EVENT_SIMULATION_START" and type != "WAKE_UP":
            return
        
        # Announce our intentions in the standard output
        currentTimestamp = simulation.currentTimestamp()
        print("%s:  Selling %d units for %s, then going to sleep until %d" % (self.name(), self.quantity, self.price.toCentString(), currentTimestamp+self.interval))

        # Schedule the (first/next) wakeup message `self.interval` time units later
        simulation.dispatchGenericMessage(currentTimestamp, self.interval, self.name(), self.name(), "WAKE_UP", {})

        # Place a limit order to buy `self.quantity` units of an instrument at the price `self.price`
        limitOrderPayload = PlaceOrderLimitPayload(OrderDirection.Sell, self.quantity, self.price)
        simulation.dispatchMessage(currentTimestamp, 0, self.name(), self.exchange, "PLACE_ORDER_LIMIT", limitOrderPayload)
    