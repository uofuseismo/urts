#include <string>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
#include "private/isEmpty.hpp"

#define UTAH_DEFAULT_TIME_WINDOW 140
#define YNP_DEFAULT_TIME_WINDOW 60
#define UTAH_DEFAULT_SEARCH_DEPTH 4780
#define YNP_DEFAULT_SEARCH_DEPTH 6600
#define UTAH_MAX_SEARCH_DEPTH 65000
#define YNP_MAX_SEARCH_DEPTH 30000
#define UTAH_HORIZONTAL_REFINEMENT 50000
#define YNP_HORIZONTAL_REFINEMENT 35000

using namespace URTS::Services::Scalable::Locators::ULocator;
namespace UAuth = UMPS::Authentication;

class ServiceOptions::ServiceOptionsImpl
{
public:
    UAuth::ZAPOptions mZAPOptions;
    std::filesystem::path mStaticCorrectionFile;
    std::filesystem::path mSourceSpecificCorrectionFile;
    std::filesystem::path mTopographyFile;
    std::string mAddress;

    double mDefaultDepth{4780}; // Utah and 6600 for YNP
    double mPNorm{1.5};

    std::chrono::milliseconds mPollingTimeOut{10};
    
    double mDIRECTOriginTimeTolerance{1};
    double mMaximumSearchDepth{UTAH_MAX_SEARCH_DEPTH};
    double mDIRECTLocationTolerance{1000}; // Meters
    double mHorizontalRefinement{UTAH_HORIZONTAL_REFINEMENT};
    double mOriginTimeSearchWindow{UTAH_DEFAULT_TIME_WINDOW};
    int mDIRECTFunctionEvaluations{1200};
    int mPSOParticles{20}; // 15 for YNP
    int mPSOGenerations{140}; // 120 for YNP
    int mSendHighWaterMark{8192};
    int mReceiveHighWaterMark{4096};
    ServiceOptions::Region mRegion{ServiceOptions::Region::Utah};
    ServiceOptions::Norm mNorm{ServiceOptions::Norm::Lp};
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
    pImpl->mHaveRegion = true;
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
    const std::filesystem::path &fileName)
{
    if (std::filesystem::exists(fileName))
    {
        pImpl->mSourceSpecificCorrectionFile = fileName;
    }
    else
    {
        throw std::invalid_argument("SSSC " + fileName.string() + " does not exist");
    }
}

std::filesystem::path ServiceOptions::getSourceSpecificCorrectionFile() const noexcept
{
    return pImpl->mSourceSpecificCorrectionFile;
}

/// Static correction
void ServiceOptions::setStaticCorrectionFile(const std::filesystem::path &fileName)
{
    if (std::filesystem::exists(fileName))
    {
        pImpl->mStaticCorrectionFile = fileName;
    }
    else
    {
        throw std::invalid_argument("Static corrections "
                                  + fileName.string() + " does not exist");
    }
}

std::filesystem::path ServiceOptions::getStaticCorrectionFile() const noexcept
{
    return pImpl->mStaticCorrectionFile;
}

/// Topography file
void ServiceOptions::setTopographyFile(const std::filesystem::path &fileName)
{
    if (std::filesystem::exists(fileName))
    {
        pImpl->mTopographyFile = fileName;
    }
    else
    {
        throw std::invalid_argument("Topography file " 
                                  + fileName.string() + " does not exist");
    }
}

std::filesystem::path ServiceOptions::getTopographyFile() const noexcept
{
    return pImpl->mTopographyFile;
}

/// The p norm
void ServiceOptions::setNorm(const Norm norm, const double p)
{
    if (std::abs(p - 2) < 1.e-10 || norm == Norm::L2)
    {
        pImpl->mNorm = Norm::L2;
        pImpl->mPNorm = 2;
    }
    else if (std::abs(p - 1) < 1.e-10 || norm == Norm::L1)
    {
        pImpl->mNorm = Norm::L1;
        pImpl->mPNorm = 1;
    }
    else
    {
        if (p < 1)
        {
            throw std::invalid_argument("p must be >= 1");
        }
        pImpl->mNorm = Norm::Lp;
        pImpl->mPNorm = p;
    }
}

std::pair<ServiceOptions::Norm, double> ServiceOptions::getNorm() const noexcept
{
    return std::pair {pImpl->mNorm, pImpl->mPNorm};
}


/// Number of PSO particles
void ServiceOptions::setNumberOfParticles(int nParticles)
{
    if (nParticles <= 0)
    {
        throw std::invalid_argument("Number of particles must be positive");
    }
    pImpl->mPSOParticles = nParticles;
}

int ServiceOptions::getNumberOfParticles() const noexcept
{
    return pImpl->mPSOParticles;
}

/// Number of generations
void ServiceOptions::setNumberOfGenerations(int nGenerations)
{
    if (nGenerations <= 0)
    {
        throw std::invalid_argument("Number of PSO generations must be positive");
    }
    pImpl->mPSOGenerations = nGenerations;
}

int ServiceOptions::getNumberOfGenerations() const noexcept
{
    return pImpl->mPSOGenerations;
}
 
void ServiceOptions::setMaximumSearchDepth(double depth)
{
    if (depth <= -8600)
    {
        throw std::invalid_argument("depth <= -8600");
    }
    if (depth >= 800000)
    {
        throw std::invalid_argument("depth >= 800,000");
    }
    pImpl->mMaximumSearchDepth = depth;
}

