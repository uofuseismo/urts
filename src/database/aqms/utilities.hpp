#ifndef PRIVATE_DATABASE_AQMS_UTILITIES_HPP
#define PRIVATE_DATABASE_AQMS_UTILITIES_HPP
#include <iostream>
#include <chrono>
#include <date/date.h>
//#include <time/utc.hpp>
#ifndef NDEBUG
#include <cassert>
#endif
namespace
{

/*
[[maybe_unused]]
[[nodiscard]]
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
*/

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

[[maybe_unused]] [[nodiscard]]
std::string formatTimeUTC(const std::chrono::microseconds &time)
{
    std::string result(27, '\0');
    auto timeStamp = static_cast<double> (time.count())*1.e-6;
    // Figure out the fractional second 
    auto iUTCStamp = static_cast<int64_t> (timeStamp);
    auto fraction = timeStamp - static_cast<double> (iUTCStamp);
    auto microSecond = static_cast<int> (std::lround(fraction*1.e6));
    // Create the epochal time
    std::chrono::seconds chronoSeconds(iUTCStamp);
    std::chrono::system_clock::time_point timePoint{chronoSeconds};
    // Year/month/day
    //const std::chrono::year_month_day
    //     yearMonthDate{std::chrono::floor<std::chrono::days> (timePoint)};
    auto dayPoint = date::floor<date::days> (timePoint);
    date::year_month_day ymd{dayPoint};
    auto year = static_cast<int> (ymd.year());
    auto month = static_cast<int> (unsigned(ymd.month()));
    auto dayOfMonth = static_cast<int> (unsigned(ymd.day()));
    // Hour/minute/second        
    //const std::chrono::hh_mm_ss hourMinuteSecond{timePoint - dayPoint}; 
    date::hh_mm_ss tod{timePoint - dayPoint};
    auto hour = static_cast<int> (tod.hours().count());
    auto minute = static_cast<int> (tod.minutes().count());
    auto second = static_cast<int> (tod.seconds().count());
    // Format it
    sprintf(result.data(), "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ",
            year, month, dayOfMonth,
            hour, minute, second,
            microSecond);
    return result;
}

}
#endif
