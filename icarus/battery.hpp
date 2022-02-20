#pragma once
#include <numeric>
#include <array>
#include "util.hpp"
#include "config.hpp"

extern config::Config g_config;

struct Battery
{
	Battery(uint32_t max_capacity);

	uint32_t fill(uint32_t whr);
	uint32_t request(uint32_t whr);
	uint32_t Eco7Charge(uint32_t minute);
	uint32_t GetPreviousExcess();
	uint32_t CurrentCapacity() { return current_capacity; }
	uint32_t MinimumCapacity() { return min_capacity; }
	uint32_t MaximumCapacity() { return max_capacity; }

private:
	template<size_t S>
	struct peak_log
	{
		std::array<uint32_t, S> peaks = { 0 };
		size_t valid_count = 0u;
		size_t loc = 0u;

		uint32_t get_max()
		{
			return *std::max_element(peaks.begin(), peaks.begin() + valid_count);
		}

		uint32_t mean()
		{
			uint32_t sum = 0u;
			std::accumulate(peaks.begin(), peaks.begin() + valid_count - 1, sum);
			return sum / valid_count;
		}

		// not quite accurate if S is a multiple of 2 but quicker
		uint32_t median()
		{
			std::array<uint32_t, S> cpy = peaks;
			std::sort(cpy.begin(), cpy.begin() + valid_count);
			return cpy[valid_count / 2];
		}

		void insert(uint32_t val)
		{
			peaks[loc] = val;
			valid_count = std::min(valid_count + 1, S);
			loc = ++loc % S;
		}
	};

	uint32_t _Eco7Charge();

	uint32_t max_capacity;
	uint32_t current_capacity;
	uint32_t min_capacity;

	uint32_t day = 0u;
	uint32_t prev_day = ~0u;
	uint32_t peak;
	uint32_t prev_eco7_charge = 0u;
	peak_log<10> peak_log;
	bool eco_7_charge_today = false;
};