double ServiceOptions::getMaximumSearchDepth() const noexcept
{
    return pImpl->mMaximumSearchDepth;
}

/// Search window
void ServiceOptions::setOriginTimeSearchWindow(double window)
{
    if (window <= 0)
    {
        throw std::invalid_argument("Search window duration must be positive");
    }
    pImpl->mOriginTimeSearchWindow = window;
}

double ServiceOptions::getOriginTimeSearchWindow() const noexcept
{
    return pImpl->mOriginTimeSearchWindow;
}

/// Sets the horizontal refinement
void ServiceOptions::setHorizontalRefinement(double extent)
{
    if (extent <= 0)
    {
        throw std::invalid_argument("The refinement distance must be positive");
    }
    pImpl->mHorizontalRefinement = extent;
}

double ServiceOptions::getHorizontalRefinement() const noexcept
{
    return pImpl->mHorizontalRefinement;
}

/// Number of DIRECT function evaluations
void ServiceOptions::setNumberOfFunctionEvaluations(int nEval)
{
    if (nEval < 5)
    {
        throw std::invalid_argument("At least 5 function evaluations required");
    }
    pImpl->mDIRECTFunctionEvaluations = nEval;
}

int ServiceOptions::getNumberOfFunctionEvaluations() const noexcept
{
    return pImpl->mDIRECTFunctionEvaluations;
}
 
/// Parses the initialization file
void ServiceOptions::parseInitializationFile(
    const std::string &fileName,
    const std::string &section)
{
    if (!std::filesystem::exists(fileName))
    {
        throw std::invalid_argument("Initialization file "
                                  + fileName
                                  + " does not exist");
    }
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(fileName, propertyTree);
    // Load the essentials
    ServiceOptions options;
    auto region = propertyTree.get<std::string> (section + ".region");
    if (region == "utah" || region == "Utah")
    {
        options.setRegion(ServiceOptions::Region::Utah);
        options.setMaximumSearchDepth(UTAH_MAX_SEARCH_DEPTH);
        options.setOriginTimeSearchWindow(UTAH_DEFAULT_TIME_WINDOW);
        options.setHorizontalRefinement(UTAH_HORIZONTAL_REFINEMENT);
    }
    else if (region == "YNP" || region == "ynp" || region == "Yellowstone")
    {
        options.setRegion(ServiceOptions::Region::YNP);
        options.setMaximumSearchDepth(YNP_MAX_SEARCH_DEPTH);
        options.setOriginTimeSearchWindow(YNP_DEFAULT_TIME_WINDOW);
        options.setHorizontalRefinement(YNP_HORIZONTAL_REFINEMENT);
    }
    else
    {
        throw std::invalid_argument("Unhandled region: " + region);
    }
    // Load the variables with defaults
    // Location properties
    auto maximumSearchDepth
        = propertyTree.get<double> (section + ".maximumSearchDepth",
                                    options.getMaximumSearchDepth());
    options.setMaximumSearchDepth(maximumSearchDepth);
    auto timeWindow
        = propertyTree.get<double> (section + ".originTimeSearchWindow",
                                    options.getOriginTimeSearchWindow());
    options.setOriginTimeSearchWindow(timeWindow);
    auto horizontalRefinement
        = propertyTree.get<double> (section + ".horizontalRefinement",
                                    options.getHorizontalRefinement());
    options.setHorizontalRefinement(horizontalRefinement);
    auto nDIRECTFunctionEvaluations
       = propertyTree.get<int> (section + ".numberOfFunctionEvaluations",
                                options.getNumberOfFunctionEvaluations());
    options.setNumberOfFunctionEvaluations(nDIRECTFunctionEvaluations);
    // PSO: number of epochs
    auto nPSOGenerations
       = propertyTree.get<int> (section + ".numberOfGenerations",
                                options.getNumberOfGenerations());
    options.setNumberOfGenerations(nPSOGenerations);
    auto nPSOParticles
       = propertyTree.get<int> (section + ".numberOfParticles",
                                options.getNumberOfGenerations());
    options.setNumberOfParticles(nPSOParticles);
    // Corrections files
    auto staticCorrectionFile
        = propertyTree.get<std::string> (section + ".staticCorrections");
    if (!staticCorrectionFile.empty())
    {
        options.setStaticCorrectionFile(staticCorrectionFile);
    }
    auto sourceSpecificCorrectionFile
        = propertyTree.get<std::string>
          (section + ".sourceSpecificCorrections"); 
    if (!sourceSpecificCorrectionFile.empty())
    {
        options.setSourceSpecificCorrectionFile(sourceSpecificCorrectionFile);
    }
    // Topography file
    auto topographyFile
        = propertyTree.get<std::string>
          (section + ".topography"); 
    if (!topographyFile.empty())
    {   
        options.setTopographyFile(topographyFile);
    }   
/*
    auto password = std::string(std::getenv("URTS_AQMS_RDONLY_PASSWORD"));
    password = propertyTree.get<std::string> (section + ".password", password);
    int port = getPort();
    port = propertyTree.get<int> (section + ".port", getPort());
    auto address = propertyTree.get<std::string> (section + ".address",
                                                  getAddress());
    auto databaseName
        = propertyTree.get<std::string> (section + ".databaseName");
    auto applicationName
        = propertyTree.get<std::string> (section + ".application",
                                          getApplication()); 
*/
    // I guess this worked
    *this = options;
}

