#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_CREATE_TRAVEL_TIME_CALCULATOR_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_CREATE_TRAVEL_TIME_CALCULATOR_HPP
#include <string>
#include <filesystem>
#include <uLocator/station.hpp>
#include <uLocator/travelTimeCalculator.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/corrections/sourceSpecific.hpp>
#include <uLocator/corrections/static.hpp>
#include <uLocator/position/knownUtahEvent.hpp>
#include <uLocator/position/knownUtahQuarry.hpp>
#include <uLocator/position/knownYNPEvent.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/connection/connection.hpp"
namespace 
{

[[nodiscard]] ULocator::Station createStation(
    const std::string &network, const std::string &station,
    const double latitude, const double longitude, const double elevation,
    const ULocator::Position::IGeographicRegion &region,
    const int utmZone = 12)
{
    ULocator::Station uStation;
    uStation.setNetwork(network);
    uStation.setName(station);
    ULocator::Position::WGS84 location{latitude, longitude, utmZone};
    uStation.setElevation(elevation);
    uStation.setGeographicPosition(location, *region.clone());
    return uStation;
}

[[nodiscard]] ULocator::Station createUtahStation(
    const std::string &network, const std::string &station,
    const double latitude, const double longitude, const double elevation)
{
    constexpr int utmZone{12};
    ULocator::Position::UtahRegion region;
    return ::createStation(network, station,
                           latitude, longitude, elevation,
                           region, utmZone);
}

[[nodiscard]] ULocator::Station createUtahStation(
    const URTS::Database::AQMS::StationData &station)
{
    return ::createUtahStation(station.getNetwork(),
                               station.getStation(),
                               station.getLatitude(),
                               station.getLongitude(),
                               station.getElevation());
} 

ULocator::Station createYNPStation(
    const std::string &network, const std::string &station,
    const double latitude, const double longitude, const double elevation)
{   
    constexpr int utmZone{12};
    ULocator::Position::YNPRegion region;
    return ::createStation(network, station,
                           latitude, longitude, elevation,
                           region, utmZone);
}

[[nodiscard]] ULocator::Station createYNPStation(
    const URTS::Database::AQMS::StationData &station)
{
    return ::createYNPStation(station.getNetwork(),
                              station.getStation(),
                              station.getLatitude(),
                              station.getLongitude(),
                              station.getElevation());
}

[[maybe_unused]] [[nodiscard]] ULocator::Station createStation(
    const std::string &network, const std::string station,
    const bool isUtah,
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection,
    std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    constexpr bool queryCurrent{true};
    URTS::Database::AQMS::StationDataTable stationDataTable{connection, logger};
    stationDataTable.query(network, station, queryCurrent);
    auto stationData = stationDataTable.getStationData();
    if (stationData.empty())
    {
        throw std::runtime_error("No entry in AQMS database for "
                                + network + "." + station); 
    }
    if (stationData.size() > 1 && logger != nullptr)
    {
        logger->warn("Multiple entries found for " 
                   + network + "." + station + ".  Using first one");
    }
    if (isUtah)
    {
        return ::createUtahStation(stationData.at(0));
    }
    else
    {
        return ::createYNPStation(stationData.at(0));
    }
} 

/// @brief Creates a 1D travel time calculator.
/// @param[in] station   The station information.
/// @param[in] phase     The phase - e.g., P or S.
/// @param[in] isUtah    True indicates to make a Utah ray tracer.  Otherwise,
///                      a Yellowstone ray tracer will be made.
/// @param[in] staticCorrectionsFile  If this is not empty then this is the path
///                                   to the static corrections file. 
///                                   Otherwise, statics will not be used.
/// @param[in] sourceSpecificCorrectionsFile  If this is not empty then this is
///                                           the path to the SSSC file.  
///                                           Otherwise, SSSCs will not used.
/// @param[in] logger  An UMPS logger.
/// @result A travel time calculator that can be added to a travel time
///         calculator map.
[[nodiscard]] std::unique_ptr<ULocator::ITravelTimeCalculator>
   createTravelTimeCalculator(
      const ULocator::Station &station,
      const std::string &phase,
      const bool isUtah,
      const std::filesystem::path &staticCorrectionsFile,
      const std::filesystem::path &sourceSpecificCorrectionsFile,
      std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    auto name = station.getNetwork() + "." + station.getName();
    ULocator::Corrections::Static staticCorrection;
    ULocator::Corrections::SourceSpecific sssc;
    auto uPhase = ULocator::UUSSRayTracer::Phase::P;
    if (phase == "S")
    {
        uPhase = ULocator::UUSSRayTracer::Phase::S;
    }
    else
    {
        if (phase != "P"){throw std::invalid_argument("Unhandled phase");}
    }
    auto uRegion = ULocator::UUSSRayTracer::Region::Utah;
    if (!isUtah){uRegion = ULocator::UUSSRayTracer::Region::YNP;}
    if (std::filesystem::exists(staticCorrectionsFile))
    {
        staticCorrection.setStationNameAndPhase(station.getNetwork(),
                                                station.getName(),
                                                phase);
        try
        {
            staticCorrection.load(staticCorrectionsFile);
            if (logger)
            {
               logger->info("Loaded static correction for "
                                + name + "." + phase);
            }
        }
        catch (const std::exception &e)
        {
            if (logger)
            {
                logger->warn("Did not load static correction for "
                           + name + "." + phase + " because "
                           + std::string {e.what()});
                staticCorrection.clear();
            }
        }
    }
    if (std::filesystem::exists(sourceSpecificCorrectionsFile))
    {
        sssc.setStationNameAndPhase(station.getNetwork(),
                                    station.getName(),
                                    phase);
        try
        {
            sssc.load(sourceSpecificCorrectionsFile);
            if (logger)
            {
                logger->info("Loaded source specific correction for "
                           + name + "." + phase);
            }
        }
        catch (const std::exception &e) 
        {
            if (logger)
            {
                logger->warn("Did not load source specific correction for " 
                           + name + "." + phase + " because "
                           + std::string {e.what()});
                sssc.clear();
            }
        }
    }
    std::unique_ptr<ULocator::ITravelTimeCalculator> rayTracer{nullptr};
    try
    {
        rayTracer
            = std::make_unique<ULocator::UUSSRayTracer>
                  (station, uPhase, uRegion,
                   std::move(staticCorrection), std::move(sssc),
                   logger);
        if (logger)
        {
            logger->debug("Created travel time calculator for "
                        + name + "." + phase);
        }
    }
    catch (const std::exception &e) 
    {
        if (logger != nullptr)
        {
            logger->warn("Could not create travel time calculator for "
                       + name + "." + phase + " because "
                       + std::string {e.what()});
        }
        rayTracer = nullptr;
    }
    return rayTracer;
}

/// Creates the Utah search locations
class LocalUtahQuarry : public ULocator::Position::IKnownLocalLocation
{
public:
    explicit LocalUtahQuarry(const ULocator::Position::UtahQuarry &quarry)
    {
        auto [x, y] = quarry.getLocalCoordinates();
        mX = x;
        mY = y;
        mZ =-quarry.getElevation();
    }
    LocalUtahQuarry() = delete;
    LocalUtahQuarry(const LocalUtahQuarry &quarry){*this = quarry;}
    ~LocalUtahQuarry() = default;
    double x() const override{return mX;}
    double y() const override{return mY;}
    double z() const override{return mZ;}
    LocalUtahQuarry& operator=(LocalUtahQuarry &&quarry) noexcept
    {
        mX = quarry.mX;
        mY = quarry.mY;
        mZ = quarry.mZ;
        return *this;
    }
    LocalUtahQuarry& operator=(const LocalUtahQuarry &quarry)
    {
        mX = quarry.mX;
        mY = quarry.mY;
        mZ = quarry.mZ;
        return *this;
    }
    std::unique_ptr<ULocator::Position::IKnownLocalLocation> clone() const override
    {
        std::unique_ptr<ULocator::Position::IKnownLocalLocation> result
            = std::make_unique<LocalUtahQuarry> (*this);
        return result;
    }
    double mX{0};
    double mY{0};
    double mZ{0};
};

[[maybe_unused]]
std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
    createKnownUtahSearchLocations()
{
    std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
        searchLocations;
    auto knownEvents = ULocator::Position::getKnownUtahEvents();
    for (const auto &event : knownEvents)
    {
        searchLocations.push_back(event.clone());
    }
    auto knownQuarries = ULocator::Position::getUtahQuarries();
    for (const auto &quarry : knownQuarries)
    {
        LocalUtahQuarry localQuarry{quarry};
        searchLocations.push_back(localQuarry.clone());
    }
    return searchLocations;
}

[[maybe_unused]]
std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
    createKnownYNPSearchLocations()
{
    std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
        searchLocations;
    auto knownEvents = ULocator::Position::getKnownYNPEvents();
    for (const auto &event : knownEvents)
    {   
        searchLocations.push_back(event.clone());
    }
    return searchLocations;
}

}
#endif
