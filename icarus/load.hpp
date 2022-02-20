#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <array>
#include "util.hpp"

struct Load
{
	Load(uint32_t base_power, uint32_t variable_power, float load_scale, float* p_hourly_profile, float* p_monthly_profile, float* p_yearly_profile, bool use_minutely_profile);

	void get_minutely_profile(const char* path); // load the minutely profile from path
	uint32_t load(uint32_t year, uint32_t day, uint32_t minute, uint32_t tick_size); // get the current load at the given time for the given time period

	uint32_t base_power; // baseload power 
	uint32_t variable_power; // variable power
	float load_scale; // scaling of variable power
	float hourly_profile[24]; // profile of variable power by hour
	float monthly_profile[12]; // profile of variable power by month
	float yearly_profile[50]; // profile of variable power by year
	float* minutely_profile = nullptr; // minutely power profile, todo: only load this once
};