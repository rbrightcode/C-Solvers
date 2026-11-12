// This bot will calculate a Nash equilibrium strategy. Then play according to that strategy against you...
// Since LEDUC poker is small. There is no abstraction... Additionally LEDUC follows limit rules for betting...
// Both players put up a blind of 1. Preflop bets must be 2. Postflop bets must be 4
// Stacks will start with 100
// The flop consists of 1 card.

#include "Leduc.h"
#include "Blueprint.h"
#include <iostream>
#include <random>
#include <chrono>
#include "BestRespone.h"
int get_winner(int card0, int card1, int community)
{
	int rank0 = card0 / 2;
	int rank1 = card1 / 2;
	int comrank = community / 2;

	bool pair0 = (rank0 == comrank);
	bool pair1 = (rank1 == comrank);

	if (pair0)
	{
		return 0;
	}
	if (pair1)
	{
		return 1;
	}
	if (rank0 == rank1)
	{
		return 2;
	}
	return (rank0 > rank1) ? 0 : 1;
}

int main() {
	std::cout << "How many iterations do you want to train the bot for?\n";
	int iter;
	std::cin >> iter;
	auto time0 = std::chrono::high_resolution_clock::now();

	LEDUC trainer;
	trainer.train(iter);

	auto time1 = std::chrono::high_resolution_clock::now();
	std::cout << "Training Done. Time elapsed: " << ((time1 - time0)/1e9).count() << " Seconds\n";

	Blueprint blueprint;
	trainer.build_blueprint(blueprint);

	auto time2 = std::chrono::high_resolution_clock::now();
	std::cout << "Bluepring Done. Time elapsed: " << ((time2 - time1)/1e9).count() << " Seconds\n";

	//Check Exploitability using the best response against an open check and an open raise
	BestResponse response(trainer, blueprint);
	LEDUC::State state;

	state.history = "";

	state.contributions = std::array<int, 2> { 1, 1 };

	state.player = 0;

	state.street = 0;

	state.community = -1;

	float exploitability_2 = response.solve(state, 0);
	float exploitability_1 = response.solve(state, 1);


	std::cout << "Exploitability of player 2: " << exploitability_2 << std::endl;
	std::cout << "Exploitability of player 1: " << exploitability_1 << std::endl;

	auto time3 = std::chrono::high_resolution_clock::now();
	std::cout << "Exploitability Done. Time elapsed: " << ((time3 - time2) / 1e9).count() << " Seconds\n";

	std::cout << "Begin simulated play.\n";

	int player_stack = 100;
	int bot_stack = 100;
	bool end = true;
	int player_position = 0;
	std::array<char, 4> action_dict = { 'f', 'x', 'c', 'r' };
	std::array<char, 6> card_dict = { 'J', 'J', 'Q', 'Q', 'K', 'K' };

	std::array<int, 6> deck = { 0,1,2,3,4,5 };
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_real_distribution<> dis(0.0, 1.0);

	while (end)
	{
		std::cout << "You have " << player_stack << " | The bot has " << bot_stack << std::endl;
		char c;

		while (true)
		{
			std::cout << "Would you like to continue? (y/n)\n";
			std::cin >> c;
			try
			{
				if (c == 'y')
				{
					break;
				}
				else if (c == 'n')
				{
					end = false;
					break;
				}
			}
			catch (...)
			{
				std::cout << "Please enter (y/n)\n";
			}
		}
		if (!end)
		{
			break;
		}
		LEDUC::State game;
		std::shuffle(deck.begin(), deck.end(), gen);
		game.community = deck[2];
		game.history = "";
		game.street = 0;
		game.contributions = { 1,1 };
		game.player = 0;

		int player_card = deck[0];
		int bot_card = deck[1];
		int terminal = true;

		std::cout << "\n";
		std::cout << "You are dealt a " << card_dict[player_card] << std::endl;

		while (terminal)
		{
			if (game.player == player_position)
			{
				std::vector<int> legal_actions = trainer.get_legal(game);
				std::cout << "Would you like to: ";
				for (int act : legal_actions)
				{
					std::cout << action_dict[act] << ", ";
				}
				std::cout << std::endl;

				while (true)
				{
					std::string a;
					std::cin >> a;
					try
					{
						trainer.apply_action(game, a);
						break;
					}
					catch (...)
					{
						std::cout << "Please enter a matching character\n";
					}
				}
			}
			else
			{
				std::string key = trainer.make_key(game, bot_card);
				const Blueprint::Strategy* strategy = &blueprint.get_strategy(key);
				float random_value = dis(gen);
				float cumulative = 0.0f;
				for (int i = 0; i < strategy->actions.size(); i++)
				{
					cumulative += strategy->probabilities[i];
					if (cumulative >= random_value)
					{
						std::string action = strategy->actions[i];
						std::cout << "The bot chooses to " << action << std::endl;
						trainer.apply_action(game, action);
						break;
					}
				}
			}
			if (trainer.is_terminal(game))
			{
				int pot = 0;
				for (int c : game.contributions)
				{
					pot += c;
				}
				if (game.history.back() == 'f')
				{
					int futil = std::min(game.contributions[0], game.contributions[1]);
					if (player_position == game.player)
					{
						player_stack -= futil;
						bot_stack += futil;
					}
					else
					{
						player_stack += futil;
						bot_stack -= futil;
					}
				}
				else
				{
					std::cout << "The bot has a " << card_dict[bot_card] << std::endl;
					int winner = get_winner(player_card, bot_card, game.community);
					if (winner == 0)
					{
						player_stack += pot / 2;
						bot_stack -= pot / 2;
					}
					else if (winner == 1)
					{
						player_stack -= pot / 2;
						bot_stack += pot / 2;
					}
				}
				player_position = (player_position + 1) % 2;
				terminal = false;
			}
			else if (trainer.round_over(game))
			{
				game.street = 1;
				game.player = 0;
				game.history += " ";
				std::cout << "The community card is: " << card_dict[game.community] << std::endl;
				continue;
			}
		}
	}
}

// 1000: BR against p2 blueprint: 0.82
// 1000: BR against p1 blueprint: 0.77
//
// 10000: BR against p2 blueprint: -0.2 Non convergent
// 10000: BR against p1 blueprint: 0.3
// 
// 50000: BR against p2 blueprint: -0.3
// 50000: BR against p1 blueprint: 0.2
// 
// 100000: BR against p2 blueprint: -0.33 near convergent
// 100000: BR against p1 blueprint: 0.43

// About 30 min. Classic CFR.
// 1,000,000: BR against p2 blueprint: - 0.36 Assume this is nash?
// 1,000,000: BR against p1 blueprint: 0. 43

// Implement Linear or Discounted CFR see how convergence changes...
// Implement outcome sampling to improve cfr speed.

// Speed checks, 10000: 34 seconds -> 333/second
// Speed checks, 20000: 68 seconds -> 333/second

// Linear CFR+
//10000: -0.23, 0.51 
//100,000: -0.33, 0.43
//250,000 - 0.36, 0.43

// DCFR
//10000:  - 0.19, 0.61
//100,000: -0.34, 0.45
//250,000: -0.34, 0.42