#ifndef URTS_PYTHON_MESSAGE_FORMATS_HPP
#define URTS_PYTHON_MESSAGE_FORMATS_HPP
#include <memory>
#include <python/messaging.hpp>
#include <python/logging.hpp>
#include <pybind11/pybind11.h>
namespace URTS::MessageFormats
{
 class DataPacket;
}
namespace URTS::Python::MessageFormats
{
 class DataPacket
 {
 public:
     DataPacket();
     ~DataPacket();
 //private:
     std::unique_ptr<URTS::MessageFormats::DataPacket> pImpl{nullptr};
 };
}
#endif
