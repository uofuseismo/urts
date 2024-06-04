#ifndef URTS_BROADCASTS_INTERNAL_PICK_JSON_HELPERS_HPP
#define URTS_BROADCASTS_INTERNAL_PICK_JSON_HELPERS_HPP
#include <string>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"

namespace
{

void propertiesToJSON(const URTS::Broadcasts::Internal::Pick::Pick &pick,
                      nlohmann::json &obj)
{
    //nlohmann::json obj;
    // Essential stuff (this will throw): 
    // Network/Station/Channel/Location
    //obj["MessageType"] = pick.getMessageType();
    //obj["MessageVersion"] = pick.getMessageVersion();
    obj["Network"] = pick.getNetwork();
    obj["Station"] = pick.getStation();
    obj["Channel"] = pick.getChannel();
    obj["LocationCode"] = pick.getLocationCode();
    // Pick time
    obj["Time"] = static_cast<int64_t> (pick.getTime().count());
    // Identifier
    obj["Identifier"] = pick.getIdentifier();
    // Non-essential stuff:
    if (pick.haveLowerAndUpperUncertaintyBound())
    {
        auto bounds = pick.getLowerAndUpperUncertaintyBound();
        nlohmann::json jsonBounds;
        jsonBounds["LowerPercentile"] = bounds.first.getPercentile();
        jsonBounds["LowerPerturbation"]
            = bounds.first.getPerturbation().count();
        jsonBounds["UpperPercentile"] = bounds.second.getPercentile();
        jsonBounds["UpperPerturbation"]
            = bounds.second.getPerturbation().count();
        obj["UncertaintyBounds"] = jsonBounds;
    }
    else
    {
        obj["UncertaintyBounds"] = nullptr;
    }
    // Original channels
    auto originalChannels = pick.getOriginalChannels();
    if (!originalChannels.empty())
    {
        obj["OriginalChannels"] = originalChannels;
    }
    else
    {
        obj["OriginalChannels"] = nullptr;
    }
    // Phase hint
    auto phaseHint = pick.getPhaseHint();
    if (!phaseHint.empty())
    {
        obj["PhaseHint"] = phaseHint;
    }
    else
    {
        obj["PhaseHint"] = nullptr;
    }
    // First motion
    obj["FirstMotion"] = static_cast<int> (pick.getFirstMotion()); 
    // Review
    obj["ReviewStatus"] = static_cast<int> (pick.getReviewStatus());
    // Algorithm
    obj["ProcessingAlgorithms"] = pick.getProcessingAlgorithms();
    //return obj;
}

}

#endif
