#pragma once

#include "Key.h"
#include "Node.h"
#include "NLH.h"

class Blueprint
{
public:

    static constexpr uint8_t MAX_ACTIONS = 5;

    struct Strategy
    {
        std::array<uint8_t, MAX_ACTIONS> probabilities{};
    };

private:

    std::vector<Strategy> strategies;
    std::unordered_map<uint64_t, uint32_t> history_lookup;
	NLH& nl;

public:

	Blueprint(NLH& nl_instance) : nl(nl_instance)
    {
		build_blueprint(nl.history_lookup, nl.nodes);
    }

    void build_blueprint(const std::unordered_map<uint64_t, uint32_t>& hist_lookup, const std::vector<Node>& nodes)
    {
        strategies.clear();
        strategies.reserve(nodes.size());
		history_lookup = hist_lookup;

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const Node& node = nodes[i];

            Strategy strategy;

            float total = 0;
			int count = node.num_actions;

            for (uint8_t i = 0; i < count; ++i) {
                total += node.strategy_sum[i];
            }

            if (total > 0.0f)
            {
                for (uint8_t i = 0; i < count; ++i)
                {
                    strategy.probabilities[i] = static_cast<uint8_t>(node.strategy_sum[i] / total * 255.0f);
                }
            }
            else
            {
                uint8_t p = static_cast<uint8_t>(255.0f / static_cast<float>(count));

                for (uint8_t i = 0; i < count; ++i)
                    strategy.probabilities[i] = p;
            }

            strategies.push_back(strategy);
        }
    }

    Strategy get_strategy(const State& state) const
    {
        const BucketManager::BucketLookup* lookup = nullptr;

        switch (state.board_count)
        {
        case 0:
            lookup = &nl.buckets->preflop;
            break;

        case 3:
            lookup = &nl.buckets->flop;
            break;

        case 4:
            lookup = &nl.buckets->turn;
            break;

        case 5:
            lookup = &nl.buckets->river;
            break;

        default:
            throw std::runtime_error("Invalid board_count");
        }

        auto board_it = lookup->board_lookup.find(state.canon_mask);

        if (board_it == lookup->board_lookup.end())
            throw std::runtime_error("Canonical board not found in bucket lookup");

        const uint32_t board_id = board_it->second;
        const size_t offset = static_cast<size_t>(board_id) * ComboIndexer::N_COMBOS;

        uint16_t combo = nl.canon.combo_map[state.perm_id][state.combos[state.player]];
        const uint16_t bucket_id = lookup->data[offset + combo];

        if (bucket_id == BucketManager::INVALID_BUCKET)
            throw std::runtime_error("Invalid bucket for sampled combo");

        auto it = history_lookup.find(state.history.data);
        
        if (it == history_lookup.end())
        {
            LegalActions actions = nl.get_legal(state);
            Strategy strat;
			for (int i = 0; i < actions.count; ++i)
			{
				strat.probabilities[i] = static_cast<uint8_t>(255.0f / static_cast<float>(actions.count));
			}

			return  strat;
        }

		//std::cout << static_cast<int>(strategies[it->second + bucket_id].probabilities[0]) << std::endl;

        return strategies[it->second + bucket_id];
    }
};