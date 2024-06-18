#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_CREATE_TRAVEL_TIME_CALCULATOR_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_CREATE_TRAVEL_TIME_CALCULATOR_HPP
#include <string>
#include <filesystem>
#include <uLocator/station.hpp>
#include <uLocator/travelTimeCalculator.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/corrections/sourceSpecific.hpp>
#include <uLocator/corrections/static.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include "urts/database/aqms/stationData.hpp"
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

}
#endif
