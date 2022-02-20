#if !defined(CONFIG_ITEM)
#define CONFIG_ITEM(type, name, def)
#endif

// This file gets included repeatedly with different definitions of the macro
// CONFIG_ITEM to suit the use case. Don't declare any variables here or do 
// anything that would make that break things!

// Config items are in the form:
// type
// variable name
// default value
// help string

// Simulation Precision Options
CONFIG_ITEM(uint32_t, run_time_mins, 60 * 24 * 365, "Time to run for in minutes")
CONFIG_ITEM(uint32_t, tick_size_mins, 10, "Size of a tick in minutes")

// Simulation Calibration options
CONFIG_ITEM(uint32_t, panel_target, 3500000, "Target output of a 4kW panel")
CONFIG_ITEM(uint32_t, base_load, 372, "Baseline load of the household")
CONFIG_ITEM(uint32_t, var_load, 592 * 2, "Peak of household variable load (i.e. peak load is base_load + var_load)")

// Electricity Pricing options
CONFIG_ITEM(float, price_per_kwh, 0.25f, "Price in pounds per kwh drawn from the grid")
CONFIG_ITEM(float, feed_in_per_kwh, 0.025f, "Price in pounds per kwh fed into the grid")


// Battery Options
CONFIG_ITEM(float, battery_start, 0.5f, "Starting capacity of the battery, e.g. 0.5 means it starts 50% full")
CONFIG_ITEM(float, battery_min_ratio, 0.1f, "Minimum capacity of the battery, e.g. 0.1 means it won't go below 10%")
CONFIG_ITEM(float, battery_sat_percentage, 0.95f, "The battery capacity above which it starts adding saturation effects")
CONFIG_ITEM(float, battery_clip_percentage, battery_min_ratio + 0.05f, "The battery capacity below which it starts adding clipping effects")
CONFIG_ITEM(float, battery_sat_loss, 0.1f, "The percentage of exported power due to saturation when saturated")
CONFIG_ITEM(float, battery_clip_loss, 0.1f, "The percentage of imported power due to saturation when clipping")

// Eco 7 options
CONFIG_ITEM(bool, eco_7_enable, false, "Whether or not to use eco7")
CONFIG_ITEM(bool, eco_7_charge, true, "Whether to use eco7 charging")
CONFIG_ITEM(float, eco_7_charge_scale, 1.0f, "How much more energy to draw overnight than the gap between peak and max")
CONFIG_ITEM(float, eco_7_price_kwh_night, 0.17f, "Price in pounds per kwh at night")
CONFIG_ITEM(float, eco_7_price_kwh_day, 0.33f, "Price in pounds per kwh during the day")
CONFIG_ITEM(uint32_t, eco_7_start_hr, 0, "24 hour clock hour of eco 7 start (inclusive)")
CONFIG_ITEM(uint32_t, eco_7_end_hr, 7, "24 hour clock hour of eco 7 end (exclusive)")


// System configuration ranges
CONFIG_ITEM(Range<uint32_t>, panel_ranges, { 0 }, "Range of panel sizes to try in watts, format start:stop:step")
CONFIG_ITEM(Range<uint32_t>, battery_ranges, { 0 }, "Range of battery sizes to try in watt hours, format start:stop:step")
CONFIG_ITEM(Range<float>, load_scale_ranges, { 0 }, "Range of load scales to try (multiplier for load), format start:stop:step")

// System configuration options
CONFIG_ITEM(std::vector<uint32_t>, panel_sizes, { 4000 }, "List of panel sizes to try in watts, comma separated list")
CONFIG_ITEM(std::vector<uint32_t>, battery_sizes, { 10000 }, "List of battery sizes to try in watt hours, comma separated list")
CONFIG_ITEM(std::vector<float>, load_scales, { 1.3f }, "List of load scales to try (multiplier for load), comma separated list")

// Simulation configuration files
CONFIG_ITEM(const char*, yearly_solar_csv_path, nullptr, "Location of yearly solar irridance csv")

// logging options
CONFIG_ITEM(bool, detailed_logs, false, "Create detailed logs")
