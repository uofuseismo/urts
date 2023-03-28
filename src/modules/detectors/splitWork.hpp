#ifndef URTS_MODULES_DETECTORS_SPLIT_WORK_HPP
#define URTS_MODULES_DETECTORS_SPLIT_WORK_HPP
#include <cmath>
#include <vector>
namespace
{
/// @brief Attempts to evenly split work items among workers.
/// @param[in] nWorkItems  The number of tasks to subdivide.
/// @param[in] nWorkers    The number of workers (e.g., threads or processors).
/// @result This is an array of dimension nWorkers + 1.  The jobs assigned to
///         the i'th worker are given by result[i] to result[i+1] (exclusive).
/// @note This assumes the amount of work between jobs is approximately equal.
/// @throws std::invalid_argument if nWorkItems is negative or nWorkers is
///         not positive.
std::vector<int> splitWork(const int nWorkItems,
                           const int nWorkers = 1)
{
    if (nWorkItems < 0)
    {
        throw std::invalid_argument(
            "Number of work items must be non-negative");
    }
    if (nWorkers < 1)
    {
        throw std::invalid_argument("Number of workers must be positive");
    }
    std::vector<int> result(nWorkers + 1, 0);
    if (nWorkItems == 0){return result;}
    if (nWorkers == 1)
    {
        result[0] = 0;
        result[1] = nWorkItems;
    }
    // Subdivide work
    auto dWork = static_cast<double> (nWorkItems)/nWorkers;
    result[0] = 0;
    for (int i = 1; i < nWorkers; ++i)
    {
        result[i] = std::min(nWorkItems, static_cast<int> (dWork*i));
    }
    result[nWorkers] = nWorkItems;
    return result;
}

/*

int main()
{
    // Only work item
    auto result = splitWork(5, 1); 
    assert(result.size() == 2);  
    assert(result[0] == 0); 
    assert(result[1] == 5); 

    // No work items
    result = splitWork(0, 2); 
    assert(result.size() == 3); 
    assert(result[0] == 0); 
    assert(result[1] == 0); 
    assert(result[2] == 0); 

    // Ideal case
    result = splitWork(12, 4); 
    assert(result.size() == 5); 
    assert(result[0] == 0); 
    assert(result[1] == 3); 
    assert(result[2] == 6); 
    assert(result[3] == 9); 
    assert(result[4] == 12);

    // A general case
    result = splitWork(20, 3); 
    assert(result.size() == 4); 
    assert(result[0] == 0); 
    assert(result[1] == 6); 
    assert(result[2] == 13);
    assert(result[3] == 20);

    // More threads than work
    result = splitWork(3, 5);  
    for (auto &r : result){std::cout << r << std::endl;}
}
*/
}
#endif
