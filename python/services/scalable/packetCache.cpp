#include <urts/services/scalable/packetCache/bulkDataRequest.hpp>
#include <urts/services/scalable/packetCache/bulkDataResponse.hpp>
#include <urts/services/scalable/packetCache/dataRequest.hpp>
#include <urts/services/scalable/packetCache/dataResponse.hpp>
#include <urts/services/scalable/packetCache/sensorRequest.hpp>
#include <urts/services/scalable/packetCache/sensorResponse.hpp>
#include "packetCache.hpp"

namespace UPC = URTS::Services::Scalable::PacketCache;
using namespace URTS::Python::Services::Scalable::PacketCache;

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::Services::Scalable::PacketCache::initialize(pybind11::module &m) 
{
    //---------------------------------Data Packet----------------------------//
    pybind11::module pc = m.def_submodule("PacketCache");
    pc.attr("__doc__") = "A high-speed data query utility.";


}
