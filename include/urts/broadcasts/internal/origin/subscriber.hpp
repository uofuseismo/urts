#ifndef URTS_BROADCASTS_INTERNAL_ORIGIN_SUBSCRIBER_HPP
#define URTS_BROADCASTS_INTERNAL_ORIGIN_SUBSCRIBER_HPP
#include <memory>
#include <umps/logging/log.hpp>
#include <umps/messaging/context.hpp>
namespace URTS::Broadcasts::Internal::Origin
{
 class Origin;
 class SubscriberOptions;
}
namespace URTS::Broadcasts::Internal::Origin
{
/// @class Subscriber "subscriber.hpp" "urts/broadcasts/internal/origin/subscriber.hpp"
/// @brief A convenience function to subscribe to a origin broadcast.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Modules_Broadcasts_Internal_Origin
class Subscriber
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Subscriber();
    /// @brief Constructs a subscriber with the given logger.
    /// @param[in] logger  A pointer to the application's logger.
    explicit Subscriber(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Constructs a subscriber with a given ZeroMQ context.
    /// @param[in] context  The context from which to initialize.
    /// @note This can be useful for inproc communication where a separate
    ///       thread IO thread is not required.  In this case, the context
    ///       can be made with:
    ///       auto context = std::shared_ptr<UMPS::Messaging::Context> (0).
    explicit Subscriber(std::shared_ptr<UMPS::Messaging::Context> &context);
    /// @brief Construtcs a subscriber with a given ZeroMQ context and logger.
    Subscriber(std::shared_ptr<UMPS::Messaging::Context> &context,
               std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Move constructor.
    /// @param[in,out] subscriber  The subscriber from which to initialize this
    ///                            class.  On exit, subscriber's behavior is
    ///                            undefined. 
    Subscriber(Subscriber &&subscriber) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Move assignment.
    /// @param[in,out] subscriber  The subscriber whose memory will be moved to
    ///                            this.  On exit, subscriber's behavior is
    ///                            undefined.
    Subscriber& operator=(Subscriber &&subscriber) noexcept;
    /// @}

    /// @brief Sets the subscriber options.
    /// @throws std::invalid_argument if options.haveAddress() is false.
    /// @throws std::runtime_error if the connection cannot be established.
    void initialize(const SubscriberOptions &options);
    /// @result True indicates that the subscriber is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Receives an origin message.
    /// @throws std::invalid_argument if the message cannot be serialized.
    /// @throws std::runtime_error if \c isIinitialized() is false.
    [[nodiscard]] std::unique_ptr<Origin> receive() const;

    /// @brief Destructor. 
    ~Subscriber();

    Subscriber(const Subscriber &) = delete;
    Subscriber& operator=(const Subscriber &) = delete;
private:
    class SubscriberImpl;
    std::unique_ptr<SubscriberImpl> pImpl;
};
}
#endif
