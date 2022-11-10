#include <pybind11/pybind11.h>
#include "proxyBroadcasts.hpp"
#include "proxyBroadcasts/dataPacket.hpp"

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::ProxyBroadcasts::initialize(pybind11::module &m) 
{
    pybind11::module proxyBroadcasts = m.def_submodule("ProxyBroadcasts");
    proxyBroadcasts.attr("__doc__") = "Proxy broadcasts in URTS.";
    
    DataPacket::initialize(proxyBroadcasts);
}

