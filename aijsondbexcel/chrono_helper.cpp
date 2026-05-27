#include "chrono_helper.h"
#include <iostream>
#include <ctime>
#include <chrono>
std::string format_utc(const std::chrono::zoned_seconds& zs) {
    auto tp = zs.get_sys_time();
    auto dp = std::chrono::floor<std::chrono::days>(tp);
    auto ymd = std::chrono::year_month_day{ std::chrono::sys_days{dp} };
    auto time = std::chrono::hh_mm_ss{ tp - dp };
    std::ostringstream oss;
    oss << static_cast<int>(ymd.year()) << "-"
        << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
        << std::setw(2) << static_cast<unsigned>(ymd.day()) << "T"
        << std::setw(2) << time.hours().count() << ":"
        << std::setw(2) << time.minutes().count() << ":"
        << std::setw(2) << time.seconds().count() << "Z";
    return oss.str();
}

std::string utc_time_string(std::tm& tm){
    const auto* local_tz = std::chrono::current_zone();
    auto ymd = std::chrono::year_month_day{
        std::chrono::year{tm.tm_year + 1900},
        std::chrono::month{static_cast<unsigned>(tm.tm_mon + 1)},
        std::chrono::day{static_cast<unsigned>(tm.tm_mday)}
    };
    auto local_time = std::chrono::local_days{ ymd } +
        std::chrono::hours{ tm.tm_hour } +
        std::chrono::minutes{ tm.tm_min } +
        std::chrono::seconds{ tm.tm_sec };

    // Convert to sys_time (UTC)
    auto zt = std::chrono::zoned_time{ local_tz, local_time };
    return format_utc(zt);
}


void print_local(const std::chrono::zoned_seconds& zs) {
    auto lt = zs.get_local_time();
    auto tz = zs.get_time_zone();
    auto dp = std::chrono::floor<std::chrono::days>(lt);
    auto ymd = std::chrono::year_month_day{ std::chrono::local_days{dp} };
    auto time = std::chrono::hh_mm_ss{ lt - dp };
    std::cout << tz->name() << " local: "
        << static_cast<int>(ymd.year()) << "-"
        << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
        << std::setw(2) << static_cast<unsigned>(ymd.day()) << " "
        << std::setw(2) << time.hours().count() << ":"
        << std::setw(2) << time.minutes().count() << ":"
        << std::setw(2) << time.seconds().count() << "\n";
}