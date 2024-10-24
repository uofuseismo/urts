#include <thread>
#include <limits>
#include <cmath>
#include <mutex>
#include <string>
#include <algorithm>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/message.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <massociate/pick.hpp>
#include <massociate/associator.hpp>
#include <massociate/waveformIdentifier.hpp>
#include <massociate/dbscan.hpp>
#include <massociate/event.hpp>
#include <massociate/migrator.hpp>
#include <uLocator/arrival.hpp>
#include <uLocator/origin.hpp>
#include <uLocator/station.hpp>
#include <uLocator/travelTimeCalculator.hpp>
#include <uLocator/travelTimeCalculatorMap.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/optimizers/optimizer.hpp>
#include <uLocator/optimizers/originTime.hpp>
#include <uLocator/optimizers/nlopt/dividedRectangles.hpp>
#include <uLocator/optimizers/pagmo/particleSwarm.hpp>
#include <uLocator/origin.hpp>
#include <uLocator/position/knownUtahEvent.hpp>
#include <uLocator/position/knownUtahQuarry.hpp>
#include <uLocator/position/knownYNPEvent.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/topography/topography.hpp>
#include <uLocator/topography/constant.hpp>
#include <uLocator/topography/gridded.hpp>
#include "urts/services/scalable/locators/uLocator/service.hpp"
#include "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
#include "urts/services/scalable/locators/uLocator/locationRequest.hpp"
#include "urts/services/scalable/locators/uLocator/locationResponse.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"
#include "urts/services/scalable/locators/uLocator/origin.hpp"
//#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/database/connection/connection.hpp"
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/aqms/stationData.hpp"

#include "services/scalable/associators/massociate/createTravelTimeCalculator.hpp"

namespace ULoc = ULocator;
using namespace URTS::Services::Scalable::Locators::ULocator;

