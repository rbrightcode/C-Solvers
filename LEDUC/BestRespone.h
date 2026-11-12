#pragma once

#include "Blueprint.h"
#include "Leduc.h"
#include <vector>
#include <array>
#include <string>
#include <limits>

class BestResponse
{
private:
	LEDUC& game;
	const Blueprint& blueprint;

public:
	BestResponse(LEDUC& game, const Blueprint& blueprint) : game(game), blueprint(blueprint)
	{ }

	float solve(LEDUC::State& state, int br_player)
	{
		std::array<float, 6> opp_reach{};
		for (int i = 0; i < 6; i++)
		{
			opp_reach[i] = 1.0f;
			if (i == state.community)
			{ 
				opp_reach[i] = 0.0f;
			}
			else
			{
				std::string temp = "";
				int player = 0;
				for (int j = 0; j < state.history.length(); j++)
				{
					if (state.history[j] == ' ')
					{
						player = 0;
						temp += state.history[j];
						continue;
					}
					if (player == br_player)
					{
						player = (player + 1) % 2;
						temp += state.history[j];
						continue;
					}

					LEDUC::State temp_state = state;
					temp_state.history = temp;
					std::string key = game.make_key(temp_state, i);
					const Blueprint::Strategy* strat = &blueprint.get_strategy(key);
					for (int k = 0; k < strat->actions.size(); k++)
					{
						if (strat->actions[k] == std::string(1, state.history[j]))
						{
							opp_reach[i] *= strat->probabilities[k];
							temp += state.history[j];
							player = (player + 1) % 2;
							break;
						}
					}
				}
			}
		}
		float EV = 0.0f;
		int pos_cards = 0;
		for (int i = 0; i < 6; i++)
		{
			std::array<float, 6> new_reach = opp_reach;
			new_reach[i] = 0.0f;
			EV += br(state, br_player, new_reach, i);
			pos_cards++;
		}
		EV /= pos_cards;
		return EV;
	}

	float get_payoff(LEDUC::State& state, int br_player, std::array<float, 6> opp_reach, int card)
	{
		float payoff = 0.0f;
		int pot = std::min(state.contributions[0], state.contributions[1]);
		if (state.history.back() == 'f')
		{
			if (state.player == br_player)
			{
				pot *= -1;
			}
			for (int villain = 0; villain < 6; villain++)
			{
				float weight = opp_reach[villain];
				if (weight == 0.0f)
				{
					continue;
				}

				payoff += pot * weight;
			}
		}
		else
		{
			for (int villain = 0; villain < 6; villain++)
			{
				float weight = opp_reach[villain];

				if (weight == 0.0f)
					continue;

				// Pair beats every high card.
				bool hero_pair = (card / 2 == state.community / 2);

				bool villain_pair = (villain / 2 == state.community / 2);

				if (hero_pair)
				{
					payoff += weight * pot;
				}
				else if (villain_pair)
				{
					payoff -= weight * pot;
				}
				else
				{
					// Both have high cards.
					int hero_rank = card / 2;
					int villain_rank = villain / 2;

					if (hero_rank > villain_rank)
						payoff += weight * pot;
					else if (hero_rank < villain_rank)
						payoff -= weight * pot;
					// Equal rank = chop, so add nothing.
				}
			}
		}
		return payoff;
	}

private:
	float br(LEDUC::State state, int br_player, std::array<float, 6> opp_reach, int card)
	{
		// Handle Terminality
		if (game.is_terminal(state))
		{
			return get_payoff(state, br_player, opp_reach, card);
		}

		if (game.round_over(state))
		{
			if (state.street == 0)
			{
				float utils = 0.0f;
				state.history += " ";
				state.street = 1;
				state.player = 0;
				for (int i = 0; i < 6; i++)
				{
					if (i == card)
					{
						continue;
					}
					std::array<float, 6> new_reach = opp_reach;
					new_reach[i] = 0.0f;
					state.community = i;
					float card_utils = br(state, br_player, new_reach, card);
					utils += card_utils / 5;
				}
				return utils;
			}
		}

		int player = state.player;
		std::vector<int> actions = game.get_legal(state);
		int n_actions = actions.size();

		if (player == br_player)
		{
			float best = -std::numeric_limits<float>::infinity();
			for (int i = 0; i < n_actions; i++)
			{
				std::string act = game.action_dict[actions[i]];
				LEDUC::State new_state = state;
				game.apply_action(new_state, act);
				float action_value = br(new_state, br_player, opp_reach, card);
				if (action_value > best)
				{
					best = action_value;
				}
			}
			return best;
		}
		else
		{
			std::array<const Blueprint::Strategy*, 6> strategies = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
			for (int i = 0; i < 6; i++)
			{
				if (i == state.community)
				{
					continue;
				}
				std::string key = game.make_key(state, i);
				strategies[i] = &blueprint.get_strategy(key);
			}
			float utils = 0.0f;
			for (int i = 0; i < n_actions; i++)
			{
				std::string act = game.action_dict[actions[i]];
				LEDUC::State new_state = state;
				game.apply_action(new_state, act);
				std::array<float, 6> new_reach{};
				for (int j = 0; j < 6; j++)
				{
					if (j == state.community)
					{
						continue;
					}
					new_reach[j] = opp_reach[j] * strategies[j]->probabilities[i];
				}
				utils += br(new_state, br_player, new_reach, card);
			}
			return utils;
		}
	}
};