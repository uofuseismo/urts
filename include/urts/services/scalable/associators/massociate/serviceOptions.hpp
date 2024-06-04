#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_SERVICE_OPTIONS_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_SERVICE_OPTIONS_HPP
#include <memory>
#include <chrono>
namespace UMPS::Authentication
{
 class ZAPOptions;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/scalable/associators/massociate/serviceOptions.hpp"
/// @brief The options that define the backend MAssociate associator.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ServiceOptions
{
public:
    /// @brief Defines the monitoring region.
    enum class Region
    {
        Utah,
        YNP
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ServiceOptions();
    /// @brief Copy constructor.
    /// @param[in] options  The options from which to initialize
    ///                     this class. 
    ServiceOptions(const ServiceOptions &options);
    /// @brief Move constructor.
    /// @param[in] options  The options from which to initialize this
    ///                     class.  On exit, option's behavior is
    ///                     undefined. 
    ServiceOptions(ServiceOptions &&options) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] options   The options class to copy to this.
    /// @result A deep copy of options.
    ServiceOptions& operator=(const ServiceOptions &options);
    /// @brief Move assignment.
    /// @param[in,out] options  The options whose memory will be moved to
    ///                         this.  On exit, options's behavior is
    ///                         undefined.
    /// @result The memory from options moved to this.
    ServiceOptions& operator=(ServiceOptions &&options) noexcept;
    /// @}

    /// @brief Loads the options from an initialization file.
    /// @param[in] fileName   The name of the initialization file.
    /// @param[in] section    The section of the initialization file with the
    ///                       packet cache service options to be parsed.
    /// @throws std::invalid_argument if the initialization file does not,
    ///         exist cannot be parsed, does not have the specified section,
    ///         or has incorrect information.
    void parseInitializationFile(const std::string &fileName,
                                 const std::string &section = "MAssociate");

    /// @name Replier Required Options
    /// @{
 
    /// @brief Sets the replier address to which to connect.
    /// @param[in] address  The address to which this backend service will
    ///                     connect.
    void setAddress(const std::string &address);
    /// @result The replier address to which to connect.
    /// @throws std::runtime_error if \c haveBackendAddress() is false.
    [[nodiscard]] std::string getAddress() const;
    /// @result True indicates the replier address was set.
    [[nodiscard]] bool haveAddress() const noexcept;
    /// @} 

    /// @name Optional Options
    /// @{

    /// @brief Sets the ZeroMQ Authentication Protocol options.
    /// @param[in] zapOptions  The ZAP options for the replier.
    void setZAPOptions(
        const UMPS::Authentication::ZAPOptions &zapOptions) noexcept;
    /// @result The ZAP options.
    [[nodiscard]] UMPS::Authentication::ZAPOptions getZAPOptions() const noexcept;

    /// @brief To receive replies we poll on a socket.  After this amount of
    ///        time has elapsed the process can proceed and handle other
    ///        essential activities like if the program has stopped.  If this
    ///        is too small then the thread will needlessly burn cycles on a
    ///        while loop but if its too big then thread will be unresponsive
    ///        on shut down.
    /// @param[in] timeOut  The time to wait for a request before the thread
    ///                     checks other things.
    /// @throws std::invalid_argument if this is negative.
    void setPollingTimeOut(const std::chrono::milliseconds &timeOut);
    /// @result The polling time out.
    [[nodiscard]] std::chrono::milliseconds getPollingTimeOut() const noexcept;
    /// @brief Influences the maximum number of request messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setReceiveHighWaterMark(int highWaterMark);
    /// @result The high water mark.  The default is 4096.
    [[nodiscard]] int getReceiveHighWaterMark() const noexcept;
    /// @brief Influences the maximum number of response messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setSendHighWaterMark(int highWaterMark);
    /// @result The send high water mark.  The default is 8192.
    [[nodiscard]] int getSendHighWaterMark() const noexcept;
    /// @}

    /// @name Associator Options
    /// @{

    /// @brief Sets the association region.
    /// @param[in] region   The association region.
    void setRegion(Region region) noexcept;
    /// @result The association region.
    /// @throws std::invalid_argument if the region was not set.
    [[nodiscard]] Region getRegion() const;
    /// @result True indicates the region was set.
    [[nodiscard]] bool haveRegion() const noexcept;

    /// @brief Sets the static corrections file.
    /// @param[in] correctionsFile  The HDF5 file with the static corrections.
    void setStaticCorrectionFile(const std::string &correctionFile);
    /// @result The file with the station statics.
    /// @note By default the result is empty and statics are not used.
    [[nodiscard]] std::string getStaticCorrectionFile() const noexcept;

    /// @brief Sets the source-specific corrections file.
    /// @param[in] correctionsFile  The HDF5 file with the static corrections.
    void setSourceSpecificCorrectionFile(const std::string &correctionsFile);
    /// @result The file with the source-specific corrections.
    /// @note By default the result is empty and source-specific corrections
    ///       are not used.
    [[nodiscard]] std::string getSourceSpecificCorrectionFile() const noexcept;

    /// @brief The epsilon in DBSCAN when clustering on origin time.
    /// @param[in] epsilon   The epsilon in seconds.
    /// @throws std::invalid_argument if epsilon is not positive.
    void setDBSCANEpsilon(double epsilon);
    /// @result The epsilon in seconds.  By default this is 0.25 seconds.
    [[nodiscard]] double getDBSCANEpsilon() const noexcept;

    /// @brief Sets the minimum number of events required to nucleate.
    /// @param[in] minimumClusterSize  The minimum cluster size.
    ///                                This must be at least 4.
    void setDBSCANMinimumClusterSize(int minimumClusterSize);
    /// @result The minimum number of phases required to nucleate an event.
    [[nodiscard]] int getDBSCANMinimumClusterSize() const noexcept;

    /// @brief Beyond this distance, in meters, picks wills not be associated.
    /// @param[in] distance   The distance up to which picks will be associated.
    /// @throws std::invalid_argument if this is not positive.
    void setMaximumDistanceToAssociate(double distance);
    /// @result The maximum distance to associate. 
    ///         By default this is 150,000 m.
    [[nodiscard]] double getMaximumDistanceToAssociate() const noexcept;

    /// @brief Defines the number of particles in the migration 
    ///        particle swarm optimizer.
    /// @param[in] nParticles  The number of particles in the optimizer.
    /// @throws std::invalid_argument if nParticles is not positive.
    void setNumberOfParticles(int nParticles);
    /// @result The number of particles.  By default this is60.
    [[nodiscard]] int getNumberOfParticles() const noexcept;

    /// @brief Sets the number of epochs in advancing the particle swarm
    ///        optimizer.
    /// @param[in] nEpochs  Th enumber of epochs to advance the particle swarm.
    /// @throws std::invalid_argument if nEpochs is not positive.
    void setNumberOfEpochs(int nEpochs); 
    /// @result The number of epochs.
    [[nodiscard]] int getNumberOfEpochs() const noexcept;

    /// @brief Sets the search extent in depth.
    /// @param[in] extent   extent.first is the shallowest depth to search
    ///                     in meters and extent.second si the deepest depth
    ///                     to search in meters.
    /// @throws std::invalid_argument if extent.first >= extent.second,
    ///         extent.first < -8600 metesr or extent.second > 800,000 meters.
    void setExtentInDepth(const std::pair<double, double> &extent);
    /// @result The extent in depth for association.  result.first is the 
    ///         shallowest depth in meters, and result.second is the
    ///         deepest depth.
    [[nodiscard]] std::pair<double, double> getExtentInDepth() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~ServiceOptions();
    /// @}
private:
    class ServiceOptionsImpl;
    std::unique_ptr<ServiceOptionsImpl> pImpl;
};
}
#endif
