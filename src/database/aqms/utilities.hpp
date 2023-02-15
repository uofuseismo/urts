#ifndef PRIVATE_DATABASE_AQMS_UTILITIES_HPP
#define PRIVATE_DATABASE_AQMS_UTILITIES_HPP
#include <iostream>
#include <time/utc.hpp>
#ifndef NDEBUG
#include <cassert>
#endif
namespace
{

Time::UTC fromTM(const std::tm &tm)
{
   Time::UTC result;
   int year = tm.tm_year + 1900;

   int second = 0;
   int carryMinute = 0;
   if (tm.tm_sec < 60)
   {
       second = tm.tm_sec;
   }
   else
   {
       carryMinute = 1;
   }

   int minute = tm.tm_min + carryMinute;
   int carryHour = 0;
   if (minute >= 60)
   {
       minute = minute%60;
       carryHour = 1;  
   }
   
   int hour = tm.tm_hour + carryHour;
   int carryDay = 0;
   if (hour >= 24)
   {
       hour = hour%24;
       carryDay = 1;
   }

   if (carryDay > 0){std::cerr << "Overflow day not done" << std::endl;}
 
   int month = tm.tm_mon + 1;
   int dom = tm.tm_mday;
   result.setYear(year);
   result.setMonthAndDay(std::pair(month, dom));
   result.setHour(hour);
   result.setMinute(minute);
   result.setSecond(second);
   return result;
}

[[maybe_unused]]
[[nodiscard]]
double lonTo180(const double lonIn)
{
    auto lon = lonIn;
    if (lon < -180)
    {
        for (int k = 0; k < std::numeric_limits<int>::max(); ++k)
        {
            auto tempLon = lon + k*360;
            if (tempLon >= -180)
            {
                lon = tempLon;
                break;
            }
        }
    }
    if (lon >= 180)
    {
        for (int k = 0; k < std::numeric_limits<int>::max(); ++k)
        {
            auto tempLon = lon - k*360;
            if (tempLon < 180)
            {
                lon = tempLon;
                break;
            }
        }
    }
#ifndef NDEBUG
    assert(lon >= -180 && lon < 180);
#endif
    return lon;
}
}
#endif
