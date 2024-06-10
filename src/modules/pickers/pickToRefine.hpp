#ifndef PICK_TO_REFINE_HPP
#define PICK_TO_REFINE_HPP
#include <chrono>
#include "urts/broadcasts/internal/pick/pick.hpp"
namespace
{

struct PickToRefine
{
    URTS::Broadcasts::Internal::Pick::Pick mPick;
    std::chrono::seconds mFirstTry
    {   
        std::chrono::duration_cast<std::chrono::seconds> (
            std::chrono::system_clock::now().time_since_epoch())
    };  
    std::chrono::seconds mNextTry{mFirstTry};
    int mTries{0};
    void setNextTry()
    {   
        mTries = mTries + 1;
        if (mTries == 1)
        {
            mNextTry = mFirstTry + std::chrono::seconds{3};
        }
        else if (mTries == 2)
        {
            mNextTry = mNextTry + std::chrono::seconds {10};
        }
        else
        {
            throw std::runtime_error("Exceeded number of tries");
        }
    }
};


}
#endif
