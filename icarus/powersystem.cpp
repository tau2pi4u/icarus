#include "powersystem.hpp"

PowerSystem::PowerSystem(
	uint32_t id, 
	uint32_t tick, 
	uint32_t panel_peak, 
	uint32_t bat_max, 
	uint32_t base_load, 
	uint32_t var_load, 
	float load_scale, 
	float* h_profile, 
	float* m_profile, 
	float* y_profile,
	bool use_minutely_profile) :
	panel(panel_peak),
	battery(bat_max),
	load(base_load, var_load, load_scale, h_profile, m_profile, y_profile, use_minutely_profile),
	clock(0),
	tick_size(tick),
	logger(id)
{
	// Register the new system to icarus_register.csv
	logger.Register(panel.peak_power, bat_max, load.load_scale, load.minutely_profile);
}

// Runs one tick of time, calculating solar production, power demand from the load and 
// battery interactions.
void PowerSystem::advance()
{
	// Start date is 2022 (only really matters for leap years)
	// todo: make config option
	static const uint32_t base_year = 2022;

	// Advance the clock
	clock += tick_size;

	// Convert the clock in minutes into the minute in day
	auto const& minute = [&]
	{
		return clock % (60 * 24);
	}();

	// Convert the clock in minutes into day of year
	auto day = [&]
	{
		return (clock / (60 * 24));
	}();

	// Convert the day into the day of the month and the overall year
	auto const& year = [&]
	{
		uint32_t year = 0;
		while (day >= 365)
		{
			day -= month_getter.is_leap_year(base_year + (year++)) ? 366 : 365;
		}
		return year;
	}();

	uint32_t hour = (minute % 1440) / 60;
	bool eco_7_pricing = g_config.eco_7_enable && hour >= g_config.eco_7_start_hr && hour < g_config.eco_7_end_hr;

	// Get the supply and demand for energy for this tick (WHr)
	uint32_t demand = load.load(year, day, minute, tick_size);
	uint32_t supply = panel.generate(year, day, minute, tick_size);

	// Log the time + initial supply and demand
	logger.LogTime(clock);
	logger.LogTotalDemand(demand, eco_7_pricing);
	logger.LogTotalSupply(supply);

	// at eco 7 pricing, just take energy from the grid
	if (eco_7_pricing)
	{
		supply = battery.fill(supply);
	}
	else
	{
		// If we're in surplus, fill the battery
		if (supply > demand)
		{
			// battery.fill returns the amount of energy (WHr) it was unable to store
			// supply will be leftover electricity to be fed into the grid
			supply = battery.fill(supply - demand);
			demand = 0;
		}
		else
		{
			// battery.request returns the amount of energy (WHr) it was unable to supply
			// demand will  be the leftover electricity to be sourced from the grid
			demand = battery.request(demand - supply);
			supply = 0;
		}
	}

	if (g_config.eco_7_enable)
	{
		if (g_config.eco_7_charge)
		{
			uint32_t eco7demand = battery.Eco7Charge(clock);
			logger.LogEco7Demand(eco7demand);
		}
	}

	// Log the battery state and net grid supply/demand
	logger.LogBatteryState(battery.CurrentCapacity());
	logger.LogNetDemand(demand, eco_7_pricing);
	logger.LogNetSupply(supply);
}

// When complete, write the final state out
void PowerSystem::finish()
{
	logger.WriteHeadlines(panel.peak_power, battery.MinimumCapacity(), battery.MaximumCapacity(), load.load_scale, load.minutely_profile);
}
