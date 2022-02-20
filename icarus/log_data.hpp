#if !defined(LOG_ITEM)
#define LOG_ITEM(var, name, headline)
#endif

// This file gets included repeatedly with different definitions of the macro
// LOG_ITEM to suit the use case. Don't declare any variables here or do 
// anything that would make that break things!

// Log items are in the form:
// variable name
// pretty name (for printing)
// whether or not it should be included in the headline results table

LOG_ITEM(current_time, "Current Time", true)
LOG_ITEM(current_demand, "Current Demand", false)
LOG_ITEM(current_demand_eco7, "Current Demand Eco7", false)
LOG_ITEM(current_supply, "Current Supply", false)
LOG_ITEM(current_demand_from_grid, "Current Demand from Grid", false)
LOG_ITEM(current_demand_from_grid_eco7, "Current Demand from Grid Eco7", false)
LOG_ITEM(current_supply_to_grid, "Current Supply to Grid", false)
LOG_ITEM(current_battery_state, "Current Battery State", true)
LOG_ITEM(total_demand, "Total Demand", true)
LOG_ITEM(total_demand_eco7, "Total Demand Eco7", g_config.eco_7_enable)
LOG_ITEM(total_supply, "Total Supply", true)
LOG_ITEM(total_demand_from_grid, "Total Demand from Grid", true)
LOG_ITEM(total_supply_to_grid, "Total Supply to Grid", true)
LOG_ITEM(total_demand_from_grid_eco7, "Total Demand from Grid Eco7", g_config.eco_7_enable)
LOG_ITEM(total_eco7_charge_demand, "Total demand from grid for eco7 charging", g_config.eco_7_enable)