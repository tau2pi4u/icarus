#include "load.hpp"

namespace load
{
	// Default profiles based on usage from the smart meter
	// Used to scale the variable load - i.e. load is base_load + (scaling * var_load)
	// todo: add support for non default profiles
	namespace defaults
	{
		static const float default_hourly_profile[24] = {
		0.374f,
		0.257f,
		0.135f,
		0.013f,
		0.007f,
		0.000f,
		0.060f,
		0.121f,
		0.170f,
		0.220f,
		0.323f,
		0.426f,
		0.492f,
		0.558f,
		0.576f,
		0.594f,
		0.696f,
		0.797f,
		0.899f,
		1.000f,
		0.757f,
		0.515f,
		0.502f,
		0.490f
		};

		static const float default_monthly_profile[12] = {
				0.887f,
				1.008f,
				0.664f,
				0.686f,
				0.621f, // May looks like an outlier so filled in with June (was 0.448f)
				0.621f,
				0.656f,
				0.695f,
				0.622f,
				0.625f,
				0.866f,
				1.039f
		};
	}
}


Load::Load(uint32_t base_power, uint32_t variable_power, float load_scale, float* p_hourly_profile, float* p_monthly_profile, float* p_yearly_profile, bool use_minutely_profile) :
	base_power(base_power),
	variable_power(variable_power),
	load_scale(load_scale)
{
	auto load_from_ptr = [&](float* dst, const float* src, const float* def, size_t size)
	{
		if (src)
		{
			std::memcpy(dst, src, size);
		}
		else
		{
			std::memcpy(dst, def, size);
		}
	};

	// Fill in array of 1s as default for yearly profile
	// Assumes power usage constant over time
	static std::array<float, sizeof(yearly_profile) / sizeof(float)> yearly_defaults;
	if (yearly_defaults[0] != 1.0f)
	{
		std::fill(yearly_defaults.begin(), yearly_defaults.end(), 1.0f);
	}

	// Load profiles
	load_from_ptr(hourly_profile, p_hourly_profile, load::defaults::default_hourly_profile, sizeof(hourly_profile));
	load_from_ptr(monthly_profile, p_monthly_profile, load::defaults::default_monthly_profile, sizeof(monthly_profile));
	load_from_ptr(yearly_profile, p_yearly_profile, yearly_defaults.data(), sizeof(yearly_profile));

	if (use_minutely_profile)
	{
		// todo: make config option
		get_minutely_profile("D:\\source\\meter_reader\\minutely.csv");
	}
}

// Load the minutely profile 
void Load::get_minutely_profile(const char* path)
{
	size_t i = 0;
	FILE* mf = nullptr;
	fopen_s(&mf, path, "r");

	static const size_t buf_size = 2048;
	static const size_t minute_count = 60 * 24;
	char buf[buf_size];

	if (!mf) { return; }

	minutely_profile = static_cast<float*>(malloc(minute_count * sizeof(float)));
	if (!minutely_profile) { return; }

	// File is csv with one datapoint per line
	while (fgets(buf, buf_size, mf) && i < minute_count)
	{
		float f;
		sscanf_s(buf, "%f", &f);
		minutely_profile[i++] = f;
	}

	fclose(mf);
}

// Get the current load at the given time for the given time period
uint32_t Load::load(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	// Scale for the hour of the day
	auto const& hour_mod = [&]{
		if (minutely_profile)
		{
			return minutely_profile[minute % 1440];
		}

		return lerp(hourly_profile, minute / 60.0f, sizeof(hourly_profile) / sizeof(float));
	}();

	// Scale for the month of the year
	auto const& month_mod = [&] {
		auto const& month = month_getter.get_month_from_day(day, year);
		auto const& day_in_month = month_getter.get_day_in_month_from_day(day, year);
		auto const& month_len = month_getter.get_days_in_month(month, year);
		float idx = static_cast<float>(month) + (static_cast<float>(day_in_month) / static_cast<float>(month_len));

		return lerp(monthly_profile, idx, sizeof(monthly_profile) / sizeof(float));
	}();

	// Scale for the year
	auto const& year_mod = [&] {
		return yearly_profile[year];
	}();

	uint32_t var_power = static_cast<uint32_t>(variable_power * hour_mod * month_mod * year_mod);

	// Scale watts to watt hours using tick_size / 60.0f i.e. minutes / minutes in hour
	return static_cast<uint32_t>((base_power + var_power) * load_scale * tick_size / 60.0f);
}
