#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

class Blueprint
{
public:
	struct Strategy
	{
		std::vector<std::string> actions;
		std::vector<float> probabilities;
	};

private:
	std::unordered_map<std::string, Strategy> strategies;

public:
	Blueprint() = default;

	void add_strategy(const std::string& key, const std::vector<std::string>& actions, const std::vector<float>& probabilities)
	{
		strategies[key] = Strategy{ actions, probabilities };
	}

	const Strategy& get_strategy(const std::string& key) const
	{
		auto it = strategies.find(key);
		return it->second;
	}

	bool contains(const std::string& key) const 
	{ 
		return strategies.find(key) != strategies.end(); 
	}

	std::size_t size() const
	{
		return strategies.size();
	}
};