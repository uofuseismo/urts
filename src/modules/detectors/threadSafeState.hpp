#ifndef PRIVATE_DETECTORS_THREAD_SAFE_STATE_HPP
#define PRIVATE_DETECTORS_THREAD_SAFE_STATE_HPP
#include <mutex>
#include <atomic>
namespace
{

class ThreadSafeState
{
public:
    enum class State
    {
        Undefined,
        Initializing,
        ReadyToQueryData,
        QueryData,
        ReadyForInferencing,
        InferencePS, 
        InferenceP,
        InferenceS,
        ReadyForBroadcasting,
        BroadcastPS,
        BroadcastP,
        BroadcastS,
        Destroy
    }; 
public:
    ThreadSafeState() = default;
    explicit ThreadSafeState(const State state)
    {
        setState(state);
    }
    explicit ThreadSafeState(const size_t hash)
    {
        setHash(hash);
    }
    ThreadSafeState(const size_t hash, const State state)
    {
        setHash(hash);
        setState(state);
    }
    ~ThreadSafeState() = default;
    ThreadSafeState(const ThreadSafeState &state)
    {
        *this = state;
    }
    ThreadSafeState(ThreadSafeState &&state) noexcept
    {
        *this = std::move(state);
    }
    ThreadSafeState &operator=(const ThreadSafeState &state)
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        mHash = state.getHash();//state.mHash;
        mState = state.getState();//state.mState;
        return *this;
    }
    ThreadSafeState &operator=(const ThreadSafeState &&state) noexcept
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        mHash = state.getHash(); //mHash; //std::move(state.mHash);
        mState = state.getState();//mState; //std::move(state.mState);
        return *this;
    }
    void setState(const State state) noexcept
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        mState = state;
    }
    [[nodiscard]] State getState() const noexcept
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        return mState;
    }
    void setHash(const size_t hash) noexcept
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        mHash = hash;
    }
    [[nodiscard]] size_t getHash() const noexcept
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
        return mHash;
    }
//private:
    //mutable std::mutex mMutex;
    std::atomic<size_t> mHash{0};
    std::atomic<State> mState{State::Undefined};
};

[[maybe_unused]]
[[nodiscard]] bool operator<(const ::ThreadSafeState &lhs,
                             const ::ThreadSafeState &rhs)
{
    return lhs.getHash() < rhs.getHash();
}
}
#endif