namespace
{

struct InitialSolution
{
    ULoc::Origin origin;
    std::string description;
    double value{std::numeric_limits<double>::max()};
};


ULoc::Optimizers::IOptimizer::Norm
     normToNorm(const ServiceOptions::Norm norm)
{
    if (norm == ServiceOptions::Norm::Lp)
    { 
        return ULoc::Optimizers::IOptimizer::Norm::Lp;
    }
    else if (norm == ServiceOptions::Norm::L1)
    {
        return ULoc::Optimizers::IOptimizer::Norm::L1;
    }
    else if (norm == ServiceOptions::Norm::L2)
    {
        return ULoc::Optimizers::IOptimizer::Norm::LeastSquares;
    }
    throw std::runtime_error("Unhandled norm!");
}

template<typename T>
[[nodiscard]] std::pair<ULoc::Origin, double>
    searchKnownLocations(const std::vector<ULoc::Arrival> &arrivalsIn,
                         const ULoc::TravelTimeCalculatorMap &travelTimeCalculatorMap,
                         const std::vector<T> &locations,
                         const ULoc::Position::IGeographicRegion &region,
                         std::shared_ptr<UMPS::Logging::ILog> &logger,
                         const ULoc::Optimizers::IOptimizer::Norm norm,
                         const double p = 1.5,
                         const double timeWindow = 150,
                         const bool isQuarry = false,
                         const bool applyCorrection = true,
                         const int utmZone = 12)
{
    if (arrivalsIn.empty())
    {
        throw std::invalid_argument("No arrivals to search!");
    }
    auto arrivals = arrivalsIn;
    // Initialize the origin time optimizer
    ULoc::Optimizers::OriginTime optimizer;
    optimizer.setNorm(norm, p);
    optimizer.setTimeWindow(timeWindow);
    optimizer.enableTimeReduction();
    // Extract the station/phase pairs
    std::vector<std::pair<ULoc::Station, std::string>> stationPhases;
    stationPhases.reserve(arrivals.size());
    for (int i = 0; i < static_cast<int> (arrivals.size()); ++i)
    {   
        const auto &stationReference = arrivals.at(i).getStationReference();
        stationPhases.push_back(std::pair{stationReference,
                                          arrivals[i].getPhase()}); 
    }
    // Loop through the candidate positions
    ULoc::Origin bestOrigin;
    double bestObjectiveFunction{std::numeric_limits<double>::max()};
    for (const auto &location : locations)
    {
        std::vector<double> travelTimes;
        constexpr double zeroOriginTime{0};
        const auto xSource = location.x();
        const auto ySource = location.y();
        const auto zSource = location.z();
        double objectiveFunction{std::numeric_limits<double>::max()};
        try
        {
            travelTimeCalculatorMap.evaluate(stationPhases,
                                             zeroOriginTime,
                                             xSource, ySource, zSource,
                                             &travelTimes,
                                             applyCorrection);
            // Optimize
            optimizer.setTravelTimes(travelTimes);
            optimizer.optimize();
            objectiveFunction = optimizer.computeObjectiveFunction();
        }
        catch (const std::exception &e)
        {
            logger->warn("Known event location optmization; failed with: "
                       + std::string {e.what()} + " - skipping...");
        }
        // Better than the rest?  Then update...
        if (objectiveFunction < bestObjectiveFunction)
        {
            bestObjectiveFunction = objectiveFunction;
            double originTime = optimizer.getTime();
            // Update the arrivals
            std::transform(travelTimes.begin(), travelTimes.end(),
                           travelTimes.begin(),
                           [&](const double t)
                           {
                               return originTime + t;
                           });
            for (int i = 0; i < static_cast<int> (arrivals.size()); ++i)
            {
                arrivals[i].setResidual(arrivals[i].getTime() - travelTimes[i]);
            }
            const bool fixedDepth{true};
            auto [latitude, longitude]
                = region.localToGeographicCoordinates(xSource, ySource);
            bestOrigin.setEpicenter(ULoc::Position::WGS84 {latitude, longitude, utmZone});
            bestOrigin.setDepth(zSource, fixedDepth); 
            bestOrigin.setTime(originTime);
            bestOrigin.setArrivals(arrivals);
            bestOrigin.setEventType(ULoc::Origin::EventType::Earthquake);
            if (isQuarry)
            {
                bestOrigin.setEventType(ULoc::Origin::EventType::QuarryBlast);
            }
        }
    }
    return std::pair {bestOrigin, bestObjectiveFunction};
}

[[nodiscard]] std::pair<ULoc::Origin, double>
    searchStations(const std::vector<ULoc::Arrival> &arrivalsIn,
                   const ULoc::TravelTimeCalculatorMap &travelTimeCalculatorMap,
                   double defaultDepth,
                   const ULoc::Optimizers::IOptimizer::Norm norm,
                   const double p = 1.5,
                   const double timeWindow = 150,
                   const bool applyCorrection = true)
{
    if (arrivalsIn.empty())
    {   
        throw std::invalid_argument("No arrivals to search!");
    }   
    auto arrivals = arrivalsIn;
    // Initialize the optimizer
    ULoc::Optimizers::OriginTime optimizer;
    optimizer.setNorm(norm, p); 
    optimizer.setTimeWindow(timeWindow);
    optimizer.enableTimeReduction();
    // Find the earliest arrival and we'll use that station's
    // location as the epicenter
    std::vector<std::pair<ULoc::Station, std::string>> stationPhases;
    stationPhases.reserve(arrivals.size());
    double earliestArrivalTime{std::numeric_limits<double>::max()};
    int earliestArrivalIndex{-1};
    for (int i = 0; i < static_cast<int> (arrivals.size()); ++i)
    {
        const auto &stationReference = arrivals.at(i).getStationReference();
        stationPhases.push_back(std::pair{stationReference,
                                          arrivals[i].getPhase()}); 
        if (arrivals[i].getTime() < earliestArrivalTime)
        {
            earliestArrivalTime = arrivals[i].getTime();
            earliestArrivalIndex = i;
        }
    }
    // Set the closest station as the source position
    auto [xSource, ySource]
        = arrivals.at(earliestArrivalIndex).getStationReference()
                                           .getLocalCoordinates();
    auto zSource = defaultDepth;
    constexpr double zeroOriginTime{0};
    // Tabulate travel times
    std::vector<double> travelTimes;
    travelTimeCalculatorMap.evaluate(stationPhases,
                                     zeroOriginTime, xSource, ySource, zSource,
                                     &travelTimes,
                                     applyCorrection);
    // Set them on the solver
    optimizer.setTravelTimes(travelTimes);
    // Optimize for an origin time
    optimizer.optimize();
    auto objectiveFunction = optimizer.computeObjectiveFunction();
    double originTime = optimizer.getTime();
    // Build the event
    ULoc::Origin candidateOrigin;
    candidateOrigin.setEpicenter(
        arrivals.at(earliestArrivalIndex).getStationReference()
                                         .getGeographicPosition());
    candidateOrigin.setDepth(defaultDepth);
    candidateOrigin.setEventType(ULoc::Origin::EventType::Earthquake);
    candidateOrigin.setTime(originTime); 
    // Update the arrivals
    std::transform(travelTimes.begin(), travelTimes.end(), travelTimes.begin(),
                   [&](const double t)
                   {
                       return originTime + t;
                   });
    for (int i = 0; i < static_cast<int> (arrivals.size()); ++i)
    {   
        arrivals[i].setResidual(arrivals[i].getTime() - travelTimes[i]);
    }   
    candidateOrigin.setArrivals(arrivals);
    // Done
    return std::pair {candidateOrigin, objectiveFunction};
}
}

