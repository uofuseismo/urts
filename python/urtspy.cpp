#include <urts/version.hpp>
#include <pybind11/pybind11.h>
#include "proxyBroadcasts.hpp"

PYBIND11_MODULE(urtspy, m)
{
    m.attr("__version__") = URTS_VERSION;
    m.attr("__name__") = "umpspy";
    m.attr("__doc__") = "A Python interface to the Univeristy of Utah Seismograph Stations Real-Time Seismology library.";

    URTS::Python::ProxyBroadcasts::initialize(m);
}
