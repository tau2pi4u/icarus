#include "config.hpp"

namespace config
{
	bool _get_helper(const char* line, uint32_t& t)
	{
		return sscanf_s(line, "%u", &t);
	}

	bool _get_helper(const char* line, float& t)
	{
		return sscanf_s(line, "%f", &t);
	}

	bool _get_helper(const char* line, bool& t)
	{
		uint32_t tmp;
		auto const& res = sscanf_s(line, "%u", &tmp);
		t = tmp != 0;
		return res;
	}

	bool _get_helper(const char* line, const char*& t)
	{
		static char buf[256];
		auto const& res = sscanf_s(line, "%s", buf, 256);
		
		char * cpy = (char*) malloc(strlen(buf) + 1);
		if (!cpy) { return false; }
		strcpy_s(cpy, strlen(buf) + 1, buf);

		t = static_cast<const char*>(cpy);
		return res;
	}

	// Load range from string using strtok
	template<typename T>
	bool _get_helper(const char* line, Range<T>& t)
	{
		// Buffer for local copy of string - strok_s modifies it
		static char buf[256];
		size_t count = 0;

		strcpy_s(buf, 256, line);
		char* ctx = nullptr;

		// Expected format is start:end:step
		const char* tok = strtok_s(buf, ":", &ctx);
		count += _get_helper(tok, t.begin);

		tok = strtok_s(NULL, ":", &ctx);
		count += _get_helper(tok, t.end);

		tok = strtok_s(NULL, ":", &ctx);
		count += _get_helper(tok, t.step);

		return count == 3;
	}

	// Load vector from string using strtok (expect comma separated list)
	template<typename T>
	bool _get_helper(const char* line, std::vector<T>& t)
	{
		// Local copy because strtok modifes the string
		static char buf[256];
		strcpy_s(buf, 256, line);

		// Clear out the default value
		// note: hopefully this should always overwrite
		t.clear();

		char* ctx = nullptr;
		const char* tok = strtok_s(buf, ",", &ctx);

		while (tok && *tok)
		{ 
			T val;
			if(_get_helper(tok, val))
			{
				t.push_back(val);
			}
			tok = strtok_s(NULL, ",", &ctx);
		}

		return !t.empty();
	}

	template<typename T>
	bool ConfigLoader<T>::get(const char * line, T& t)
	{
		if (!strncmp(line, match, len))
		{
			// format is option=option_value
			// + len + 1 leaves just option value to parse
			return _get_helper(line + len + 1, t);
		}
		return false;
	}

	void Config::load(const char* path)
	{
		// Create a ConfigLoader<T> for each config option named option_loader
#define CONFIG_ITEM(type, name, def, help) ConfigLoader<type> name ##_loader(#name);
#include "config_inc.hpp"
#undef CONFIG_ITEM

		// Open the config file
		FILE* config_file = nullptr;
		fopen_s(&config_file, path, "r");
		if (!config_file) { return; }

		// Iterate over each line and compare it to each expected option
		// ConfigLoader<T>::get returns true if the option was successfully loaded
		static char buf[256];
		while (fgets(buf, 256, config_file))
		{
			if (buf[0] == '#') { continue; }
#define CONFIG_ITEM(type, name, def, help) if(name ##_loader.get(buf, name)) { continue; }
#include "config_inc.hpp"
#undef CONFIG_ITEM
			// If none match then clearly there's an issue
			printf("Unrecognised line: %s\n", buf);
		}

		fclose(config_file);		
	}

	void Config::print_help(bool print_to_cmd, bool print_to_file)
	{
		if(print_to_cmd)
		{
			printf("Place config options in a file called icarus.ini in the working directory.\nFormat should be:\noption_name=value\nOptions:\n");
			// Macro prints help lines for each config item
#define CONFIG_ITEM(type, name, def, help) printf("%20s - %-73s - %s\n", #name, help, #type);
#include "config_inc.hpp"
#undef CONFIG_ITEM
		}

		// Write to a file as well to be helpful (will create it if someone just runs icarus.exe without options)
		if (print_to_file)
		{
			FILE* help_file = nullptr;
			fopen_s(&help_file, "icarus_help.txt", "w");
			if (!help_file) { return; }

			fprintf_s(help_file, "Place config options in a file called icarus.ini in the working directory.\nFormat should be:\noption_name=value\nOptions:\n");
#define CONFIG_ITEM(type, name, def, help) fprintf_s(help_file, "%20s - %-73s - %s\n", #name, help, #type);
#include "config_inc.hpp"
#undef CONFIG_ITEM

			fclose(help_file);
		}
	}
}

// Create the global config variable
config::Config g_config;
