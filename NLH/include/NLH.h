#pragma once

#include <random>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "Node.h"
#include "Combos.h"
#include "Evaluator.h"
#include  <limits>
#include <memory>
#include <iostream>
#include "Canon.h"
#include "Buckets.h"
#include "absl/container/flat_hash_map.h"
#include "Key.h"
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <type_traits>
#include <stdexcept>

class NLH
{
public:
    using CardMask = uint64_t;
    using Card = uint8_t;
    using ComboId = uint16_t;
    using NodeId = uint32_t;

    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    static constexpr int MAX_DEPTH = 33;

    Evaluator eval;
    ComboIndexer Combos;
    Canon canon;

    static constexpr std::array<uint16_t, 5> BET_PCTS = { 33, 50, 67, 100, 125 };

    std::unique_ptr<BucketManager> buckets;
    std::unordered_map<uint64_t, NodeId> history_lookup;
    std::vector<Node> nodes;

    int traverser = 0;
    uint64_t next_iteration = 0;

    std::array<uint8_t, ComboIndexer::N_CARDS> deck = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
        21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51 };

    std::random_device rd;
    std::mt19937 gen{ rd() };
    std::uniform_real_distribution<float> dis{ 0.0f, 1.0f };

    NLH() : buckets(std::make_unique<BucketManager>())
    {
        buckets->preflop = BucketManager::load_buckets("Data/Preflop_Buckets");
        buckets->flop = BucketManager::load_buckets("Data/Flop_Buckets.dat");
        buckets->turn = BucketManager::load_buckets("Data/Turn_Buckets.dat");
        buckets->river = BucketManager::load_buckets("Data/River_BucketsS.dat");

        canon.load_perm_ids("Data/PermIds.dat");

        //nodes.reserve(340'000'000);
        nodes.reserve(165'000'000);
    }

    NodeId get_node_id(const State& state)
    {
        const BucketManager::BucketLookup* lookup = nullptr;

        switch (state.board_count)
        {
        case 0:
            lookup = &buckets->preflop;
            break;

        case 3:
            lookup = &buckets->flop;
            break;

        case 4:
            lookup = &buckets->turn;
            break;

        case 5:
            lookup = &buckets->river;
            break;

        default:
            throw std::runtime_error("Invalid board_count");
        }

        auto board_it = lookup->board_lookup.find(state.canon_mask);

        if (board_it == lookup->board_lookup.end())
            throw std::runtime_error("Canonical board not found in bucket lookup");

        const uint32_t board_id = board_it->second;
        const size_t offset = static_cast<size_t>(board_id) * ComboIndexer::N_COMBOS;

        uint16_t combo = canon.combo_map[state.perm_id][state.combos[state.player]];
        const uint16_t bucket_id = lookup->data[offset + combo];

        if (bucket_id == BucketManager::INVALID_BUCKET)
            throw std::runtime_error("Invalid bucket for sampled combo");

        const NodeId node_offset = static_cast<NodeId>(nodes.size());

        auto [it, inserted] = history_lookup.try_emplace(state.history.data, node_offset); // some issue is happening here to fix eventually

        if (!inserted)
            return it->second + bucket_id;

		LegalActions legal = get_legal(state);
    
        for (int i = 0; i < lookup->num_buckets; ++i)
        {
            nodes.emplace_back(legal.count);
        }

        return node_offset + bucket_id;
    }

    inline uint32_t raise_amount(const State& state, uint16_t pct) const
    {
        const uint8_t p = state.player;
        const uint8_t o = p ^ 1;

        const uint32_t mine = state.contributions[p];
        const uint32_t opp = state.contributions[o];

        const uint32_t to_call = (opp > mine) ? opp - mine : 0;

        const uint32_t pot = mine + opp;
        const uint32_t pot_after_call = pot + to_call;

        return pot_after_call * pct / 100;
    }

    inline uint32_t action_amount(const State& state, Actions action) const
    {
        const uint8_t p = state.player;
        const uint8_t o = p ^ 1;

        const uint32_t mine = state.contributions[p];
        const uint32_t opp = state.contributions[o];

        const uint32_t to_call = (opp > mine) ? opp - mine : 0;

        if (action == Actions::Call)
            return to_call;

        if (action >= Actions::B33 && action <= Actions::B125)
        {
            uint16_t pct = BET_PCTS[static_cast<int>(action) - static_cast<int>(Actions::B33)];
            return to_call + raise_amount(state, pct);
        }

        if (action == Actions::Jam)
            return state.stacks[p];

        return 0;
    }

    inline LegalActions get_legal(const State& state)
    {
        LegalActions legal;

        const uint8_t p = state.player;
        const uint8_t o = p ^ 1;

        const uint32_t my_contribution = state.contributions[p];
        const uint32_t opp_contribution = state.contributions[o];

        const uint32_t pot = my_contribution + opp_contribution;
        const uint32_t to_call = (opp_contribution > my_contribution) ? opp_contribution - my_contribution : 0;
        const uint32_t pot_eff = pot + to_call;

        const uint32_t stack = state.stacks[p];
        const uint32_t stack_eff = (stack > to_call) ? stack - to_call : 0;

        if (to_call == 0)
        {
            legal.add(Actions::Check);
            for (int i = 0; i < 5; i++)
            {
                if (i == 1 || i == 3)
                    continue; // This is just an abstraction choice.

                uint32_t bet = BET_PCTS[i] * pot / 100;
                if (bet < stack * 100 / 125)
                {
                    legal.add(static_cast<Actions>(i + 3));
                }
            }
        }
        else
        {
            legal.add(Actions::Fold);
            legal.add(Actions::Call);
            if (state.round_raises < 3) // Capping raises abstraction choicee.
            {
                for (int i = (state.last_raise == 1 && state.board_count == 0) ? 3 : 0; i < 5; i++)
                {
                    if (i == 0 || i == 2 || i == 4) // abstraction choice.
                        continue;
                    uint32_t bet = BET_PCTS[i] * pot_eff / 100;
                    if (bet < stack_eff * 100 / 125 && bet >= state.last_raise)
                    {
                        legal.add(static_cast<Actions>(i + 3));
                    }
                }
            }
        }
        if (stack > to_call)
        {
            legal.add(Actions::Jam);
        }

        return legal;
    }

    void apply_action(State& state, Actions action)
    {
        const uint32_t opp = state.contributions[state.player ^ 1];
        const uint32_t mine = state.contributions[state.player];
        const uint32_t call = (opp > mine) ? opp - mine : 0;

        switch (action)
        {
        case Actions::Fold:
        {
            state.history.push(action);
            return;
        }

        case Actions::Check:
        {
            break;
        }

        case Actions::Call:
        {
            state.contributions[state.player] += call;
            state.stacks[state.player] -= call;
            state.last_raise = 2;
            break;
        }

        case Actions::Jam:
        {
            uint32_t bet = state.stacks[state.player];
            state.contributions[state.player] += bet;
            state.stacks[state.player] -= bet;
            state.round_raises += 1;
            break;
        }

        default:
        {
            const uint32_t raise = raise_amount(state, BET_PCTS[static_cast<int>(action) - 3]);
            const uint32_t total = call + raise;

            state.contributions[state.player] += total;
            state.stacks[state.player] -= total;

            state.last_raise = raise;
            state.round_raises += 1;
            break;
        }

        }
        state.player = state.player ^ 1;
        state.history.push(action);
    }

    bool is_terminal(const State& state) const 
    {
        if (state.history.length == 0)
            return false;

        if (state.stacks[0] == 0 && state.stacks[1] == 0)
            return true;

        const Actions last = state.history.back();

        if (last == Actions::Fold)
            return true;

        if (state.board_count == 5 && round_over(state))
        {
            return true;
        }

        return false;
    }

    bool round_over(const State& state) const 
    {
        if (state.history.length - state.round_start < 2)
            return false;

        const Actions last = state.history.back();
        const Actions previous = state.history.get(state.history.length - 2);

        if (last == Actions::Check && previous == Actions::Check)
        {
            return true;
        }

        if (last == Actions::Call)
        {
            return true;
        }

        return false;
    }

    void next_round(State& state, const std::array<uint8_t, ComboIndexer::N_CARDS>& deck)
    {
        if (state.board_count == 0)
        {
            state.board_count = 3;
            state.board_mask = uint64_t(1) << deck[4] | uint64_t(1) << deck[5] | uint64_t(1) << deck[6];
            state.perm_id = state.flop_id;
        }

        else if (state.board_count == 3)
        {
            state.board_count = 4;
            state.board_mask |= uint64_t(1) << deck[7];
            state.perm_id = state.turn_id;

        }

        else if (state.board_count == 4)
        {
            state.board_count = 5;
            state.board_mask |= uint64_t(1) << deck[8];
            state.perm_id = state.river_id;
        }

        state.player = 0;
        state.canon_mask = canon.get_canon_mask(state.board_mask, state.perm_id);
        state.round_raises = 0;
        state.last_raise = 2;
        state.round_start = state.history.length;
    }

    float get_reward(State& state, int trav, const std::array<uint8_t, ComboIndexer::N_CARDS>& deck)
    {
        Actions final = state.history.back();

        if (final == Actions::Check || final == Actions::Call || final == Actions::Jam)
        {
            std::array<Card, 5> board = { deck[4], deck[5], deck[6], deck[7], deck[8] };
            uint16_t util = state.contributions[0];

            uint16_t hero_score;
            uint16_t villain_score;

            if (trav == 0)
            {
                hero_score = eval.evaluate(deck[0], deck[1], board);
                villain_score = eval.evaluate(deck[2], deck[3], board);
            }
            else
            {
                villain_score = eval.evaluate(deck[0], deck[1], board);
                hero_score = eval.evaluate(deck[2], deck[3], board);
            }

            if (hero_score > villain_score)
                return util;
            else if (villain_score > hero_score)
                return -util;

            return 0;
        }
        else
        {
            int fold_player = state.player;
            uint16_t util = state.contributions[fold_player];

            const float payoff = (fold_player == trav) ? -static_cast<float>(util) : static_cast<float>(util);
            return payoff;
        }
    }

    float cfr(State state, float r1, float iteration, int depth)
    {
        if (is_terminal(state))
        {
            return get_reward(state, traverser, deck);
        }

        if (round_over(state))
        {
            next_round(state, deck);
        }

        NodeId node_id = get_node_id(state);

        Node& node = nodes[node_id];

        LegalActions actions = get_legal(state);

		std::array<float, Node::MAX_ACTIONS> strategy;

        node.get_strategy(strategy);

        // For traverser nodes iterate over all possible actions and update regrets accordingly.
        if (state.player == traverser)
        {
            std::array<float, Node::MAX_ACTIONS> action_utils;
            float util = 0;

            for (uint8_t a = 0; a < actions.count; ++a)
            {
                const Actions action = static_cast<Actions>(actions.actions[a]);

                State new_state = state;

                apply_action(new_state, action);

                float new_reach = r1 * strategy[a];

                action_utils[a] = cfr(new_state, new_reach, iteration, depth + 1);

                util += strategy[a] * action_utils[a];
            }

            // Update regrets using linear CFR

            Node& node = nodes[node_id];

            for (uint8_t a = 0; a < actions.count; ++a)
            {
                node.regret_sum[a] += (action_utils[a] - util) * iteration;
                node.strategy_sum[a] += strategy[a] * r1 * iteration;
            }
            return util;
        }

        // For opponent nodes we are going to randomly select an action across all combos, based on probability.
        else
        {
            Actions action = static_cast<Actions>(actions.actions[0]);
            float random_value = dis(gen);
            float cumulative = 0.0f;
            for (uint8_t a = 0; a < actions.count; ++a)
            {
                cumulative += strategy[a];
                if (cumulative >= random_value)
                {
                    action = static_cast<Actions>(actions.actions[a]);
                    break;
                }
            }

            State new_state = state;

            apply_action(new_state, action);

            return cfr(new_state, r1, iteration, depth + 1);
        }
    }

    void train(uint64_t iterations)
    {
        for (uint64_t i = 0; i < iterations; ++i)
        {
            if (next_iteration % 100000 == 0)
            {
                std::cout << "Iteration: " << next_iteration << '\n';
                print_nodes_size();
            }

            traverser = static_cast<int>(i % 2);

            std::shuffle(deck.begin(), deck.end(), gen);

            State state;

            state.contributions = { 2, 1 };
            state.stacks = { 198, 199 };
            state.player = 1;
            state.last_raise = 1;

            state.combos[0] = Combos.get_id(deck[0], deck[1]);
            state.combos[1] = Combos.get_id(deck[2], deck[3]);

            state.board = {
                deck[4], deck[5], deck[6], deck[7], deck[8]
            };

            state.flop_id = canon.get_perm_id(state.board, 3);
            state.turn_id = canon.get_perm_id(state.board, 4);
            state.river_id = canon.get_perm_id(state.board, 5);

            cfr(state, 1.0f, static_cast<float>(next_iteration), 0);

            // Advance only after this iteration completes.
            next_iteration += 1;
        }
    }

    void print_nodes_size()
    {
        const double gb =
            static_cast<double>(nodes.size() * sizeof(Node)) /
            (1024.0 * 1024.0 * 1024.0);

        const double capacity_gb =
            static_cast<double>(nodes.capacity() * sizeof(Node)) /
            (1024.0 * 1024.0 * 1024.0);

        std::cout
            << "Nodes: " << nodes.size()
            << "/" << nodes.capacity()
            << "\nNode size: " << sizeof(Node)
            << " bytes"
            << "\nNode memory: " << gb << " GB"
            << "\nReserved: " << capacity_gb << " GB"
            << "\nHistories: " << history_lookup.size()
            << "\n";
    }


