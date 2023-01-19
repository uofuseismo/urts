#include <pybind11/pybind11.h>
#include "services.hpp"
#include "services/scalable/packetCache.hpp"

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::Services::initialize(pybind11::module &m) 
{
    pybind11::module services = m.def_submodule("Services");
    services.attr("__doc__") = "Services in URTS.";

    pybind11::module scalableServices = services.def_submodule("Scalable");
    scalableServices.attr("__doc__") = "Scalable services in URTS.";
    
//    URTS::Python::Services::Scalable::PacketCache::initialize(scalableServices);
}
