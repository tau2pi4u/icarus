// icarus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include "powersystem.hpp"
#include "config.hpp"

extern config::Config g_config;

int main(int argc, char* argv[])
{
    bool help_requested = false;
    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-h"))
        {
            help_requested = true;
        }
    }

    g_config.print_help(help_requested, true);

    g_config.load("icarus.ini");

    uint32_t tick_size = g_config.tick_size_mins;
    uint32_t total_ticks = g_config.run_time_mins / tick_size;

    g_config.panel_ranges.get(g_config.panel_sizes);
    g_config.battery_ranges.get(g_config.battery_sizes);
    g_config.load_scale_ranges.get(g_config.load_scales);

    UNIQUIFY(g_config.panel_sizes);
    UNIQUIFY(g_config.battery_sizes);
    UNIQUIFY(g_config.load_scales);

    uint32_t id = 0;
    std::vector<PowerSystem*> aPs;

    aPs.resize(g_config.panel_sizes.size() * g_config.battery_sizes.size() * g_config.load_scales.size());

    for (auto const& panel_size : g_config.panel_sizes)
    {
        for (auto const& battery_size : g_config.battery_sizes)
        {
            for (auto const& load_scale : g_config.load_scales)
            {
                aPs[id++] = new PowerSystem(id, tick_size, panel_size, battery_size, g_config.base_load, g_config.var_load, load_scale);
            }
        }
    }

    printf("Ticks left: %5u", total_ticks);
    while (--total_ticks)
    {
        if (total_ticks % 1024 == 0) { printf("\rTicks left: %5u", total_ticks); }
        for(uint32_t i = 0; i < aPs.size(); i++)
        {
            aPs[i]->advance();
        }
    }
    for (uint32_t i = 0; i < aPs.size(); i++)
    {
        aPs[i]->finish();
    }
    printf("\rTicks left: %5u", total_ticks);

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
