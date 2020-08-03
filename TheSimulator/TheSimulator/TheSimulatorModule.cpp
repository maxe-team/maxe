#include "Simulation.h"
#include "ExchangeAgentMessagePayloads.h"

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(thesimulator, m) {
	py::class_<Simulation>(m, "Simulation")
		.def("currentTimestamp", &Simulation::currentTimestamp)
		.def("dispatchGenericMessage", &Simulation::dispatchGenericMessage)
		.def("dispatchMessage", &Simulation::dispatchMessage)
		.def("queueMessage", &Simulation::queueMessage)
		;

	py::class_<Money>(m, "Money")
		.def(py::init<signed long long int>())
		.def(py::init<double>())

		.def("roundToCents", &Money::roundToCents)
		.def("floorToCents", &Money::floorToCents)
		.def("ceilToCents", &Money::ceilToCents)

		.def("setCents", &Money::setCents)
		.def("cents", &Money::cents)
		.def("roundedCents", &Money::roundedCents)
		.def("ceiledCents", &Money::ceiledCents)

		.def("toCentString", &Money::toCentString)
		.def("__repr__", &Money::toCentString)
		.def("toPostfixedString", &Money::toPostfixedString)

		.def(py::self + py::self)
		.def(py::self + int())
		//.def(py::self + signed long long int(0))
		.def(py::self + double())

		.def(py::self += py::self)
		.def(py::self += int())
		//.def(py::self += (signed long long int(0)))
		.def(py::self += double())

		.def(py::self - py::self)
		.def(py::self - int())
		//.def(py::self - (signed long long int(0)))
		.def(py::self - double())

		.def(py::self -= py::self)
		.def(py::self -= int())
		//.def(py::self -= (signed long long int(0)))
		.def(py::self -= double())

		.def(py::self * py::self)
		.def(py::self * int())
		//.def(py::self * (signed long long int(0)))
		.def(py::self * double())

		.def(py::self *= py::self)
		.def(py::self *= int())
		//.def(py::self *= (signed long long int(0)))
		.def(py::self *= double())

		.def(py::self / py::self)
		.def(py::self / int())
		//.def(py::self / (signed long long int(0)))
		.def(py::self / double())

		.def(py::self /= py::self)
		.def(py::self /= int())
		//.def(py::self /= (signed long long int(0)))
		.def(py::self /= double())

		.def(py::self > py::self)
		.def(py::self < py::self)
		.def(py::self >= py::self)
		.def(py::self == py::self)
		.def(py::self != py::self)

		.def(-py::self)
		;

	py::enum_<OrderDirection>(m, "OrderDirection")
		.value("Buy", OrderDirection::Buy)
		.value("Sell", OrderDirection::Sell)
		;

	py::class_<MessagePayload, std::shared_ptr<MessagePayload>>(m, "MessagePayload")
		;

	py::class_<EmptyPayload, MessagePayload, std::shared_ptr<EmptyPayload>>(m, "EmptyPayload")
		.def(py::init<>())
		;

	py::class_<PlaceOrderMarketPayload, MessagePayload, std::shared_ptr<PlaceOrderMarketPayload>>(m, "PlaceOrderMarketPayload")
		.def(py::init<OrderDirection, Volume>())
		.def_readwrite("direction", &PlaceOrderMarketPayload::direction)
		.def_readwrite("volume", &PlaceOrderMarketPayload::volume)
		;

	py::class_<PlaceOrderMarketResponsePayload, MessagePayload, std::shared_ptr<PlaceOrderMarketResponsePayload>>(m, "PlaceOrderMarketResponsePayload")
		.def(py::init<OrderID, const std::shared_ptr<PlaceOrderMarketPayload>&>())
		.def_readwrite("id", &PlaceOrderMarketResponsePayload::id)
		.def_readwrite("requestPayload", &PlaceOrderMarketResponsePayload::requestPayload)
		;
	
	py::class_<PlaceOrderLimitPayload, MessagePayload, std::shared_ptr<PlaceOrderLimitPayload>>(m, "PlaceOrderLimitPayload")
		.def(py::init<OrderDirection, Volume, Money>())
		.def_readwrite("direction", &PlaceOrderLimitPayload::direction)
		.def_readwrite("volume", &PlaceOrderLimitPayload::volume)
		.def_readwrite("price", &PlaceOrderLimitPayload::price)
		;

	py::class_<PlaceOrderLimitResponsePayload, MessagePayload, std::shared_ptr<PlaceOrderLimitResponsePayload>>(m, "PlaceOrderLimitResponsePayload")
		.def(py::init<OrderID, const std::shared_ptr<PlaceOrderLimitPayload>&>())
		.def_readwrite("id", &PlaceOrderLimitResponsePayload::id)
		.def_readwrite("requestPayload", &PlaceOrderLimitResponsePayload::requestPayload)
		;

	py::class_<RetrieveOrdersPayload, MessagePayload, std::shared_ptr<RetrieveOrdersPayload>>(m, "RetrieveOrdersPayload")
		.def(py::init<const std::vector<OrderID>&>())
		.def_readwrite("ids", &RetrieveOrdersPayload::ids)
		;

	py::class_<RetrieveOrdersResponsePayload, MessagePayload, std::shared_ptr<RetrieveOrdersResponsePayload>>(m, "RetrieveOrdersResponsePayload")
		.def(py::init<>())
		.def_readwrite("orders", &RetrieveOrdersResponsePayload::orders)
		;

	py::class_<CancelOrdersCancellation>(m, "CancelOrdersCancellation")
		.def(py::init<OrderID, Volume>())
		.def_readwrite("id", &CancelOrdersCancellation::id)
		.def_readwrite("volume", &CancelOrdersCancellation::volume)
		;

	py::class_<CancelOrdersPayload, MessagePayload, std::shared_ptr<CancelOrdersPayload>>(m, "CancelOrdersPayload")
		.def(py::init<const std::vector<CancelOrdersCancellation>&>())
		.def_readwrite("cancellations", &CancelOrdersPayload::cancellations)
		;

	py::class_<RetrieveBookPayload, MessagePayload, std::shared_ptr<RetrieveBookPayload>>(m, "RetrieveBookPayload")
		.def(py::init<unsigned int>())
		.def_readwrite("depth", &RetrieveBookPayload::depth)
		;

	py::class_<RetrieveBookResponsePayload, MessagePayload, std::shared_ptr<RetrieveBookResponsePayload>>(m, "RetrieveBookResponsePayload")
		.def(py::init<Timestamp, const std::vector<TickContainer>&>())
		.def_readwrite("time", &RetrieveBookResponsePayload::time)
		.def_readwrite("tickContainers", &RetrieveBookResponsePayload::tickContainers)
		;

	py::class_<RetrieveL1Payload, MessagePayload, std::shared_ptr<RetrieveL1Payload>>(m, "RetrieveL1Payload")
		.def(py::init<>())
		;

	py::class_<RetrieveL1ResponsePayload, MessagePayload, std::shared_ptr<RetrieveL1ResponsePayload>>(m, "RetrieveL1ResponsePayload")
		.def(py::init<Timestamp, Money, Volume, Volume, Money, Volume, Volume>())
		.def_readwrite("time", &RetrieveL1ResponsePayload::time)
		.def_readwrite("bestAskPrice", &RetrieveL1ResponsePayload::bestAskPrice)
		.def_readwrite("bestAskVolume", &RetrieveL1ResponsePayload::bestAskVolume)
		.def_readwrite("askTotalVolume", &RetrieveL1ResponsePayload::askTotalVolume)
		.def_readwrite("bestBidPrice", &RetrieveL1ResponsePayload::bestBidPrice)
		.def_readwrite("bestBidVolume", &RetrieveL1ResponsePayload::bestBidVolume)
		.def_readwrite("bidTotalVolume", &RetrieveL1ResponsePayload::bidTotalVolume)
		;

	py::class_<SubscribeEventTradeByOrderPayload, MessagePayload, std::shared_ptr<SubscribeEventTradeByOrderPayload>>(m, "SubscribeEventTradeByOrderPayload")
		.def(py::init<OrderID>())
		.def_readwrite("id", &SubscribeEventTradeByOrderPayload::id)
		;

	py::class_<EventOrderMarketPayload, MessagePayload, std::shared_ptr<EventOrderMarketPayload>>(m, "EventOrderMarketPayload")
		.def(py::init<MarketOrder>())
		.def_readonly("order", &EventOrderMarketPayload::order)
		;

	py::class_<EventOrderLimitPayload, MessagePayload, std::shared_ptr<EventOrderLimitPayload>>(m, "EventOrderLimitPayload")
		.def(py::init<LimitOrder>())
		.def_readonly("order", &EventOrderLimitPayload::order)
		;

	py::class_<EventTradePayload, MessagePayload, std::shared_ptr<EventTradePayload>>(m, "EventTradePayload")
		.def(py::init<Trade>())
		.def_readonly("trade", &EventTradePayload::trade)
		;
}