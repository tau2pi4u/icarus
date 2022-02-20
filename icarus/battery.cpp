#include "battery.hpp"

Battery::Battery(uint32_t max_capacity) :
	max_capacity(max_capacity),
	min_capacity(static_cast<uint32_t>(static_cast<float>(max_capacity)* g_config.battery_min_ratio)),
	current_capacity(static_cast<uint32_t>(static_cast<float>(max_capacity)* g_config.battery_start)),
	peak(max_capacity)
{}

uint32_t Battery::fill(uint32_t whr)
{
	// work out how much will be in the battery after filling and how much overflows
	uint32_t post_fill = std::min(current_capacity + whr, max_capacity);
	uint32_t overflow = post_fill == max_capacity ? current_capacity + whr - max_capacity : 0u;

	auto post_fill_perc = static_cast<float>(post_fill) / max_capacity;

	// add in saturation effects
	// the idea is to model the inefficiencies around full charge where it may not be able
	// to perfectly smooth moment to moment
	if (post_fill_perc > g_config.battery_sat_percentage)
	{
		// Work out how much may have been lost to saturation
		float loss_rate = (post_fill_perc - g_config.battery_sat_percentage) * g_config.battery_sat_loss;
		uint32_t excess = static_cast<uint32_t>(loss_rate * (whr));

		// Move that excess from the battery to grid output
		post_fill -= excess;
		overflow += excess;
	}

	current_capacity = post_fill;
	return overflow;
}

uint32_t Battery::request(uint32_t whr)
{
	// slightly more complicated than fill because of underflows
	// underflow is the amount that can't be supplied by the battery
	uint32_t underflow = whr > current_capacity ? whr - current_capacity : 0u;
	
	// whr <= current_capacity
	whr -= underflow;

	uint32_t post_drain = std::max(current_capacity - whr, min_capacity);


	// if post drain == min capacity then we have some underflow to add on
	// min_capacity >= current_capacity - whr >= 0
	// min_capacity - (current_capacity - whr) >= 0
	underflow += post_drain == min_capacity ? min_capacity - (current_capacity - whr) : 0u;

	// battery percentage post drain
	auto post_drain_perc = static_cast<float>(post_drain) / max_capacity;

	// add in clipping effects if we're below the clip percentage
	if (post_drain_perc < g_config.battery_clip_percentage)
	{
		float loss_rate = (g_config.battery_clip_percentage - post_drain_perc) * g_config.battery_clip_loss;
		uint32_t excess = static_cast<uint32_t>(loss_rate * whr);

		// pass the excess back into the battery and add it to the grid demand
		post_drain += excess;
		underflow += excess;
	}

	current_capacity = post_drain;
	return underflow;
}

uint32_t Battery::Eco7Charge(uint32_t minute)
{
	uint32_t cday = minute / 1440; // get day of sim
	if (cday == day)
	{
		peak = std::max(current_capacity, peak);
	}
	else
	{
		peak_log.insert(peak - prev_eco7_charge);
		prev_day = day;
		day = cday;
		peak = current_capacity;
		eco_7_charge_today = false;
	}

	uint32_t hour = (minute % 1440) / 60;

	if (!eco_7_charge_today && hour >= g_config.eco_7_start_hr && hour < g_config.eco_7_end_hr)
	{
		return static_cast<uint32_t>(_Eco7Charge());
	}
	return 0u;
}

uint32_t Battery::GetPreviousExcess()
{
	uint32_t target_charge = static_cast<uint32_t>(static_cast<float>(max_capacity - peak_log.get_max()) * g_config.eco_7_charge_scale);
	return max_capacity - current_capacity; //std::min(max_capacity - current_capacity, target_charge);
}

uint32_t Battery::_Eco7Charge()
{
	eco_7_charge_today = true;
	prev_eco7_charge = GetPreviousExcess();
	current_capacity += prev_eco7_charge;
	return prev_eco7_charge;
}
