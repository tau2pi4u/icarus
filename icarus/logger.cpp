#include "logger.hpp"

extern config::Config g_config;

FILE* register_file = nullptr;
FILE* headline_file = nullptr;

Logger::Logger(uint32_t id) :
	id(id),
	log_path{ 0 },
	detailed_logs(g_config.detailed_logs)
{
	// todo: make detailed logs a config option
	// todo: consider storing detailed logs in string and dumping at the end
	// more memory usage but much faster than repeatedly opening and closing files
	if (detailed_logs)
	{
		FILE* logfile = nullptr;
		sprintf_s(log_path, 256, "logs/icarus_log_%u.csv", id);
		fopen_s(&logfile, log_path, "w");
		WriteHeader(logfile);
		fclose(logfile);
	}
}

Logger::~Logger()
{
}

// Register new option combo to icarus_register.csv
void Logger::Register(uint32_t panel_size, uint32_t battery_size, float load_scale, bool minutely_profile)
{
	const char* path = "icarus_register.csv";
	// If nothing else has opened it before (this is the first logger) then open it
	if (!register_file)
	{
		fopen_s(&register_file, path, "w");
		if (!register_file) { return; }

		// Write csv header
		fprintf_s(register_file, "Id, Panel Size (W), Battery Size (WHr), Load Scale, Minutely Profile\n");
	}
	fprintf_s(register_file, "%u, %u, %u, %.02f, %s\n", id, panel_size, battery_size, load_scale, minutely_profile ? "True" : "False");
}

// Headline file with key data from each combo
void Logger::WriteHeadlines(uint32_t panel_size, uint32_t battery_min, uint32_t battery_size, float load_scale, bool minutely_profile)
{
	const char* path = "icarus_headlines.csv";
	
	// If nothing else has opened it before (this is the first logger) then open it
	if (!headline_file)
	{
		auto error = fopen_s(&headline_file, path, "w");
		if (!headline_file) { return; }

		// Write the csv header, uses a macro to get the names of the variables which should be included
		fprintf_s(headline_file, "Id,Panel Size (W),Battery Min (WHr),Battery Size (WHr),Load Scale,Minutely Profile");
#define LOG_ITEM(var, name, headline) { if (headline) { fprintf_s(headline_file, ",%s", name); } }
#include "log_data.hpp"
#undef LOG_ITEM
		// Adds in the calculated variables after
		if (g_config.eco_7_enable)
		{
			fprintf_s(headline_file, ",No Solar Flat,No Solar Eco7,Flat,Eco7,Feed In,Battery Val,Net\n");
		}
		else
		{
			fprintf_s(headline_file, ",No Solar Flat,Flat,Feed In,Battery Val,Net\n");
		}
		
	}

	// Print the config variables
	fprintf_s(headline_file, "%u, %u, %u, %u, %.02f, %s", id, panel_size, battery_min, battery_size, load_scale, minutely_profile ? "True" : "False");

	// Use LOG_ITEM to print the data for the headline variables
#define LOG_ITEM(var, name, headline) { if(headline) { fprintf_s(headline_file, ",%u", var); } }
#include "log_data.hpp"
#undef LOG_ITEM


	float flat_charge_nosolar = [&]
	{
		return (total_demand + total_demand_eco7 + total_eco7_charge_demand) * g_config.price_per_kwh / 1000.0f;
	}();

	float eco7_charge_nosolar = [&]
	{
		return ((total_demand * g_config.eco_7_price_kwh_day) +
			((total_demand_eco7 + total_eco7_charge_demand) * g_config.eco_7_price_kwh_night)) / 1000.0f;
	}();

	float flat_charge = [&]
	{
		return (total_demand_from_grid + total_demand_from_grid_eco7 + total_eco7_charge_demand) * g_config.price_per_kwh / 1000.0f;
	}();

	float eco7_charge = [&]
	{
		return ((total_demand_from_grid * g_config.eco_7_price_kwh_day) + ((total_eco7_charge_demand + total_demand_from_grid_eco7) * g_config.eco_7_price_kwh_night)) / 1000.0f;
	}();

	float feed_in = static_cast<float>(total_supply_to_grid) * g_config.feed_in_per_kwh / 1000.0f;
	float battery_val = static_cast<float>(
		static_cast<int>(current_battery_state) - static_cast<int>(battery_size / 2)) * 
		(g_config.eco_7_enable ? g_config.eco_7_price_kwh_night : g_config.price_per_kwh) / 1000.0f;

	if (g_config.eco_7_enable)
	{
		fprintf_s(headline_file, ",%.02f,%.02f,%.02f,%.02f,%.02f,%.02f,%.02f\n",
			flat_charge_nosolar,
			eco7_charge_nosolar,
			flat_charge,
			eco7_charge,
			feed_in,
			battery_val,
			feed_in + battery_val + eco7_charge
		);
	}
	else
	{
		fprintf_s(headline_file, ",%.02f,%.02f,%.02f,%.02f,%.02f\n",
			flat_charge_nosolar,
			flat_charge,
			feed_in,
			battery_val,
			feed_in + battery_val + flat_charge
		);
	}
}

// Logging functions
// Used to log different variables
// todo: move this to macro?

void Logger::LogTime(uint32_t simtime)
{
	WriteLine();
	current_time = simtime;
}

void Logger::LogTotalDemand(uint32_t demand, bool eco7)
{
	if (!eco7)
	{
		current_demand = demand;
		total_demand += demand;
		
		current_demand_eco7 = 0u;
	}
	else
	{
		current_demand_eco7 = demand;
		total_demand_eco7 += demand;

		current_demand = 0u;
	}
}

void Logger::LogTotalSupply(uint32_t supply)
{

	current_supply = supply;
	total_supply += supply;
}

void Logger::LogEco7Demand(uint32_t eco7Demand)
{
	total_eco7_charge_demand += eco7Demand; // to avoid pricing this twice, we remove it from the total demand
}

void Logger::LogNetDemand(uint32_t demand, bool eco7)
{
	if (!eco7)
	{
		current_demand_from_grid = demand;
		total_demand_from_grid += demand;
	}
	else
	{
		current_demand_from_grid_eco7 = demand;
		total_demand_from_grid_eco7 += demand;
	}

}

void Logger::LogNetSupply(uint32_t supply)
{
	current_supply_to_grid = supply;
	total_supply_to_grid += supply;
}

void Logger::LogBatteryState(uint32_t state)
{
	current_battery_state = state;
}

// Writes the header for detailed stats
void Logger::WriteHeader(FILE * logfile)
{
#define LOG_ITEM(var, name, headline) fprintf_s(logfile, "%s,", name);
#include "log_data.hpp"
#undef LOG_ITEM
	fprintf_s(logfile, "\n");
}

// Writes the data for detailed stats - opens and closes file
// because it's easy to hit the file limit so is very slow
void Logger::WriteLine()
{
	if (!detailed_logs) { return; }
	FILE* logfile = nullptr;
	fopen_s(&logfile, log_path, "a");

#define LOG_ITEM(var, name, headline) fprintf_s(logfile, "%u,", var);
#include "log_data.hpp"
#undef LOG_ITEM
	fprintf_s(logfile, "\n");
	fclose(logfile);
}
