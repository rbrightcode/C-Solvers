#pragma once

#include <iostream>
#include <array>
#include <cstdint>
#include <vector>

// Account for a large number of SPRS: 200, 100, 50, 25, 16, 8, 4, 2, 1

class Node
{
public:
    static constexpr uint8_t MAX_ACTIONS = 5;
    using Real = float;

    std::array<Real, MAX_ACTIONS> regret_sum{};
    std::array<Real, MAX_ACTIONS> strategy_sum{};
	uint8_t num_actions = 0;

    Node() = default;

    Node(uint8_t num_actions_) : num_actions(num_actions_)
    {}

    inline void get_strategy(std::array<Real, MAX_ACTIONS>& s) const
    {
		const uint8_t n = num_actions;

        float r0 = regret_sum[0] > 0.0f ? regret_sum[0] : 0.0f;
        float r1 = regret_sum[1] > 0.0f ? regret_sum[1] : 0.0f;
        float r2 = regret_sum[2] > 0.0f ? regret_sum[2] : 0.0f;
        float r3 = regret_sum[3] > 0.0f ? regret_sum[3] : 0.0f;
        float r4 = regret_sum[4] > 0.0f ? regret_sum[4] : 0.0f;

        const float sum = r0 + r1 + r2 + r3 + r4;

        if (sum > 0.0f)
        {
            const float inv = 1.0f / sum;

            s[0] = r0 * inv;
            s[1] = r1 * inv;
            s[2] = r2 * inv;
            s[3] = r3 * inv;
            s[4] = r4 * inv;
        }
        else
        {
            const float uniform = 1.0f / n;

            s[0] = n > 0 ? uniform : 0.0f;
            s[1] = n > 1 ? uniform : 0.0f;
            s[2] = n > 2 ? uniform : 0.0f;
            s[3] = n > 3 ? uniform : 0.0f;
            s[4] = n > 4 ? uniform : 0.0f;
        }
    }
};