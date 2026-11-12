#pragma once

#include <iostream>
#include <random>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include "Node.h"
#include "Blueprint.h"
#include <cmath>

class LEDUC
{
public:
    std::unordered_map<std::string, Node> nodeMap;
    int traverser = 0;
    float expected_value = 0.0f;
    int n_cards = 6;
    std::vector<std::string> action_dict = { "f", "x", "c", "r" };
    std::vector<int> deck = { 0,1,2,3,4,5 };

    struct State
    {
        std::string history = "";

        int street = 0;
        int community = -1;
        
        int player=  0;
        std::array<int, 2> contributions{};
    };

    std::random_device rd;
    std::mt19937 gen{ rd() };

    std::vector<int> get_legal(State& state) const
    {
        std::vector<int> legal;
        int n = state.history.length();
        if (n == 0 || state.history.back() == 'x' || state.history.back() == ' ')
        {
            legal.assign({ 1, 3 });
        }
        else if (state.history.back() == 'r')
        {
            if (n >= 3)
            {
                if (state.history.substr(n - 3) == "rrr")
                {
                    legal.assign({ 0,2 });
                }
                else
                {
                    legal.assign({ 0, 2, 3 });
                }
            }
            else
            {
                legal.assign({ 0, 2, 3 });
            }
        }
        return legal;
    }

    bool is_terminal(State& state)
    {
        if (state.history.empty())
        {
            return false;
        }
        else if (state.history.back() == ' ')
        {
            return false;
        }
        char last = state.history.back();
        if (last == 'f')
        {
            return true;
        }
        if (state.street == 1)
        {
            std::string last_two = state.history.substr(state.history.size() - 2);
            if (last_two == "xx" || last_two == "rc")
            {
                return true;
            }
        }
        return false;
    }

    bool round_over(State& state)
    {
        if (state.history.size() < 2)
        {
            return false;
        }
        std::string last_two = state.history.substr(state.history.size() - 2);
        if (last_two == "xx" || last_two == "rc")
        {
            return true;
        }
        return false;
    }

    void get_reward(State& state, std::array<float, 6>& out, const std::array<float, 6>& opp_reach, int trav)
    {
        char final = state.history.back();

        if (final == 'x' || final == 'c')
        {
            showdown(state, out, opp_reach);
        }
        else
        {
            int fold_player = state.player;
            int fold_amount = state.contributions[fold_player];

            fold_reward(out, fold_amount, opp_reach, fold_player, trav);
        }
    }

    void showdown(LEDUC::State& state, std::array<float, 6>& out, const std::array<float, 6>& opp_reach)
    {
        float utility = state.contributions[0];

        for (int hero = 0; hero < 6; hero++)
        {
            if (hero == state.community)
                continue;

            float value = 0.0f;

            for (int villain = 0; villain < 6; villain++)
            {
                if (villain == state.community)
                    continue;

                if (villain == hero)
                    continue;

                float weight = opp_reach[villain];

                if (weight == 0.0f)
                    continue;

                // Pair beats every high card.
                bool hero_pair = hero / 2 == state.community / 2;

                bool villain_pair = villain / 2 == state.community / 2;

                if (hero_pair && !villain_pair)
                {
                    value += weight;
                }
                else if (!hero_pair && villain_pair)
                {
                    value -= weight;
                }
                else
                {
                    // Both have high cards.
                    int hero_rank = hero / 2;
                    int villain_rank = villain / 2;

                    if (hero_rank > villain_rank)
                        value += weight;
                    else if (hero_rank < villain_rank)
                        value -= weight;
                    // Equal rank = chop, so add nothing.
                }
            }

            out[hero] = value * utility;
        }
    }

    void fold_reward(std::array<float, 6>& out, int utility, const std::array<float, 6>& opp_reach, int fold_player, int trav)
    {
        int payoff;
        if (fold_player == trav)
        {
            payoff = -1 * utility;
        }
        else
        {
            payoff = utility;
        }
        float p_loss = 0;
        for (int i = 0; i < opp_reach.size(); i++)
        {
            p_loss += opp_reach[i];
        }
        for (int i = 0; i < out.size(); i++)
        {
            out[i] = (p_loss - opp_reach[i]) * payoff;
        }
    }

    std::string make_key(State& state, int card)
    {
        if (state.street == 1)
        {
            return std::to_string(card) + "|" +
                std::to_string(state.community) + "|" +
                state.history;
        }
        return std::to_string(card) + "|" + state.history;
        
    }

    Node& get_node(State& state, int card)
    {
        std::string key = make_key(state, card);

        auto it = nodeMap.find(key);

        if (it == nodeMap.end())
        {
            std::vector<int> actions = get_legal(state);

            auto result =
                nodeMap.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(key, actions, actions.size(), state.player)
                );

            return result.first->second;
        }

