#include <pybind11/pybind11.h>
#include "broadcasts.hpp"
#include "broadcasts/dataPacket.hpp"
#include "broadcasts/pick.hpp"

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::Broadcasts::initialize(pybind11::module &m) 
{
    pybind11::module broadcasts = m.def_submodule("Broadcasts");
    broadcasts.attr("__doc__") = "Broadcasts in URTS.";

    pybind11::module internalBroadcasts = broadcasts.def_submodule("Internal");
    internalBroadcasts.attr("__doc__") = "Internal broadcasts in URTS";
    
    URTS::Python::Broadcasts::Internal::DataPacket::initialize(internalBroadcasts);
    URTS::Python::Broadcasts::Internal::Pick::initialize(internalBroadcasts);
}

