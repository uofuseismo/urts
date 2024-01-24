#include <urts/version.hpp>
#include <pybind11/pybind11.h>
#include "broadcasts.hpp"
#include "services.hpp"

PYBIND11_MODULE(urtspy, m)
{
    m.attr("__version__") = URTS_VERSION;
    m.attr("__name__") = "urtspy";
    m.attr("__doc__") = "A Python interface to the University of Utah Seismograph Stations Real-Time Seismology library.";

    URTS::Python::Broadcasts::initialize(m);
}
