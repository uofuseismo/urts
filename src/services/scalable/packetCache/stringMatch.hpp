#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_STRING_MATCH_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_STRING_MATCH_HPP 
#include <string>
#include <vector>
namespace
{
/// Tests whether string match.  For example:
/// matches(BHZ, BHZ) -> True
/// matches(BHZ, BH?) -> True
/// matches(BHZ, B*)  -> True 
/// matches(BHZ, *HZ) -> True
/// matches(BHZ, *H?) -> True
/// @param[in] string   The string to test.
/// @param[in] pattern  The pattern to match against.
/// @result True indicates the string matches the pattern.
[[nodiscard]] bool stringMatch(const std::string &string,
                               const std::string &pattern)
{
    auto m = static_cast<int> (pattern.size());
    auto n = static_cast<int> (string.size());

    // Empty pattern can only match with empty string
    if (m == 0 && n == 0){return true;}
    if (m == n)
    {
        if (string == pattern){return true;}
    }
    // Lookup table for storing results of subproblems
    std::vector<std::vector<bool>> lookUpTable(n + 1);
    for (auto &row : lookUpTable)
    {
        row.resize(m + 1, false);
    }
    lookUpTable[0][0] = true;
    // Only '*' can match with empty string
    for (int j = 1; j <= m; j++)
    {
        if (pattern[j - 1] == '*')
        {
            lookUpTable[0][j] = lookUpTable[0][j - 1];
        }
    }
    // Fill the table in bottom-up fashion
    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= m; j++)
        {
            // Two cases if we see a '*'
            // a) We ignore ‘*’ character and move
            //    to next  character in the pattern,
            //     i.e., ‘*’ indicates an empty sequence.
            // b) '*' character matches with ith
            //     character in input
            if (pattern[j - 1] == '*')
            {
                lookUpTable[i][j]
                    = lookUpTable[i][j - 1] || lookUpTable[i - 1][j];
            }
            // Current characters are considered as
            // matching in two cases
            // (a) current character of pattern is '?'
            // (b) characters actually match
            else if (pattern[j - 1] == '?' || string[i - 1] == pattern[j - 1])
            {
                lookUpTable[i][j] = lookUpTable[i - 1][j - 1];
            } 
            // If characters don't match
            else
            {
                lookUpTable[i][j] = false;
            }
        }
    }
    return lookUpTable[n][m];
}

}

/*
int main()
{
    std::cout << ::stringMatch("BHZ", "BHZ")*1 << " " << 1 << std::endl;
    std::cout << ::stringMatch("BHZ", "BH*")*1 << " " << 1 << std::endl;
    std::cout << ::stringMatch("BHZ", "B*")*1  << " " << 1 << std::endl;
    std::cout << ::stringMatch("BHZ", "BH?")*1 << " " << 1 << std::endl;
    std::cout << ::stringMatch("BHZ", "**")*1  << " " << 1 << std::endl;
    std::cout << ::stringMatch("HHZ", "BH*")*1 << " " << 0 << std::endl;
    std::cout << ::stringMatch("HHZ", "?H*")*1 << " " << 1 << std::endl;
    std::cout << ::stringMatch("HHZ", "?N*")*1 << " " << 0 << std::endl;
    std::cout << ::stringMatch("HHZ", "*")*1   << " " << 1 << std::endl;
}
*/
#endif
