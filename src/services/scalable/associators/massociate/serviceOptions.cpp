#include <string>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::Associators::MAssociate;
namespace UAuth = UMPS::Authentication;

class ServiceOptions::ServiceOptionsImpl
{
public:
    UAuth::ZAPOptions mZAPOptions;
    std::filesystem::path mStaticCorrectionFile;
    std::filesystem::path mSourceSpecificCorrectionFile;
    std::string mAddress;
    std::chrono::milliseconds mPollingTimeOut{10};
    std::pair<double, double> mExtentInDepth{-1700, 22000};
    double mDBSCANEpsilon{0.25};
    double mMaximumDistance{150000};
    int mDBSCANMinimumClusterSize{7};
    int mSendHighWaterMark{8192};
    int mReceiveHighWaterMark{4096};
    int mParticles{60};
    int mEpochs{20};
    ServiceOptions::Region mRegion{ServiceOptions::Region::Utah};
    bool mHaveRegion{false};
};

/// Constructor
ServiceOptions::ServiceOptions() :
    pImpl(std::make_unique<ServiceOptionsImpl> ()) 
{
}

/// Copy constructor
ServiceOptions::ServiceOptions(const ServiceOptions &options)
{
    *this = options;
}

/// Move constructor
ServiceOptions::ServiceOptions(ServiceOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
ServiceOptions& ServiceOptions::operator=(const ServiceOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ServiceOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
ServiceOptions& ServiceOptions::operator=(ServiceOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Reset class
void ServiceOptions::clear() noexcept
{
    pImpl = std::make_unique<ServiceOptionsImpl> ();
}

/// Destructor
ServiceOptions::~ServiceOptions() = default;

/// Sets the backend address
void ServiceOptions::setAddress(const std::string &address)
{
    if (::isEmpty(address)){throw std::invalid_argument("Address is empty");}
    pImpl->mAddress = address;
}

std::string ServiceOptions::getAddress() const
{
    if (!haveAddress())
    {
        throw std::runtime_error("Replier address not set");
    }
    return pImpl->mAddress;
}

bool ServiceOptions::haveAddress() const noexcept
{
    return !pImpl->mAddress.empty();
}

/// ZAP Options
void ServiceOptions::setZAPOptions(
    const UAuth::ZAPOptions &zapOptions) noexcept
{
    pImpl->mZAPOptions = zapOptions;
}

UAuth::ZAPOptions ServiceOptions::getZAPOptions() const noexcept
{
    return pImpl->mZAPOptions;
}

/// Time out
void ServiceOptions::setPollingTimeOut(
    const std::chrono::milliseconds &timeOut)
{
    if (timeOut.count() < 0)
    {   
        throw std::invalid_argument("Time out cannot be negative");
    }   
    pImpl->mPollingTimeOut = timeOut;
}

std::chrono::milliseconds 
ServiceOptions::getPollingTimeOut() const noexcept
{
    return pImpl->mPollingTimeOut;
}

/// High water mark
void ServiceOptions::setSendHighWaterMark(const int highWaterMark)
{
    pImpl->mSendHighWaterMark = highWaterMark;
}

int ServiceOptions::getSendHighWaterMark() const noexcept
{
    return pImpl->mSendHighWaterMark;
}

void ServiceOptions::setReceiveHighWaterMark(const int highWaterMark)
{
    pImpl->mReceiveHighWaterMark = highWaterMark;
}

int ServiceOptions::getReceiveHighWaterMark() const noexcept
{
    return pImpl->mReceiveHighWaterMark;
}

/// Set the region
void ServiceOptions::setRegion(const Region region) noexcept
{ 
    pImpl->mRegion = region;
}

ServiceOptions::Region ServiceOptions::getRegion() const
{
    if (!haveRegion()){throw std::runtime_error("Region not set");}
    return pImpl->mRegion;
}

bool ServiceOptions::haveRegion() const noexcept
{
    return pImpl->mHaveRegion;
}

/// Source specific 
void ServiceOptions::setSourceSpecificCorrectionFile(
    const std::string &fileName)
{
    if (std::filesystem::exists(fileName))
    {
        pImpl->mSourceSpecificCorrectionFile = fileName;
    }
    else
    {
        throw std::invalid_argument("SSSC " + fileName + " does not exist");
    }
}

std::string ServiceOptions::getSourceSpecificCorrectionFile() const noexcept
{
    return pImpl->mSourceSpecificCorrectionFile;
}

/// Static correction
void ServiceOptions::setStaticCorrectionFile(const std::string &fileName)
{
    if (std::filesystem::exists(fileName))
    {
        pImpl->mStaticCorrectionFile = fileName;
    }
    else
    {
        throw std::invalid_argument("SSSC " + fileName + " does not exist");
    }
}

std::string ServiceOptions::getStaticCorrectionFile() const noexcept
{
    return pImpl->mStaticCorrectionFile;
}

/// DBSCAN epsilon
void ServiceOptions::setDBSCANEpsilon(double epsilon)
{
    if (epsilon <= 0)
    {
        throw std::invalid_argument("DBSCAN epsilon must be positive");
    }
    pImpl->mDBSCANEpsilon = epsilon;
}

double ServiceOptions::getDBSCANEpsilon() const noexcept
{
    return pImpl->mDBSCANEpsilon;
}

void ServiceOptions::setDBSCANMinimumClusterSize(int minimumClusterSize)
{
    if (minimumClusterSize < 4)
    {
        throw std::invalid_argument("Minimum cluster size must be at least 4");
    }
    pImpl->mDBSCANMinimumClusterSize = minimumClusterSize;
}

int ServiceOptions::getDBSCANMinimumClusterSize() const noexcept
{
    return pImpl->mDBSCANMinimumClusterSize;
}

/// Maximum distance to associate
void ServiceOptions::setMaximumDistanceToAssociate(double distance)
{
    if (distance <= 0)
    {
        throw std::invalid_argument("Distance must be positive");
    }
    pImpl->mMaximumDistance = distance;
}

double ServiceOptions::getMaximumDistanceToAssociate() const noexcept
{ 
    return pImpl->mMaximumDistance;
}

/// Number of PSO particles
void ServiceOptions::setNumberOfParticles(int nParticles)
{
    if (nParticles <= 0)
    {
        throw std::invalid_argument("Number of particles must be positive");
    }
    pImpl->mParticles = nParticles;
}

int ServiceOptions::getNumberOfParticles() const noexcept
{
    return pImpl->mParticles;
}

/// Number of epochs
void ServiceOptions::setNumberOfEpochs(int nEpochs)
{
    if (nEpochs <= 0)
    {
        throw std::invalid_argument("Number of epochs must be positive");
    }
    pImpl->mEpochs = nEpochs;
}

int ServiceOptions::getNumberOfEpochs() const noexcept
{
    return pImpl->mEpochs;
}
 
void ServiceOptions::setExtentInDepth(
    const std::pair<double, double> &extent)
{
    if (extent.first >= extent.second)
    {
        throw std::invalid_argument("extent.first in depth >= extent.second");
    }
    if (extent.first < -8600)
    {
        throw std::invalid_argument("extent.first < -8600");
    }
    if (extent.second > 800000)
    {
        throw std::invalid_argument("extent.second > 800,000");
    }
    pImpl->mExtentInDepth = extent;
}

std::pair<double, double> ServiceOptions::getExtentInDepth() const noexcept
{
    return pImpl->mExtentInDepth;
}