class Service::ServiceImpl
{
public:
    /// @brief Constructor
    ServiceImpl(std::shared_ptr<UMPS::Messaging::Context> responseContext = nullptr,
                const std::shared_ptr<UMPS::Logging::ILog> &logger = nullptr)
    {   
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> (); 
        }
        else
        {
            mLogger = logger;
        }
        mReplier
            = std::make_unique<UMPS::Messaging::RouterDealer::Reply>
              (responseContext, mLogger);
    }
    /// Destructor
    ~ServiceImpl()
    {   
        stop();
    }
    /// @brief Starts the service
    void start()
    {
        stop();
        setRunning(true);
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mLogger->debug("Starting replier service...");
        mReplier->start();
    }   
    /// @brief Stops the threads
    void stop()
    {
        mLogger->debug("uLocator service stopping threads...");
        setRunning(false);
        if (mReplier != nullptr){mReplier->stop();}
    }   
    /// @result True indicates the threads should keep running
    [[nodiscard]] bool keepRunning() const
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mKeepRunning;
    }   
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }   
    /// @brief Does some stock stuff for setting x/y extent on the refined
    ///        optimizer as well as the topography and travel time calculators
    void initializeRefinedOptimizer(const ULoc::Origin &bestInitialOrigin)
    {
        // Now perform the real-deal search with PSO
        auto refineHorizontal = mOptions.getHorizontalRefinement(); 
        auto refineX = refineHorizontal;
        auto refineY = refineHorizontal;
        auto initialEpicenter = bestInitialOrigin.getEpicenter();
        auto initialLatitude = initialEpicenter.getLatitude();
        auto initialLongitude = initialEpicenter.getLongitude();
        auto [initialX, initialY]
            = mRegion->geographicToLocalCoordinates(
                 initialLatitude, initialLongitude);
        auto [x0, x1] = mRegion->getExtentInX();
        auto [y0, y1] = mRegion->getExtentInY();
        std::pair<double, double> newExtentInX {std::max(x0, initialX - refineX),
                                                std::min(x1, initialX + refineX)};
        std::pair<double, double> newExtentInY {std::max(y0, initialY - refineY),
                                                std::min(y1, initialY + refineY)};
        mPSO->setExtentInX(newExtentInX);
        mPSO->setExtentInY(newExtentInY);
        mPSO->setTravelTimeCalculatorMap(std::move(mTravelTimeCalculators));
        mPSO->setTopography(std::move(mTopography));
    }
    /// @brief Performs the location in YNP
    [[nodiscard]] ULoc::Origin doYNP(const std::vector<ULoc::Arrival> &arrivals)
    {
        constexpr bool applyCorrection{true};
        auto [urtsNorm, pNorm] = mOptions.getNorm();
        auto norm = normToNorm(urtsNorm);
        auto timeWindow = mOptions.getOriginTimeSearchWindow(); 
        int utmZone{12};
        if (mOptions.getUTMZone()){utmZone = *mOptions.getUTMZone();}
        // Check the nearest stations
        mLogger->debug("Performing YNP station search...");
        std::vector<InitialSolution> initialSolutions;
        auto defaultSearchDepth = mOptions.getInitialSearchDepth();
        try
        {
            auto result
                = ::searchStations(arrivals,
                                   *mTravelTimeCalculators,
                                   defaultSearchDepth,
                                   norm,
                                   pNorm,
                                   timeWindow,
                                   applyCorrection);
            initialSolutions.push_back({result.first,
                                        "YNP station",
                                        result.second});
        }
        catch (const std::exception &e)
        {
            mLogger->warn("YNP station search failed with: "
                         + std::string{e.what()});
        }
        // Check the default earthquake locations
        mLogger->debug("Performing YNP known location search...");
        try
        {
            constexpr bool isQuarry{false};
            auto result
                = ::searchKnownLocations(arrivals,
                                         *mTravelTimeCalculators,
                                         mTrialYNPEventLocations,
                                         *mRegion,
                                         mLogger,
                                         norm,
                                         pNorm,
                                         timeWindow,
                                         isQuarry,
                                         applyCorrection,
                                         utmZone); 
            initialSolutions.push_back({result.first,
                                        "YNP default event location",
                                         result.second});
        }
        catch (const std::exception &e)
        {
            mLogger->warn("YNP default location search failed with: "
                         + std::string{e.what()});
        }
        // Perform the crude search
        mLogger->debug("Performing initial DIRECT search for YNP...");
        mDIRECT->setArrivals(arrivals);
        mDIRECT->setTopography(std::move(mTopography));
        mDIRECT->setTravelTimeCalculatorMap(std::move(mTravelTimeCalculators));
        try
        {
            mDIRECT->locateEventWithFixedDepth(defaultSearchDepth, norm);
            initialSolutions.push_back({mDIRECT->getOrigin(),
                                        "YNP DIRECT",
                                        mDIRECT->getOptimalObjectiveFunction()});
            mTravelTimeCalculators = mDIRECT->releaseTravelTimeCalculatorMap();
            mTopography = mDIRECT->releaseTopography();
        }
        catch (std::exception &e)
        {
            mLogger->error("Initial YNP search failed");
            mTravelTimeCalculators = mDIRECT->releaseTravelTimeCalculatorMap();
            mTopography = mDIRECT->releaseTopography();
        }
        // Everything managed to fail?
        if (initialSolutions.empty())
        {
            throw std::runtime_error("Could not build initial Utah location");
        }
        // Extract the best initial solution
        std::sort(initialSolutions.begin(), initialSolutions.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                     return lhs.value < rhs.value;
                  });
        auto bestInitialOrigin = initialSolutions.at(0).origin;
        auto bestInitialObjectiveFunction = initialSolutions.at(0).value;
        mLogger->debug("Initial solution is from: "
                     + initialSolutions.at(0).description);
        // Now perform the real-deal search with PSO
        initializeRefinedOptimizer(bestInitialOrigin);
        auto optimalOrigin = bestInitialOrigin;
        double optimalObjectiveFunction{bestInitialObjectiveFunction};
        try
        {
            mLogger->debug("Beginning YNP PSO optimization...");
            auto problem{ULoc::Optimizers::IOptimizer::LocationProblem::ThreeDimensionsAndTime};
            mPSO->locate(bestInitialOrigin, problem, norm);
            if (mPSO->getOptimalObjectiveFunction() < optimalObjectiveFunction)
            {
                mLogger->debug("Using PSO optimimum in YNP");
                optimalObjectiveFunction = mPSO->getOptimalObjectiveFunction();
                optimalOrigin = mPSO->getOrigin();
            }
            else
            {
                mLogger->debug("Using initial location in YNP");
            }
            mTravelTimeCalculators = mPSO->releaseTravelTimeCalculatorMap();
            mTopography = mPSO->releaseTopography();
        }
        catch (const std::exception &e)
        {
            mLogger->error("YNP PSO optimization failed with: "
                         + std::string{e.what()});
            mTravelTimeCalculators = mPSO->releaseTravelTimeCalculatorMap();
            mTopography = mPSO->releaseTopography();
            throw std::runtime_error("YNP PSO optimization failed");
        }
        return optimalOrigin;
    }
    /// @brief Performs the location in Utah
    [[nodiscard]] ULoc::Origin doUtah(const std::vector<ULoc::Arrival> &arrivals,
                                      const bool isQuarry = false)
    {
        constexpr bool applyCorrection{true};
        auto [urtsNorm, pNorm] = mOptions.getNorm();
        auto norm = normToNorm(urtsNorm);
        auto timeWindow = mOptions.getOriginTimeSearchWindow(); 
        int utmZone{12};
        if (mOptions.getUTMZone()){utmZone = *mOptions.getUTMZone();}
        // Look through the stations and event locations
        auto defaultSearchDepth = mOptions.getInitialSearchDepth();
        std::vector<InitialSolution> initialSolutions;
        if (!isQuarry)
        {
            mLogger->debug("Performing station search...");
            try
            {
                auto result = ::searchStations(arrivals,
                                               *mTravelTimeCalculators,
                                               defaultSearchDepth,
                                               norm,
                                               pNorm,
                                               timeWindow,
                                               applyCorrection);
                initialSolutions.push_back({result.first,
                                            "Utah station",
                                            result.second});
            }
            catch (const std::exception &e)
            {
                mLogger->warn("Utah station search failed with: "
                            + std::string{e.what()});
            }
            // Try the known event locations
            mLogger->debug("Performing Utah known location search...");
            try
            {
                auto result
                    = ::searchKnownLocations(arrivals,
                                             *mTravelTimeCalculators,
                                             mTrialUtahEventLocations,
                                             *mRegion,
                                             mLogger,
                                             norm,
                                             pNorm,
                                             timeWindow,
                                             isQuarry,
                                             applyCorrection,
                                             utmZone); 
                initialSolutions.push_back({result.first,
                                            "Utah default event location",
                                            result.second});
            }
            catch (const std::exception &e)
            {
                mLogger->warn("Utah default location search failed with: "
                             + std::string{e.what()});
            }
        }
        // Try the known quarries
        mLogger->debug("Performing Utah known quarry search...");
        ULoc::Origin bestDefaultQuarryBlastOrigin;
        try
        {
            auto result
                = ::searchKnownLocations(arrivals,
                                         *mTravelTimeCalculators,
                                         mTrialUtahQuarryLocations,
                                         *mRegion,
                                         mLogger,
                                         norm,
                                         pNorm,
                                         timeWindow,
                                         isQuarry,
                                         applyCorrection,
                                         utmZone); 
            initialSolutions.push_back({result.first,
                                        "Utah known quarry",
                                        result.second});
        }
        catch (const std::exception &e)
        {
            mLogger->warn("Utah default location search failed with: "
                        + std::string{e.what()});
        }
        // Perform the crude search
        mLogger->debug("Performing initial DIRECT search for Utah...");
        mDIRECT->setArrivals(arrivals);
        mDIRECT->setTopography(std::move(mTopography));
        mDIRECT->setTravelTimeCalculatorMap(std::move(mTravelTimeCalculators));
        try
        {
            if (!isQuarry)
            {
                mDIRECT->locateEventWithFixedDepth(defaultSearchDepth, norm);
            }
            else
            {
                mDIRECT->locateEventAtFreeSurface(norm);
            }
            initialSolutions.push_back({mDIRECT->getOrigin(),
                                        "Utah DIRECT",
                                        mDIRECT->getOptimalObjectiveFunction()});
            mTravelTimeCalculators = mDIRECT->releaseTravelTimeCalculatorMap();
            mTopography = mDIRECT->releaseTopography();
        }
        catch (std::exception &e) 
        {
            mLogger->error("Initial Utah DIRECT search failed");
            mTravelTimeCalculators = mDIRECT->releaseTravelTimeCalculatorMap();
            mTopography = mDIRECT->releaseTopography();
        }
        // Everything managed to fail?
        if (initialSolutions.empty())
        {
            throw std::runtime_error("Could not build initial Utah location");
        }
        // Extract the best initial solution
        std::sort(initialSolutions.begin(), initialSolutions.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                     return lhs.value < rhs.value;
                  });
        auto bestInitialOrigin = initialSolutions.at(0).origin;
        auto bestInitialObjectiveFunction = initialSolutions.at(0).value;
        mLogger->debug("Initial solution is from: "
                     + initialSolutions.at(0).description);
        // Fine tune the search
        auto problem{ULoc::Optimizers::IOptimizer::LocationProblem::ThreeDimensionsAndTime};
        if (isQuarry)
        {
            problem = ULoc::Optimizers::IOptimizer::LocationProblem::FixedToFreeSurfaceAndTime;
        }
        // Now perform the real-deal search with PSO
        initializeRefinedOptimizer(bestInitialOrigin);
        auto optimalOrigin = bestInitialOrigin;
        double optimalObjectiveFunction{bestInitialObjectiveFunction};
        try
        {
            mLogger->debug("Beginning Utah PSO optimization...");
            mPSO->locate(bestInitialOrigin, problem, norm);
            if (mPSO->getOptimalObjectiveFunction() < optimalObjectiveFunction)
            {
                mLogger->debug("Using PSO optimimum in Utah");
                optimalObjectiveFunction = mPSO->getOptimalObjectiveFunction();
                optimalOrigin = mPSO->getOrigin();
            }
            else
            {
                mLogger->debug("Using initial location in Utah");
            }
            mTravelTimeCalculators = mPSO->releaseTravelTimeCalculatorMap();
            mTopography = mPSO->releaseTopography();
        }
        catch (const std::exception &e)
        {
            mLogger->error("YNP PSO optimization failed with: "
                         + std::string{e.what()});
            mTravelTimeCalculators = mPSO->releaseTravelTimeCalculatorMap();
            mTopography = mPSO->releaseTopography();
            throw std::runtime_error("YNP PSO optimization failed");
        }
        return optimalOrigin;
    }
    /// @brief Respond to travel time calculation requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Beginning location...");
 
        LocationRequest locationRequest;
        if (messageType == locationRequest.getMessageType())
        {
            mLogger->debug("Location request received");
            LocationResponse response;
            // Unpack the message
            try
            {
                locationRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(locationRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                response.setReturnCode(
                    LocationResponse::ReturnCode::InvalidRequest);
                return response.clone();
            }

            const auto &arrivals = locationRequest.getArrivalsReference();
            if (arrivals.empty())
            {
                response.setReturnCode(
                    LocationResponse::ReturnCode::InvalidRequest);
                return response.clone();
            }

std::vector<ULoc::Arrival> ulocArrivals;
            ULoc::Origin ulocOrigin;
            if (mIsUtah)
            {
                mLogger->debug("Beginning Utah location process...");
bool isQuarry = false;
                try
                {
                    ulocOrigin = doUtah(ulocArrivals, isQuarry);
                }
                catch (const std::exception &e) 
                {   
                    response.setReturnCode(
                       LocationResponse::ReturnCode::AlgorithmicFailure);
                    return response.clone();
                }   
            }
            else
            {
                mLogger->debug("Beginning YNP location process...");
                try
                {
                    ulocOrigin = doYNP(ulocArrivals); 
                }
                catch (const std::exception &e)
                {
                    response.setReturnCode(
                       LocationResponse::ReturnCode::AlgorithmicFailure);
                    return response.clone();
                }
            }
            mLogger->debug("Event location process finished!");
        }
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
//private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mAQMSConnection{nullptr};
    std::unique_ptr<UMPS::Messaging::RouterDealer::Reply> mReplier{nullptr};
    ServiceOptions mOptions;
    std::vector<ULoc::Position::KnownUtahEvent>
        mTrialUtahEventLocations{ULoc::Position::getKnownUtahEvents()};
    std::vector<ULoc::Position::KnownUtahQuarry>
        mTrialUtahQuarryLocations{ULoc::Position::getKnownUtahQuarries()};
    std::vector<ULoc::Position::KnownYNPEvent>
        mTrialYNPEventLocations{ULoc::Position::getKnownYNPEvents()};
    std::unique_ptr<ULoc::Position::IGeographicRegion> mRegion{nullptr};
    std::unique_ptr<ULoc::TravelTimeCalculatorMap> mTravelTimeCalculators{nullptr};
    std::unique_ptr<ULoc::Topography::ITopography> mTopography{nullptr};
    std::unique_ptr<ULoc::Optimizers::NLOpt::DividedRectangles> mDIRECT{nullptr};
    std::unique_ptr<ULoc::Optimizers::Pagmo::ParticleSwarm> mPSO{nullptr};
    double mMinimumSearchDepth{-1400};
    bool mIsUtah{true};
    bool mKeepRunning{false};
    bool mInitialized{false};
};