        return it->second;
    }

    void apply_action(State& state, std::string a)
    {
        int bet_size = (state.street == 0) ? 2 : 4;
        if (a == "c")
        {
            state.contributions[state.player] = state.contributions[(state.player + 1) % 2];
            state.player = (state.player + 1) % 2;
        }
        else if (a == "r")
        {
            state.contributions[state.player] = state.contributions[(state.player+1)%2] + bet_size;
            state.player = (state.player + 1) % 2;
        }
        else if (a == "x")
        {
            state.player = (state.player + 1) % 2;
        }

        state.history += a; 
    }

    std::array<float, 6> cfr(State state, std::array<float, 6>& r_1, std::array<float, 6>& r_2, float iteration)
    {
        // check to see if the round is over or if the node is terminal.
        if (is_terminal(state))
        {
            std::array<float, 6> payoff{};
            get_reward(state, payoff, r_2, traverser);
            return payoff;
        }
        
        if (round_over(state))
        {
            if (state.street == 0)
            {
                state.history += " ";
                state.street = 1;
                state.player = 0;
                state.community = deck[2];
                std::array<float, 6> new_r1 = r_1;
                std::array<float, 6> new_r2 = r_2;
                new_r1[deck[2]] = 0.0f;
                new_r2[deck[2]] = 0.0f;
                return cfr(state, new_r1, new_r2, iteration);
            }
        }   

        // We are doing vector cfr now...
        std::vector<Node*> info_sets(n_cards, nullptr);
        for (int i = 0; i < n_cards; i++)
        {
            if (i == state.community)
            {
                continue;
            }

            info_sets[i] = &get_node(state, i);
        }

        std::array<float, 6> utils{};
        std::vector<int> actions = get_legal(state);
        int n_actions = actions.size();
        std::array<std::array<float, 6>, 3> action_utils{};

        for (int i = 0; i < n_cards; i++)
        {
            if (i == state.community)
            {
                continue;
            }

            info_sets[i]->strategy = info_sets[i]->get_strategy();
        }

        // Iterate through legal actions.
        for (int i = 0; i < n_actions; i++)
        {
            std::string act = action_dict[actions[i]];
            State new_state = state;

            apply_action(new_state, act);

            std::vector<float> strats_taken(n_cards, 0);
            for (int j = 0; j < n_cards; j++)
            {
                if (j == state.community)
                {
                    continue;
                }
                strats_taken[j] = info_sets[j]->strategy[i];
            }

            if (traverser == state.player)
            {
                std::array<float, 6> new_reach{};
                for (int j = 0; j < n_cards; j++)
                {
                    new_reach[j] = r_1[j] * strats_taken[j];
                }
                action_utils[i] = cfr(new_state, new_reach, r_2, iteration);
                for (int j = 0; j < n_cards; j++)
                {
                    utils[j] += action_utils[i][j] * strats_taken[j];
                }
            }
            else
            {
                std::array<float, 6> new_reach{};
                for (int j = 0; j < n_cards; j++)
                {
                    new_reach[j] = r_2[j] * strats_taken[j];
                }
                action_utils[i] = cfr(new_state, r_1, new_reach, iteration);
                for (int j = 0; j < n_cards; j++)
                {
                    utils[j] += action_utils[i][j];
                }
            }
        }
        // update regrets for traverser
        if (traverser == state.player)
        {
            for (int i = 0; i < n_cards; i++)
            {
                for (int j = 0; j < n_actions; j++)
                {
                    if (i == state.community)
                    {
                        continue;
                    }

                    //info_sets[i]->regret_sum[j] = std::max(info_sets[i]->regret_sum[j] + action_utils[j][i] - utils[i], 0.0f); //CFR+
                    info_sets[i]->regret_sum[j] += (action_utils[j][i] - utils[i]) * iteration; // Linear CFR
                    info_sets[i]->strategy_sum[j] += info_sets[i]->strategy[j] * r_1[i] * iteration; //Linear CFR

                    /*
                    if (info_sets[i]->regret_sum[j] <= 0) // DCFR
                    {
                        info_sets[i]->regret_sum[j] = (info_sets[i]->regret_sum[j] / 2) + action_utils[j][i] - utils[i];
                    }
                    else
                    {
                        info_sets[i]->regret_sum[j] = (info_sets[i]->regret_sum[j] * pow(iteration, 1.5) / (pow(iteration, 1.5) + 1)) + action_utils[j][i] - utils[i];
                    }
                    info_sets[i]->strategy_sum[j] += info_sets[i]->strategy[j] * r_1[i] * pow(iteration / (iteration + 1), 2); //DCFR
                    */
                }
            }
        }

        return utils;
    }

    void train(int iter)
    {
        State state;
        state.history = "";
        std::array<int, 2> contri = { 1, 1 };
        state.contributions = contri;
        state.player = 0;
        state.street = 0;
        for (int i = 0; i < iter; i++)
        {
            traverser = i % 2;
            std::shuffle(deck.begin(), deck.end(), gen);
            state.community = -1;
            std::array<float, 6> r_1{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
            std::array<float, 6> r_2{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

            cfr(state, r_1, r_2, i);
        }
    }

    void build_blueprint(Blueprint& blueprint) const
    {
        for (const auto& [key, node] : nodeMap)
        {
            float total = std::accumulate(node.strategy_sum.begin(), node.strategy_sum.end(), 0.0f);

            std::vector<float> probabilities(node.strategy_sum.size());

            if (total > 0.0f)
            {
                for (std::size_t i = 0; i < node.strategy_sum.size(); i++)
                {
                    probabilities[i] = node.strategy_sum[i] / total;
                }
            }
            else
            {
                float uniform = 1.0f / node.strategy_sum.size();

                std::fill(probabilities.begin(),probabilities.end(),uniform);
            }

            std::vector<std::string> actions;

            for (int action : node.actions)
            {
                actions.push_back(action_dict[action]);
            }

            blueprint.add_strategy(key, actions, probabilities);
        }
    }
};