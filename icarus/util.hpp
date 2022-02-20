#pragma once
#include <stdint.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>

#define UNIQUIFY(vec) { \
 std::sort( vec.begin(), vec.end() ); \
 vec.erase(unique(vec.begin(), vec.end()), vec.end()); \
}

static struct MonthGetter
{
	MonthGetter()
	{
		uint32_t base = 0;
		for (uint32_t month = 0; month < 12; ++month)
		{
			uint32_t month_len = get_days_in_month(month, 2022);
			for (uint32_t day = 0; day < month_len; ++day)
			{
				day_to_month_lookup[base + day] = month;
				day_in_month_lookup[base + day] = day;
			}
			base += month_len;
		}
	}

	bool is_leap_year(uint32_t year)
	{
		return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
	}

	uint32_t get_days_in_month(uint32_t month, uint32_t year)
	{
		if (is_leap_year(year) && month == 2) { return 29; }
		return days_in_month_lookup[month];
	}

	uint32_t get_month_from_day(uint32_t day, uint32_t year)
	{
		if (is_leap_year(year) && day >= days_in_month_lookup[0] + days_in_month_lookup[1]) { --day; }
		return day_to_month_lookup[day];
	};

	uint32_t get_day_in_month_from_day(uint32_t day, uint32_t year)
	{
		if (is_leap_year(year) && day == days_in_month_lookup[0] + days_in_month_lookup[1]) { return 28; }
		return day_to_month_lookup[day];
	}

	uint32_t day_to_month_lookup[365];
	uint32_t day_in_month_lookup[365];
	const uint32_t days_in_month_lookup[12] = {
			31, // Jan
			28, // Feb
			31, // Mar
			30, // Apr
			31, // May
			30, // Jun
			31, // Jul
			31, // Aug
			30, // Sep
			31, // Oct
			30, // Nov
			31  // Dec
	};;
} month_getter;

template<typename T>
T lerp(T* arr, float pos, size_t size)
{
	float f = floor(pos);
	float c = ceil(pos);

	float w_f = std::abs(pos - c);
	float w_c = 1.0f - w_f;

	return static_cast<T>((w_f * arr[static_cast<size_t>(f)]) + (w_c * arr[static_cast<size_t>(c < size ? c : 0)]));
}