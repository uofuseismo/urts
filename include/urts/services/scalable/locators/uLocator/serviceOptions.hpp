#ifndef URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_SERVICE_OPTIONS_HPP
#define URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_SERVICE_OPTIONS_HPP
#include <memory>
#include <chrono>
#include <filesystem>
namespace UMPS::Authentication
{
 class ZAPOptions;
}
namespace URTS::Services::Scalable::Locators::ULocator
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
/// @brief The options that define the backend uLocator event locator.
///        The location happens in two phases.  In the first phase, a crude
///        optimization with the DIvided RECTangle algorithm is performed.
///        This initial solution is then refined with
///        Particle Swarm Optimization.
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
    /// @brief The norm.
    enum class Norm
    {
        Lp,
        L1,
        L2
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

    /// @name Locator Options
    /// @{

    /// @brief Sets the location region.
    /// @param[in] region   The location region.
    void setRegion(Region region) noexcept;
    /// @result The location region.
    /// @throws std::invalid_argument if the region was not set.
    [[nodiscard]] Region getRegion() const;
    /// @result True indicates the region was set.
    [[nodiscard]] bool haveRegion() const noexcept;

    /// @brief Sets the static corrections file.
    /// @param[in] correctionsFile  The HDF5 file with the static corrections.
    void setStaticCorrectionFile(const std::filesystem::path &correctionFile);
    /// @result The file with the station statics.
    /// @note By default the result is empty and statics are not used.
    [[nodiscard]] std::filesystem::path getStaticCorrectionFile() const noexcept;

    /// @brief Sets the source-specific corrections file.
    /// @param[in] correctionsFile  The HDF5 file with the static corrections.
    void setSourceSpecificCorrectionFile(const std::filesystem::path &correctionsFile);
    /// @result The file with the source-specific corrections.
    /// @note By default the result is empty and source-specific corrections
    ///       are not used.
    [[nodiscard]] std::filesystem::path getSourceSpecificCorrectionFile() const noexcept;

    /// @brief Sets the region's topographic file.
    /// @param[in] topographyFile  The HDF5 file with the topography.
    void setTopographyFile(const std::filesystem::path &topographyFile);
    /// @result The file with the region's topography.
    /// @note By default the result is empty and topography is not used.
    ///       Instead, a fixed elevation of 1400 meters is selected.
    [[nodiscard]] std::filesystem::path getTopographyFile() const noexcept;

    /// @brief Sets the origin time search window.  This ranges from the time
    ///        of the earliest arrival - window to the time of the earliest
    ///        arrival.  A good choice is to pick the maximum moveout
    ///        you'd expect in a network then add a bit more.
    /// @param[in] window   The window duration.
    /// @throws std::invalid_argument if this is not positive.
    void setOriginTimeSearchWindow(double window);
    /// @result The origin time search window in seconds.
    [[nodiscard]] double getOriginTimeSearchWindow() const noexcept;

    /// @brief Sets the norm.
    /// @param[in] norm  The norm.
    /// @param[in] p     If the norm is Lp then this is the value of p which
    ///                  cannot be less than 1.
    void setNorm(Norm norm, double p = 1.5);
    /// @result The norm and, if applicable, the p value.
    [[nodiscard]] std::pair<Norm, double> getNorm() const noexcept;
    

    /// @brief Sets the number of function evaluations for DIRECT.
    /// @param[in] nFunctionEvaluations  The number of function evaluations.
    /// @throws std::invalid_argument if this is not positive.
    void setNumberOfFunctionEvaluations(int nFunctionEvaluations);
    /// @result The number of function evaluations for DIRECT.
    /// @note By default this is 1200.
    [[nodiscard]] int getNumberOfFunctionEvaluations() const noexcept; 


    /// @brief Sets the horizontal refinement search size in meters.  Hence,
    ///        when we switch to the PSO optimization we will only search
    ///        this size +/- about the initial solution, 35,000 to 50,000
    ///        is a good choice for regional monitoring.
    /// @param[in] refinement   The refinement in meters.
    void setHorizontalRefinement(double refinement);
    /// @result The horizontal refinement search size in meters.
    [[nodiscard]] double getHorizontalRefinement() const noexcept;

    /// @brief Defines the number of particles in the migration 
    ///        particle swarm optimizer.
    /// @param[in] nParticles  The number of particles in the optimizer.
    /// @throws std::invalid_argument if nParticles is not positive.
    void setNumberOfParticles(int nParticles);
    /// @result The number of particles.  By default this is60.
    [[nodiscard]] int getNumberOfParticles() const noexcept;

    /// @brief Sets the number of generations in advancing the particle swarm
    ///        optimizer.
    /// @param[in] nGenerations  The number of generations to advance the
    ///                          particle swarm.
    /// @throws std::invalid_argument if nGenerations is not positive.
    void setNumberOfGenerations(int nGenerations); 
    /// @result The number of epochs.
    [[nodiscard]] int getNumberOfGenerations() const noexcept;

    /// @brief Sets the maximum search depth.
    /// @param[in] depth   The maximum search depth in meters w.r.t.
    ///                    to sea-level.
    /// @throws std::invalid_argument if
    ///         depth < -8600 meters or depth > 800,000 meters.
    void setMaximumSearchDepth(double extent);
    /// @result The extent in depth for searching in meters.
    [[nodiscard]] double getMaximumSearchDepth() const noexcept;
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
