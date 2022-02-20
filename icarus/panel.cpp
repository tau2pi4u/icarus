#include "panel.hpp"

//#define M_PI (acos(-1.0))
#define M_PI 3.141592f

extern config::Config g_config;

std::array<std::array<float, 12>, solar_mode_count> solar_mode_scaling;
std::array<float, solar_mode_count> yearly_solar_mode_scaling;
uint32_t init_mask;

SolarPanel::SolarPanel(uint32_t peak_power) :
	peak_power(peak_power)
{
	m_fit_weights[0] = { 1.4870727f , -4.0657496f , 0.7630807f  , 0.44192672f };
	m_fit_weights[1] = { 1.3542665f , -1.913961f  , 0.50447077f , 0.33801028f };
	m_fit_weights[2] = { 1.5444306f , -1.441389f  , 0.6239798f  , 0.5002983f };
	m_fit_weights[3] = { 1.8185228f , -1.8586264f , 0.31005865f , 0.803121f };

	if (g_config.yearly_solar_csv_path) { LoadSolarWeights(); }

	ScaleCurves();
}

void SolarPanel::ScaleCurves()
{
	if (init_mask & (1 << static_cast<uint32_t>(mode))) { return; }

	static const uint32_t minutes_in_year = 60 * 24 * 365;
	static const uint32_t minutes_in_day = 60 * 24;

	// a 4kWHr system can be expected to produce 3500kWHr of energy
	static const float basis_peak_power = 4000.0f;
	static const float expected_yearly_total = static_cast<float>(g_config.panel_target)*1000.0f;

	auto const opp = peak_power;
	peak_power = static_cast<uint32_t>(basis_peak_power);

	auto& mode_scaling = solar_mode_scaling[static_cast<size_t>(mode)];
	auto& ysms = yearly_solar_mode_scaling[static_cast<size_t>(mode)];

	auto run_for_one_year = [&]()
	{
		uint32_t minute = 0;
		uint32_t tick_size = g_config.tick_size_mins;

		std::array<uint32_t, 12> month_totals = { 0 };
		while (minute < minutes_in_year)
		{
			uint32_t day = minute / minutes_in_day;
			uint32_t month = month_getter.get_month_from_day(day, 2022);
			month_totals[month] += generate(2022, day, minute, tick_size);
			minute += tick_size;
		}
		return month_totals;
	};

	// set them all to 1 for now so we get output
	for (auto& ms : mode_scaling)
	{
		ms = 1.0f;
	}
	ysms = 1.0f;

	auto const& month_totals_p0 = run_for_one_year();

	auto const& peak_scale = peak_power / basis_peak_power;

	for (uint32_t month = 0; month < 12; month++)
	{
		auto const& expected_output = m_curve[month];
		auto const& actual_output = month_totals_p0[month];

		mode_scaling[month] = (expected_output * 1000 * peak_scale) / actual_output;
	}

	auto const& month_totals_p1 = run_for_one_year();
	float sum = 0.0f;
	for (auto& m : month_totals_p1) { sum += m; }

	ysms *= expected_yearly_total / (sum * peak_scale);

	init_mask |= 1 << static_cast<uint32_t>(mode);

	peak_power = opp;
}

uint32_t SolarPanel::BaseCurve(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	uint32_t month = month_getter.get_month_from_day(day, year);

	auto const& m_scale = m_curve[month];

	float mode_scale = solar_mode_scaling[static_cast<size_t>(mode)][month] * yearly_solar_mode_scaling[static_cast<size_t>(mode)];

	return static_cast<uint32_t>(peak_power * h_curve[minute / 60] * mode_scale * m_scale * tick_size / 60.0f);
}

uint32_t SolarPanel::BaseCurveLerp(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	uint32_t month = month_getter.get_month_from_day(day, year);
	uint32_t month_len = month_getter.get_days_in_month(month, year);
	uint32_t day_in_month = month_getter.get_day_in_month_from_day(day, year);
	uint32_t mid_point = month_len / 2;

	float offset = static_cast<float>(month) + (static_cast<float>(day_in_month) / static_cast<float>(month_len));
	auto const& m_scale = lerp(m_curve, offset, sizeof(m_curve) / sizeof(float));
	auto const& mode_scale = lerp(solar_mode_scaling[static_cast<size_t>(mode)].data(), offset, solar_mode_scaling.size()) * yearly_solar_mode_scaling[static_cast<size_t>(mode)];


	// todo: fake_scale is a hack to get it to match closer to reality
	// find out what's actually wrong
	float fake_scale = 1.03f * 3500000.0f / 6396480.0f;

	return static_cast<uint32_t>(peak_power * h_curve[minute / 60] * mode_scale * m_scale * tick_size / 60.0f);
}

float SolarPanel::_ExpSinCurve(uint32_t idx, float hour)
{
	auto const& ws = m_fit_weights[idx];
	float tod_offset = 12.0f - abs(hour - ws[2]);
	float dist = M_PI * tod_offset / 24.0f;
	float sin_dists = sin(dist) * sin(dist);
	float exp_dists = ws[0] * exp(sin_dists * ws[1]) + ws[3];

	return std::max(0.0f, exp_dists);
}

