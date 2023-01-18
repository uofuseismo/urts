#include <string>
#include "urts/version.hpp"

using namespace URTS;

int Version::getMajor() noexcept
{
    return URTS_MAJOR;
}

int Version::getMinor() noexcept
{
    return URTS_MINOR;
}

int Version::getPatch() noexcept
{
    return URTS_PATCH;
}

bool Version::isAtLeast(const int major, const int minor,
                        const int patch) noexcept
{
    if (URTS_MAJOR < major){return false;}
    if (URTS_MAJOR > major){return true;}
    if (URTS_MINOR < minor){return false;}
    if (URTS_MINOR > minor){return true;}
    if (URTS_PATCH < patch){return false;}
    return true;
}

std::string Version::getVersion() noexcept
{
    std::string version(URTS_VERSION);
    return version;
}
