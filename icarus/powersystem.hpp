#pragma once

#include "load.hpp"
#include "panel.hpp"
#include "battery.hpp"
#include "logger.hpp"

struct PowerSystem
{
	PowerSystem(
		uint32_t id,
		uint32_t tick,
		uint32_t panel_peak, 
		uint32_t bat_max, 
		uint32_t base_load, 
		uint32_t var_load,
		float load_scale,
		float* h_profile = nullptr, 
		float* m_profile = nullptr, 
		float* y_profile = nullptr,
		bool use_minutely_profile = true);

	void advance();
	void finish();

	Logger logger;
	Battery battery;
	Load load;
	SolarPanel panel;
	uint32_t clock;
	uint32_t tick_size;
};