/// Constructor
Service::Service() :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr))
{
}

Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, logger))
{
}

Service::Service(std::shared_ptr<UMPS::Messaging::Context> &responseContext,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (responseContext, logger))
{
}

/// Initialize
void Service::initialize(
    const ServiceOptions &options,
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
    // Database connection
    pImpl->mAQMSConnection = nullptr;
    if (connection != nullptr){pImpl->mAQMSConnection = connection;}
    // Get the region
    if (options.getRegion() == ServiceOptions::Region::Utah)
    {
        pImpl->mLogger->debug("Will initialize for Utah region");
        pImpl->mRegion = ULoc::Position::UtahRegion {}.clone();
        pImpl->mIsUtah = true;
    }
    else
    {
        pImpl->mLogger->debug("Will initialize for YNP region");
        pImpl->mRegion = ULoc::Position::YNPRegion {}.clone();
        pImpl->mIsUtah = false;
    }
    // Create the topogrpahy
    if (std::filesystem::exists(options.getTopographyFile()))
    {
        pImpl->mLogger->debug("Loading topography from "
                            + options.getTopographyFile().string());
        auto griddedTopography
            = std::make_unique<ULoc::Topography::Gridded> ();
        griddedTopography->load(options.getTopographyFile(),
                                *pImpl->mRegion);
        pImpl->mTopography = std::move(griddedTopography);
    }
    else
    {
        pImpl->mLogger->warn("Using a constant topography of 2000 m");
        auto constantTopography
            = std::make_unique<ULoc::Topography::Constant> ();
        constantTopography->set(2000);
        pImpl->mTopography = std::move(constantTopography);
    }
    // Create the DIRECT optimizer
    pImpl->mDIRECT
        = std::make_unique<ULoc::Optimizers::NLOpt::DividedRectangles>
          (pImpl->mLogger);
    pImpl->mDIRECT->setGeographicRegion(*pImpl->mRegion);
    pImpl->mDIRECT->setMaximumNumberOfObjectiveFunctionEvaluations(
        options.getNumberOfFunctionEvaluations()); 
    pImpl->mDIRECT->setOriginTimeSearchWindowDuration(
        options.getOriginTimeSearchWindow());
    pImpl->mDIRECT->setLocationTolerance(1000); // TODO i don't think this works
    pImpl->mDIRECT->setOriginTimeTolerance(1); // TODO i don't think this works
    // Create the PSO optimizer
    pImpl->mPSO
        = std::make_unique<ULoc::Optimizers::Pagmo::ParticleSwarm>
          (pImpl->mLogger);
    pImpl->mPSO->setGeographicRegion(*pImpl->mRegion);
    pImpl->mPSO->setNumberOfParticles(options.getNumberOfParticles());
    pImpl->mPSO->setNumberOfGenerations(options.getNumberOfGenerations());
    pImpl->mPSO->setOriginTimeSearchWindowDuration(
        options.getOriginTimeSearchWindow());
    auto minimumSearchDepth        
        =-pImpl->mTopography->getMinimumAndMaximumElevation().first;
    std::pair<double, double> extentInZ{minimumSearchDepth,
                                        options.getMaximumSearchDepth()};
    pImpl->mPSO->setExtentInZ(extentInZ);
    
    // Create the replier
    pImpl->mLogger->debug("Creating replier...");
    UMPS::Messaging::RouterDealer::ReplyOptions replierOptions;
    replierOptions.setAddress(options.getAddress());
    replierOptions.setZAPOptions(options.getZAPOptions());
    replierOptions.setPollingTimeOut(options.getPollingTimeOut());
    replierOptions.setSendHighWaterMark(options.getSendHighWaterMark());
    replierOptions.setReceiveHighWaterMark(
        options.getReceiveHighWaterMark());
    replierOptions.setCallback(std::bind(&ServiceImpl::callback,
                                         &*this->pImpl,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
    pImpl->mReplier->initialize(replierOptions);
    std::this_thread::sleep_for(std::chrono::milliseconds {10});

    // Initialized?
    pImpl->mInitialized = pImpl->mReplier->isInitialized();
    if (pImpl->mInitialized)
    {
        pImpl->mLogger->debug("Service initialized!");
        pImpl->mOptions = options;
    }
    else
    {
        pImpl->mLogger->error("Failed to initialize service!");
    }
}

/// Initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Start the service
void Service::start()
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    if (pImpl->mIsUtah)
    {
        pImpl->mLogger->debug("Starting uLocator Utah location service");
    }
    else
    {
         pImpl->mLogger->debug("Starting uLocator YNP location service");
    }
    pImpl->start();
}

/// Stop the service
void Service::stop()
{
    pImpl->mLogger->debug("Stopping uLocator location service");
    pImpl->stop();
}

/// Running?
bool Service::isRunning() const noexcept
{
    return pImpl->keepRunning();
}
