#pragma once

#include <iostream>
#include <random>
#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <numeric>

class Node
{
public:
    std::string key;
    int n_actions;

    std::vector<float> regret_sum;
    std::vector<float> strategy_sum;
    std::vector<int> actions;
    std::vector<float> strategy;
    int player;

    Node(const std::string& key, const std::vector<int>& action_dict, int n_actions, int p) : key(key), n_actions(n_actions), actions(action_dict), player(p)
    {
        regret_sum.resize(n_actions, 0.0f);
        strategy_sum.resize(n_actions, 0.0f);
        strategy.resize(n_actions, 1.0f / n_actions);
        player = p;
    }

    std::vector<float> get_strategy()
    {
        std::vector<float> regrets = regret_sum;

        float normalizing_sum = 0.0f;

        for (int i = 0; i < n_actions; i++)
        {
            if (regrets[i] < 0.0f)
            {
                regrets[i] = 0.0f;
            }

            normalizing_sum += regrets[i];
        }

        if (normalizing_sum > 0.0f)
        {
            for (int i = 0; i < n_actions; i++)
            {
                regrets[i] /= normalizing_sum;
            }

            return regrets;
        }

        return std::vector<float>(
            n_actions,
            1.0f / n_actions
        );
    }

    std::vector<float> get_average_strategy()
    {
        std::vector<float> strategy(n_actions, 0.0f);
        float total = 0.0f;

        for (int i = 0; i < n_actions; i++)
        {
            total += strategy_sum[i];
        }

        if (total > 0)
        {
            for (int i = 0; i < n_actions; i++)
            {
                strategy[i] = strategy_sum[i] / total;
            }
        }
        else
        {
            strategy.assign(n_actions, 1.0f / n_actions);
        }

        return strategy;
    }
};
