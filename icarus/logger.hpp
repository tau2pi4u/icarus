#pragma once
#include <stdio.h>
#include "util.hpp"
#include "config.hpp"

struct Logger
{
	Logger(uint32_t id);
	~Logger();
	void Register(uint32_t panel_size, uint32_t battery_size, float load_scale, bool minutely_profile);
	void WriteHeadlines(uint32_t panel_size, uint32_t battery_min, uint32_t battery_size, float load_scale, bool minutely_profile);
	void LogTime(uint32_t simtime);
	void LogTotalDemand(uint32_t demand, bool eco7);
	void LogTotalSupply(uint32_t supply);
	void LogNetDemand(uint32_t demand, bool eco7);
	void LogNetSupply(uint32_t supply);
	void LogBatteryState(uint32_t state);

	void WriteHeader(FILE * logfile);
	void WriteLine();
	void LogEco7Demand(uint32_t eco7Demand);

#define LOG_ITEM(var, name, headline) uint32_t var = 0;
#include "log_data.hpp"
#undef LOG_ITEM

	char log_path[256];
	uint32_t id;
	bool detailed_logs = false;
};