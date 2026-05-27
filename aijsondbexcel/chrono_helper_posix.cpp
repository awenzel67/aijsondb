#include "chrono_helper.h"
#include <ctime>
#include <string>
std::string utc_time_string(tm& tm)
{
    std::string ret;
    std::time_t t = std::mktime(&tm);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    std::tm* utc=std::gmtime(&t);
    std::strftime(buf, sizeof buf, "%FT%TZ", utc );
    ret = buf;
    return ret;
}