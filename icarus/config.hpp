#pragma once

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "util.hpp"

namespace config
{
	template<typename T>
	struct Range
	{
		T begin;
		T step;
		T end;

		void get(std::vector<T>& members)
		{
			if (step == 0) { return; }
			members.reserve(members.size() + static_cast<size_t>(1 + ((begin - end) / step)));
			for (T i = begin; i <= end; i += step)
			{
				members.push_back(i);
			}
		}
	};

	template<typename T> 
	struct is_range_t : public std::false_type
	{

	};

	template<template<typename...> typename T, typename U>
	struct is_range_t<T<U>>
	{
		static constexpr bool value =
			std::is_same<T<U>, Range<U>>::value;
	};

	template <typename T>
	struct ConfigLoader
	{
		ConfigLoader(const char* match) : match(match) { len = strlen(match); }

		bool get(const char * line, T& t);

		const char* match;
		size_t len;
	};

	struct Config
	{
#define CONFIG_ITEM(type, name, def, help) type name = def;
#include "config_inc.hpp"
#undef CONFIG_ITEM
		void load(const char* path);
		void print_help(bool print_to_cmd, bool print_to_file);
	};
}