uint32_t SolarPanel::ExpSinCurve(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	float year_progress = static_cast<float>(day) / (365.0f + !!(month_getter.is_leap_year(year)));
	float hour = static_cast<float>(minute % 1440) / 60;

	auto in_range = [year_progress](float a, float b)
	{
		return year_progress >= a && year_progress < b;
	};

	year_progress = (year_progress + (1.0f / 12.0f));
	year_progress = year_progress >= 1.0f ? year_progress - 1.0f : year_progress;
	float season = year_progress * 4.0f;

	float mode_scale = solar_mode_scaling[static_cast<size_t>(mode)][month_getter.get_month_from_day(day, year)] * yearly_solar_mode_scaling[static_cast<size_t>(mode)];


	return static_cast<uint32_t>(_ExpSinCurve(static_cast<uint32_t>(season), hour) * mode_scale);
}

uint32_t SolarPanel::ExpSinCurveLerp(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	float year_progress = static_cast<float>(day) / (365.0f + !!(month_getter.is_leap_year(year)));
	float hour = static_cast<float>(minute % 1440) / 60;

	auto frac = [](float f)
	{
		float tmp;
		return std::modf(f, &tmp);
	};

	auto in_range = [year_progress](float a, float b)
	{
		return year_progress >= a && year_progress < b;
	};

	year_progress = (year_progress + (1.0f / 12.0f));
	year_progress = year_progress >= 1.0f ? year_progress - 1.0f : year_progress;
	float season_a = year_progress * 4.0f;
	float f = frac(season_a);

	float season_b = f > 0.5 ? season_a + 1.0f : season_a - 1.0f;
	season_b = season_b < 0.0f ? season_b + 4.0f : season_b;
	season_b = season_b >= 4.0f ? season_b - 4.0f : season_b;

	float w_a = 1.0f - std::abs(0.5f - f);
	float w_b = 1.0f - w_a;


	float w = (w_a * _ExpSinCurve(static_cast<uint32_t>(season_a), hour)) +
		(w_b * _ExpSinCurve(static_cast<uint32_t>(season_b), hour));

	uint32_t month = month_getter.get_month_from_day(day, year);
	uint32_t month_len = month_getter.get_days_in_month(month, year);
	uint32_t day_in_month = month_getter.get_day_in_month_from_day(day, year);
	uint32_t mid_point = month_len / 2;

	float offset = static_cast<float>(month) + (static_cast<float>(day_in_month) / static_cast<float>(month_len));
	float mode_scale = lerp(solar_mode_scaling[static_cast<size_t>(mode)].data(), offset, solar_mode_scaling.size()) * yearly_solar_mode_scaling[static_cast<size_t>(mode)];

	return static_cast<uint32_t>(round(w * peak_power * mode_scale * tick_size / 60.0f));
}

uint32_t SolarPanel::generate(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	uint32_t base_power = base_generate(year, day, minute, tick_size);

	if (!HasSolarWeights()) { return base_power; }

	uint32_t adjday = month_getter.is_leap_year(year) && day > month_getter.get_days_in_month(0, year) + month_getter.get_days_in_month(1, year) ?
		day - 1 : day;
	uint32_t hour = (minute % 1440) / 60;

	auto const& solar_weights = LoadSolarWeights();
	auto const& weight = solar_weights[adjday][hour];

	return static_cast<uint32_t>(static_cast<float>(base_power) * weight);
}

uint32_t SolarPanel::base_generate(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size)
{
	switch (mode)
	{
	case SolarMode::BaseCurve:
		return BaseCurve(year, day, minute, tick_size);
	case SolarMode::BaseCurveLerp:
		return BaseCurveLerp(year, day, minute, tick_size);
	case SolarMode::ExpSinCurve:
		return ExpSinCurve(year, day, minute, tick_size);
	case SolarMode::ExpSinCurveLerp:
		return ExpSinCurveLerp(year, day, minute, tick_size);
	}

	_assume(0);
}

std::vector<std::array<float, 24>> const& SolarPanel::LoadSolarWeights()
{
	static std::vector<std::array<float, 24>> weights;
	weights.reserve(365);
	static char buf[1024];

	if (!g_config.yearly_solar_csv_path || !weights.empty()) { return weights; }

	FILE* solarcsv = nullptr;
	fopen_s(&solarcsv, g_config.yearly_solar_csv_path, "r");
	if (!solarcsv)
	{
		printf("Failed to load yearly solar csv!\n");
	}

	size_t day = 0;
	while (fgets(buf, 1024, solarcsv))
	{
		weights.resize(weights.size() + 1);

		char* ctx = nullptr;
		const char* tok = strtok_s(buf, ",", &ctx);
		size_t hr = 0;
		while (tok && *tok)
		{
			sscanf_s(tok, "%f", &weights[day][hr++]);
			tok = strtok_s(nullptr, ",", &ctx);
		}

		++day;
	}

	return weights;
}