private:
    template <typename T>
    static void write_binary(std::ofstream& file, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        file.write(reinterpret_cast<const char*>(&value), sizeof(T));

        if (!file)
            throw std::runtime_error("Failed to write model checkpoint");
    }

    template <typename T>
    static void read_binary(std::ifstream& file, T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        file.read(reinterpret_cast<char*>(&value), sizeof(T));

        if (!file)
            throw std::runtime_error("Invalid or incomplete model checkpoint");
    }

public:
    void save_model(const std::string& filename) const
    {
        static_assert(std::is_trivially_copyable_v<Node>, "Node must be trivially copyable for binary checkpointing");

        const std::string temp_filename = filename + ".tmp";

        try
        {
            std::ofstream file(temp_filename, std::ios::binary | std::ios::trunc);

            if (!file)
                throw std::runtime_error("Could not open temporary checkpoint file");

            // Header
            constexpr char magic[8] = { 'N', 'L', 'H', 'C', 'F', 'R', '0', '1' };

            file.write(magic, sizeof(magic));

            const uint32_t version = 1;
            const uint64_t node_count = nodes.size();
            const uint64_t history_count = history_lookup.size();

            write_binary(file, version);
            write_binary(file, next_iteration);
            write_binary(file, node_count);
            write_binary(file, history_count);

            // Save nodes as a contiguous block.
            if (!nodes.empty())
            {
                file.write(reinterpret_cast<const char*>(nodes.data()), static_cast<std::streamsize>(nodes.size() * sizeof(Node)));

                if (!file)
                    throw std::runtime_error("Failed to write nodes");
            }

            // Save history -> node offset mappings.
            for (const auto& [history, node_id] : history_lookup)
            {
                write_binary(file, history);
                write_binary(file, node_id);
            }

            write_binary(file, deck);

            // Preserve the RNG state so resumed training continues
            // with the same random sequence.
            std::ostringstream rng_stream;
            rng_stream << gen;

            const std::string rng_state = rng_stream.str();
            const uint64_t rng_size = rng_state.size();

            write_binary(file, rng_size);

            file.write(rng_state.data(), static_cast<std::streamsize>(rng_state.size()));

            if (!file)
                throw std::runtime_error("Failed to write RNG state");

            file.flush();

            if (!file)
                throw std::runtime_error("Failed to flush checkpoint");

            file.close();

            if (!file)
                throw std::runtime_error("Failed to close checkpoint");

            // Replace the previous checkpoint only after the new
            // temporary file has been written successfully.
            std::error_code ec;
            std::filesystem::rename(temp_filename, filename, ec);

            if (ec)
            {
                // On platforms where rename cannot replace an
                // existing file, remove the old file and retry.
                std::error_code remove_ec;
                std::filesystem::remove(filename, remove_ec);

                ec.clear();
                std::filesystem::rename(temp_filename, filename, ec);

                if (ec)
                    throw std::runtime_error("Could not replace checkpoint: " + ec.message());
            }
        }
        catch (...)
        {
            std::error_code ec;
            std::filesystem::remove(temp_filename, ec);
            throw;
        }

        std::cout
            << "Model saved: " << filename
            << "\nNext iteration: " << next_iteration
            << "\nNodes: " << nodes.size()
            << "\nHistories: " << history_lookup.size()
            << '\n';
    }

    void load_model(const std::string& filename)
    {
        static_assert(std::is_trivially_copyable_v<Node>, "Node must be trivially copyable for binary checkpointing");

        std::ifstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open model: " + filename);

        // Validate header.
        constexpr char expected_magic[8] = {
            'N', 'L', 'H', 'C', 'F', 'R', '0', '1'
        };

        char magic[8];
        file.read(magic, sizeof(magic));

        if (!file ||
            !std::equal(
                std::begin(magic),
                std::end(magic),
                std::begin(expected_magic)
            ))
        {
            throw std::runtime_error("Invalid NLH model checkpoint");
        }

        uint32_t version;
        uint64_t loaded_iteration;
        uint64_t node_count;
        uint64_t history_count;

        read_binary(file, version);
        read_binary(file, loaded_iteration);
        read_binary(file, node_count);
        read_binary(file, history_count);

        if (version != 1)
            throw std::runtime_error("Unsupported model version");

        if (node_count > std::numeric_limits<NodeId>::max())
            throw std::runtime_error("Checkpoint contains too many nodes");

        // Basic size sanity checks against the remaining file.
        const auto data_start = file.tellg();
        file.seekg(0, std::ios::end);
        const auto file_end = file.tellg();
        file.seekg(data_start);

        if (data_start < 0 || file_end < data_start)
            throw std::runtime_error("Invalid checkpoint file size");

        const uint64_t remaining =
            static_cast<uint64_t>(file_end - data_start);

        if (node_count > remaining / sizeof(Node))
            throw std::runtime_error("Truncated node data");

        // Read nodes into temporary storage first.
        std::vector<Node> loaded_nodes;
        loaded_nodes.resize(static_cast<size_t>(node_count));

        if (!loaded_nodes.empty())
        {
            file.read(
                reinterpret_cast<char*>(loaded_nodes.data()),
                static_cast<std::streamsize>(
                    loaded_nodes.size() * sizeof(Node)
                    )
            );

            if (!file)
                throw std::runtime_error("Failed to read node data");
        }

        std::unordered_map<uint64_t, NodeId> loaded_lookup;
        loaded_lookup.reserve(static_cast<size_t>(history_count));

        for (uint64_t i = 0; i < history_count; ++i)
        {
            uint64_t history;
            NodeId node_id;

            read_binary(file, history);
            read_binary(file, node_id);

            if (node_id >= node_count)
                throw std::runtime_error(
                    "Invalid node offset in checkpoint"
                );

            if (!loaded_lookup.emplace(history, node_id).second)
                throw std::runtime_error(
                    "Duplicate history in checkpoint"
                );
        }

        std::array<uint8_t, ComboIndexer::N_CARDS> loaded_deck;
        read_binary(file, loaded_deck);

        // Restore RNG.
        uint64_t rng_size;
        read_binary(file, rng_size);

        if (rng_size > remaining || rng_size > 1'000'000)
            throw std::runtime_error("Invalid RNG state size");

        std::string rng_state(static_cast<size_t>(rng_size), '\0');

        file.read(
            rng_state.data(),
            static_cast<std::streamsize>(rng_state.size())
        );

        if (!file)
            throw std::runtime_error("Failed to read RNG state");

        std::istringstream rng_stream(rng_state);
        std::mt19937 loaded_gen;

        if (!(rng_stream >> loaded_gen))
            throw std::runtime_error("Invalid RNG state in checkpoint");

        // Commit only after all checkpoint data has been read.
        nodes = std::move(loaded_nodes);
        history_lookup = std::move(loaded_lookup);
		deck = loaded_deck;
        next_iteration = loaded_iteration;
        gen = loaded_gen;

        std::cout
            << "Model loaded: " << filename
            << "\nResuming at iteration: " << next_iteration
            << "\nNodes: " << nodes.size()
            << "\nHistories: " << history_lookup.size()
            << '\n';

        print_nodes_size();
    }

};