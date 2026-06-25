#include "time-format.hpp"

#include <cctype>
#include <chrono>
#include <ctime>
#include <sstream>

namespace {

bool is_french_locale(const std::string &locale_tag)
{
	return locale_tag.rfind("fr", 0) == 0;
}

const char *weekday_fr(int wday)
{
	static const char *names[] = {"dimanche", "lundi",   "mardi", "mercredi",
				      "jeudi",    "vendredi", "samedi"};
	if (wday < 0 || wday > 6) {
		return "";
	}
	return names[wday];
}

const char *month_fr(int month)
{
	static const char *names[] = {"janvier",   "février", "mars",     "avril",   "mai",      "juin",
				      "juillet",   "août",    "septembre", "octobre", "novembre", "décembre"};
	if (month < 1 || month > 12) {
		return "";
	}
	return names[month - 1];
}

const char *weekday_en(int wday)
{
	static const char *names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	if (wday < 0 || wday > 6) {
		return "";
	}
	return names[wday];
}

const char *month_en(int month)
{
	static const char *names[] = {"January",  "February", "March",   "April",   "May",     "June",
				      "July",     "August",   "September", "October", "November", "December"};
	if (month < 1 || month > 12) {
		return "";
	}
	return names[month - 1];
}

std::string capitalize(const std::string &value)
{
	if (value.empty()) {
		return value;
	}
	std::string out = value;
	out[0] = static_cast<char>(toupper(out[0]));
	return out;
}

} // namespace

std::string format_time_string(bool use_24h, const std::string &)
{
	const auto now = std::chrono::system_clock::now();
	const std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm{};
#ifdef _WIN32
	localtime_s(&local_tm, &t);
#else
	localtime_r(&t, &local_tm);
#endif

	std::ostringstream oss;
	if (use_24h) {
		oss.width(2);
		oss.fill('0');
		oss << local_tm.tm_hour << ":";
		oss.width(2);
		oss.fill('0');
		oss << local_tm.tm_min;
	} else {
		int hour = local_tm.tm_hour % 12;
		if (hour == 0) {
			hour = 12;
		}
		oss << hour << ":";
		oss.width(2);
		oss.fill('0');
		oss << local_tm.tm_min;
		oss << (local_tm.tm_hour >= 12 ? " PM" : " AM");
	}
	return oss.str();
}

std::string format_day_string(const std::string &locale_tag)
{
	const auto now = std::chrono::system_clock::now();
	const std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm{};
#ifdef _WIN32
	localtime_s(&local_tm, &t);
#else
	localtime_r(&t, &local_tm);
#endif

	std::ostringstream oss;
	if (is_french_locale(locale_tag)) {
		oss << capitalize(weekday_fr(local_tm.tm_wday)) << " " << local_tm.tm_mday << " "
		    << month_fr(local_tm.tm_mon + 1);
	} else {
		oss << weekday_en(local_tm.tm_wday) << " " << month_en(local_tm.tm_mon + 1) << " "
		    << local_tm.tm_mday;
	}
	return oss.str();
}
