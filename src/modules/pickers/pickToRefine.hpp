#ifndef PICK_TO_REFINE_HPP
#define PICK_TO_REFINE_HPP
#include <chrono>
#include "urts/broadcasts/internal/pick/pick.hpp"
namespace
{

struct PickToRefine
{
    PickToRefine() = default;
    explicit PickToRefine(URTS::Broadcasts::Internal::Pick::Pick &&initialPick) :
        pick(std::move(initialPick))
    {
    }
    URTS::Broadcasts::Internal::Pick::Pick pick;
    std::chrono::milliseconds firstTry
    {   
        std::chrono::duration_cast<std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now().time_since_epoch())
    };  
    std::chrono::milliseconds nextTry{firstTry};
    int mTries{0};
    void setNextTry()
    {   
        mTries = mTries + 1;
        if (mTries == 1)
        {
            nextTry = firstTry + std::chrono::milliseconds{3000};
        }
        else if (mTries == 2)
        {
            nextTry = firstTry + std::chrono::milliseconds {10000};
        }
        else
        {
            throw std::runtime_error("Exceeded number of tries");
        }
    }
    [[nodiscard]] bool isReadyToProcess()
    {
        if (mTries == 0){return true;}
        auto now
            = std::chrono::duration_cast<std::chrono::milliseconds> (
                 std::chrono::high_resolution_clock::now().time_since_epoch());
        if (now > nextTry){return true;}
        return false;
    }  
};

}
#endif
