#include <umps/messageFormats/message.hpp>
#include <umps/logging/standardOut.hpp>
#include <massociate/pick.hpp>
#include <massociate/associator.hpp>
#include "urts/services/scalable/associators/massociate/service.hpp"
//#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
//#include "urts/services/scalable/associators/massociate/assocationRequest.hpp"
//#include "urts/services/scalable/associators/massociate/assocationResponse.hpp"

namespace MASS = MAssociate;
using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

}

class Service::ServiceImpl
{
public:
    /// @brief Respond to travel time calculation requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Beginning association...");
 
std::vector<MASS::Pick> picks;
        mAssociator->setPicks(picks);
        mAssociator->associate();
    }
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<MASS::Associator> mAssociator{nullptr};
};

/// Destructor
Service::~Service() = default;
