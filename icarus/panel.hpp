#pragma once

#include <cmath>
#include <array>
#include "util.hpp"
#include "config.hpp"


enum class SolarMode : size_t
{
	BaseCurve = 0,
	BaseCurveLerp,
	ExpSinCurve,
	ExpSinCurveLerp,
	Count
};

static const size_t solar_mode_count = static_cast<const size_t>(SolarMode::Count);

extern std::array<std::array<float, 12>, solar_mode_count> solar_mode_scaling;
extern uint32_t init_mask;

struct SolarPanel
{
	SolarPanel(uint32_t peak_power);

	SolarMode mode = SolarMode::ExpSinCurveLerp;

	void ScaleCurves();

	uint32_t BaseCurve(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	uint32_t BaseCurveLerp(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	float _ExpSinCurve(uint32_t idx, float hour);
	uint32_t ExpSinCurve(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	uint32_t ExpSinCurveLerp(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	uint32_t generate(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	uint32_t base_generate(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size);
	bool HasSolarWeights() { return !LoadSolarWeights().empty(); }
	std::vector<std::array<float, 24>> const& LoadSolarWeights();

	float h_curve[24] = {
		0.0f, // 0am
		0.0f, // 1am
		0.0f, // 2am
		0.0f, // 3am
		0.0f, // 4am
		0.0f, // 5am
		0.0f, // 6am
		0.1f, // 7am
		190.0f / 500.0f, //8am
		0.6f, // 9am
		0.85f, // 10am
		0.9f, // 11am
		1.0f, // 12am
		1.0f, // 1pm
		0.9f, // 2pm
		0.85f, // 3pm
		0.6f, // 4pm
		190.0f / 500.0f, // 5pm
		0.1f, // 6pm
		0.0f, // 7pm
		0.0f, // 8pm
		0.0f, // 9pm
		0.0f, // 10pm
		0.0f  // 11pm
	};

	// div 1400
	float m_curve[12] = {
		105.0f,
		158.0f,
		308.0f,
		385.0f,
		420.0f,
		473.0f,
		490.0f,
		403.0f,
		315.0f,
		210.0f,
		147.0f,
		88.0f
	};

	std::array<std::array<float, 4>, 4> m_fit_weights;
	

	uint32_t peak_power;
};