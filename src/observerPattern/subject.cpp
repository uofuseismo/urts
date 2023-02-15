#include <vector>
#include <map>
#include "urts/observerPattern/subject.hpp"
#include "urts/observerPattern/observer.hpp"
#include "urts/observerPattern/enums.hpp"

using namespace URTS::ObserverPattern;

namespace
{
typedef std::vector<IObserver *> ObserverList;
}

/// Implementation
class ISubject::ISubjectImpl
{
public:
    typedef std::map<Message, ObserverList> ObserverMap;
    ObserverMap mObservers;
};

/// Constructor
ISubject::ISubject() :
    pImpl(std::make_unique<ISubjectImpl> ())
{
}

/// Destructor
ISubject::~ISubject() = default;

/// An observer subscribes to a message with desired message type.
/// Source code from Martin Reddy's API design for C++ with the main
/// difference being I made the message type an enum.
[[maybe_unused]]
void ISubject::subscribe(const Message message, IObserver *observer)
{
    if (observer)
    {
        auto it = pImpl->mObservers.find(message);
        if (it == pImpl->mObservers.end())
        {
            pImpl->mObservers[message] = ObserverList();
        }
        pImpl->mObservers[message].push_back(observer);
    }
}

/// An observer unsubscribes from a message with a desired message type.
/// Source code from Martin Reddy's API design for C++ with the main
/// difference being I made the message type an enum.
[[maybe_unused]]
void ISubject::unsubscribe(const Message message, IObserver *observer)
{
    auto it = pImpl->mObservers.find(message);
    if (it != pImpl->mObservers.end())
    {
        ObserverList &list = pImpl->mObservers[message];
        ObserverList::iterator li;
        for (li = list.begin(); li != list.end();)
        {
            if ((*li) == observer)
            {
                list.erase(li);
            }
            else
            {
                ++li;
            }
        }
    }
}

/// Updates all observers monitoring a subject.
/// Source code from Martin Reddy's API design for C++ with the main
/// difference being I made the message type an enum and change
/// loop style.
[[maybe_unused]] void ISubject::notify(Message message)
{
    auto it = pImpl->mObservers.find(message);
    if (it != pImpl->mObservers.end())
    {
        ObserverList &list = pImpl->mObservers[message];
        for (auto &li : list)
        {
            li->updateObserver(message);
        }
    }
}
